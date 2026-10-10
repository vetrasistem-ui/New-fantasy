#include "FantasyMapCommandsV1.hpp"

#include "../Commands/CommandExecutor.hpp"
#include "../Export/LegacyOtbmWriter.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace fantasy::studio::mapgen {
namespace {

using fantasy::assets::LegacyAssetKind;
using foundation::AssetCatalogSourceKind;
using foundation::FantasyAssetCatalog;
using mapcore::CommandExecutor;
using mapcore::CommandOrigin;
using mapcore::Item;
using mapcore::MapCommandBatch;
using mapcore::MapCommandPayload;
using mapcore::MapDocument;
using mapcore::MapMetadata;
using mapcore::MapStorage;
using mapcore::PaintGroundCommand;
using mapcore::PlaceItemCommand;
using mapcore::Position;
using mapcore::Tile;

constexpr std::uint64_t kMaxTilesPerGeneratedOperation = 1'000'000ULL;

struct ResolvedAsset {
    Item item;
    LegacyAssetKind kind = LegacyAssetKind::Unknown;
};

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1U);
}

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> values;
    std::istringstream stream(line);
    std::string token;
    while (stream >> token) values.push_back(std::move(token));
    return values;
}

std::int64_t parseInteger(const std::string& value, const char* label) {
    std::size_t consumed = 0;
    long long parsed = 0;
    try {
        parsed = std::stoll(value, &consumed, 10);
    } catch (...) {
        throw std::invalid_argument(std::string(label) + " must be an integer: " + value);
    }
    if (consumed != value.size()) {
        throw std::invalid_argument(std::string(label) + " must be an integer: " + value);
    }
    return static_cast<std::int64_t>(parsed);
}

std::int32_t parseI32(const std::string& value, const char* label) {
    const auto parsed = parseInteger(value, label);
    if (parsed < std::numeric_limits<std::int32_t>::min() || parsed > std::numeric_limits<std::int32_t>::max()) {
        throw std::out_of_range(std::string(label) + " is outside int32 range");
    }
    return static_cast<std::int32_t>(parsed);
}

std::int16_t parseFloor(const std::string& value) {
    const auto parsed = parseInteger(value, "floor");
    if (parsed < 0 || parsed > 15) throw std::out_of_range("floor must be between 0 and 15");
    return static_cast<std::int16_t>(parsed);
}

std::uint32_t parsePositiveU32(const std::string& value, const char* label) {
    const auto parsed = parseInteger(value, label);
    if (parsed <= 0 || static_cast<std::uint64_t>(parsed) > std::numeric_limits<std::uint32_t>::max()) {
        throw std::out_of_range(std::string(label) + " must be greater than zero");
    }
    return static_cast<std::uint32_t>(parsed);
}

std::uint64_t parseU64(const std::string& value, const char* label) {
    const auto parsed = parseInteger(value, label);
    if (parsed < 0) throw std::out_of_range(std::string(label) + " cannot be negative");
    return static_cast<std::uint64_t>(parsed);
}

ResolvedAsset resolveAsset(
    const std::string& semanticId,
    const FantasyAssetCatalog& catalog,
    const fantasy::assets::FantasyAssetRegistry& registry) {

    const auto* entry = catalog.findEntry(semanticId);
    if (entry == nullptr) throw std::runtime_error("semantic asset is not in Asset Catalog: " + semanticId);
    if (entry->source != AssetCatalogSourceKind::LegacyRegistry) {
        throw std::runtime_error("Map Commands V1 currently requires a legacy_registry asset: " + semanticId);
    }

    const auto* record = registry.findBySemanticKey(entry->assetRef);
    if (record == nullptr) {
        throw std::runtime_error("Asset Catalog reference is missing from Legacy Asset Registry: " + entry->assetRef);
    }
    if (record->serverId == 0U || record->serverId > std::numeric_limits<std::uint16_t>::max()) {
        throw std::runtime_error("legacy server id cannot be exported to OTBM for semantic asset: " + semanticId);
    }

    Item item;
    item.serverId = record->serverId;
    item.clientId = record->clientId;
    return {std::move(item), record->kind};
}

ResolvedAsset resolveGround(
    const std::string& semanticId,
    const FantasyAssetCatalog& catalog,
    const fantasy::assets::FantasyAssetRegistry& registry) {

    auto value = resolveAsset(semanticId, catalog, registry);
    if (value.kind != LegacyAssetKind::Ground) {
        throw std::runtime_error("semantic terrain must resolve to a Ground asset: " + semanticId);
    }
    return value;
}

ResolvedAsset resolveObject(
    const std::string& semanticId,
    const FantasyAssetCatalog& catalog,
    const fantasy::assets::FantasyAssetRegistry& registry) {

    auto value = resolveAsset(semanticId, catalog, registry);
    if (value.kind != LegacyAssetKind::Object && value.kind != LegacyAssetKind::Border) {
        throw std::runtime_error("semantic placed asset must resolve to Object/Border: " + semanticId);
    }
    return value;
}

void requireMap(const MapDocument& document) {
    if (document.metadata().width == 0U || document.metadata().height == 0U) {
        throw std::runtime_error("NEW_REGION must run before map-generation commands");
    }
}

void requireInside(const MapDocument& document, const Position& position) {
    requireMap(document);
    if (position.z < 0 || position.z > 15 || position.x < 0 || position.y < 0 ||
        static_cast<std::uint64_t>(position.x) >= document.metadata().width ||
        static_cast<std::uint64_t>(position.y) >= document.metadata().height) {
        throw std::out_of_range(
            "position is outside generated region: " + std::to_string(position.x) + "," +
            std::to_string(position.y) + "," + std::to_string(position.z));
    }
}

std::vector<Position> rectanglePositions(
    const MapDocument& document,
    std::int32_t x,
    std::int32_t y,
    std::uint32_t width,
    std::uint32_t height,
    std::int16_t z) {

    if (width == 0U || height == 0U) throw std::invalid_argument("rectangle width/height must be greater than zero");
    const std::uint64_t count = static_cast<std::uint64_t>(width) * height;
    if (count > kMaxTilesPerGeneratedOperation) {
        throw std::runtime_error("generated operation exceeds V1 safety limit of 1,000,000 tiles");
    }

    const auto maxX = static_cast<std::int64_t>(x) + static_cast<std::int64_t>(width) - 1;
    const auto maxY = static_cast<std::int64_t>(y) + static_cast<std::int64_t>(height) - 1;
    if (maxX > std::numeric_limits<std::int32_t>::max() || maxY > std::numeric_limits<std::int32_t>::max()) {
        throw std::out_of_range("rectangle exceeds int32 coordinate range");
    }

    requireInside(document, {x, y, z});
    requireInside(document, {static_cast<std::int32_t>(maxX), static_cast<std::int32_t>(maxY), z});

    std::vector<Position> result;
    result.reserve(static_cast<std::size_t>(count));
    for (std::uint32_t dy = 0; dy < height; ++dy) {
        for (std::uint32_t dx = 0; dx < width; ++dx) {
            result.push_back({x + static_cast<std::int32_t>(dx), y + static_cast<std::int32_t>(dy), z});
        }
    }
    return result;
}

void addResult(
    FantasyMapScriptReport& report,
    MapDocument& document,
    std::string requestId,
    std::string label,
    std::vector<MapCommandPayload> commands) {

    MapCommandBatch batch;
    batch.requestId = std::move(requestId);
    batch.label = std::move(label);
    batch.origin = CommandOrigin::AI;
    batch.expectedRevision = document.revision();
    batch.commands = std::move(commands);

    const auto result = CommandExecutor{}.execute(document, batch);
    if (!result.ok()) throw std::runtime_error(result.message);
    report.affectedTiles += result.affectedTiles;
}

std::uint64_t mixSeed(std::uint64_t value) noexcept {
    value ^= value >> 12U;
    value ^= value << 25U;
    value ^= value >> 27U;
    return value * 2685821657736338717ULL;
}

void validateItem(const Item& item) {
    if (item.serverId == 0U || item.serverId > std::numeric_limits<std::uint16_t>::max()) {
        throw std::runtime_error("map contains an item with invalid OTBM server id");
    }
    for (const auto& child : item.contents) validateItem(child);
}

void validateDocument(const MapDocument& document) {
    requireMap(document);
    if (document.metadata().width > std::numeric_limits<std::uint16_t>::max() ||
        document.metadata().height > std::numeric_limits<std::uint16_t>::max()) {
        throw std::runtime_error("generated region exceeds OTBM uint16 dimensions");
    }

    std::size_t visited = 0;
    const MapStorage::Rect bounds{
        0,
        0,
        static_cast<std::int32_t>(document.metadata().width - 1U),
        static_cast<std::int32_t>(document.metadata().height - 1U)};

    for (std::int16_t z = 0; z <= 15; ++z) {
        document.map().forEachTileInRect(z, bounds, [&](const Tile& tile) {
            ++visited;
            requireInside(document, tile.position);
            if (tile.ground.has_value()) validateItem(*tile.ground);
            for (const auto& item : tile.items) validateItem(item);
        });
    }
    if (visited != document.map().tileCount()) {
        throw std::runtime_error("map contains tiles outside region bounds or supported floors");
    }
}

void requireTokenCount(const std::vector<std::string>& tokens, std::size_t expected, const char* usage) {
    if (tokens.size() != expected) throw std::invalid_argument(std::string("usage: ") + usage);
}

} // namespace

FantasyMapScriptReport FantasyMapCommandsV1::execute(
    MapDocument& document,
    std::string_view script,
    const FantasyAssetCatalog& catalog,
    const fantasy::assets::FantasyAssetRegistry& registry,
    const std::filesystem::path& outputRoot) const {

    FantasyMapScriptReport report;
    try {
        catalog.validate();
        std::istringstream input{std::string(script)};
        std::string rawLine;
        std::size_t lineNumber = 0;

        while (std::getline(input, rawLine)) {
            ++lineNumber;
            const auto comment = rawLine.find('#');
            if (comment != std::string::npos) rawLine.erase(comment);
            const auto line = trim(std::move(rawLine));
            if (line.empty()) continue;

            const auto tokens = tokenize(line);
            const auto& op = tokens.front();
            const auto requestId = "mapgen.v1." + std::to_string(lineNumber);

            if (op == "NEW_REGION") {
                requireTokenCount(tokens, 6, "NEW_REGION <name> <width> <height> <z> <terrain.semantic>");
                foundation::requireIdentifier(tokens[1], "generated region name");
                const auto width = parsePositiveU32(tokens[2], "width");
                const auto height = parsePositiveU32(tokens[3], "height");
                if (width > std::numeric_limits<std::uint16_t>::max() || height > std::numeric_limits<std::uint16_t>::max()) {
                    throw std::out_of_range("NEW_REGION dimensions exceed OTBM uint16 range");
                }
                const auto count = static_cast<std::uint64_t>(width) * height;
                if (count > kMaxTilesPerGeneratedOperation) {
                    throw std::runtime_error("NEW_REGION exceeds V1 safety limit of 1,000,000 tiles");
                }
                const auto z = parseFloor(tokens[4]);
                const auto terrain = resolveGround(tokens[5], catalog, registry);

                MapMetadata metadata;
                metadata.width = width;
                metadata.height = height;
                metadata.name = tokens[1];
                metadata.description = "Generated by Fantasy Map Commands V1";
                document.replaceMap({}, std::move(metadata));

                std::vector<Position> positions;
                positions.reserve(static_cast<std::size_t>(count));
                for (std::uint32_t y = 0; y < height; ++y) {
                    for (std::uint32_t x = 0; x < width; ++x) {
                        positions.push_back({static_cast<std::int32_t>(x), static_cast<std::int32_t>(y), z});
                    }
                }
                addResult(report, document, requestId, "New region", {
                    PaintGroundCommand{std::move(positions), terrain.item}
                });
                report.messages.push_back("NEW_REGION " + tokens[1] + " ready");
            } else if (op == "SET_TERRAIN" || op == "PLACE_WATER") {
                requireTokenCount(tokens, 7, "SET_TERRAIN <x> <y> <width> <height> <z> <terrain.semantic>");
                const auto x = parseI32(tokens[1], "x");
                const auto y = parseI32(tokens[2], "y");
                const auto width = parsePositiveU32(tokens[3], "width");
                const auto height = parsePositiveU32(tokens[4], "height");
                const auto z = parseFloor(tokens[5]);
                const auto terrain = resolveGround(tokens[6], catalog, registry);
                auto positions = rectanglePositions(document, x, y, width, height, z);
                addResult(report, document, requestId, op, {
                    PaintGroundCommand{std::move(positions), terrain.item}
                });
            } else if (op == "PLACE_FOREST") {
                requireTokenCount(tokens, 9, "PLACE_FOREST <x> <y> <width> <height> <z> <tree.semantic> <densityPercent> <seed>");
                const auto x = parseI32(tokens[1], "x");
                const auto y = parseI32(tokens[2], "y");
                const auto width = parsePositiveU32(tokens[3], "width");
                const auto height = parsePositiveU32(tokens[4], "height");
                const auto z = parseFloor(tokens[5]);
                const auto tree = resolveObject(tokens[6], catalog, registry);
                const auto density = parseInteger(tokens[7], "densityPercent");
                if (density < 0 || density > 100) throw std::out_of_range("densityPercent must be between 0 and 100");
                const auto seed = parseU64(tokens[8], "seed");
                const auto positions = rectanglePositions(document, x, y, width, height, z);

                std::vector<MapCommandPayload> commands;
                commands.reserve(positions.size());
                for (const auto& position : positions) {
                    if (document.map().findTile(position) == nullptr) continue;
                    const std::uint64_t coordinateSeed = seed ^
                        (static_cast<std::uint64_t>(static_cast<std::uint32_t>(position.x)) << 32U) ^
                        static_cast<std::uint32_t>(position.y) ^
                        (static_cast<std::uint64_t>(static_cast<std::uint16_t>(position.z)) << 56U);
                    if (static_cast<std::int64_t>(mixSeed(coordinateSeed) % 100ULL) >= density) continue;
                    commands.push_back(PlaceItemCommand{position, tree.item, std::nullopt});
                }
                if (!commands.empty()) addResult(report, document, requestId, "Place forest", std::move(commands));
                report.messages.push_back("PLACE_FOREST generated deterministic vegetation");
            } else if (op == "PLACE_BUILDING") {
                requireTokenCount(tokens, 9, "PLACE_BUILDING <x> <y> <width> <height> <z> <floor.semantic> <wall.semantic> <door.semantic>");
                const auto x = parseI32(tokens[1], "x");
                const auto y = parseI32(tokens[2], "y");
                const auto width = parsePositiveU32(tokens[3], "width");
                const auto height = parsePositiveU32(tokens[4], "height");
                if (width < 3U || height < 3U) throw std::invalid_argument("PLACE_BUILDING requires width/height >= 3");
                const auto z = parseFloor(tokens[5]);
                const auto floor = resolveGround(tokens[6], catalog, registry);
                const auto wall = resolveObject(tokens[7], catalog, registry);
                const auto door = resolveObject(tokens[8], catalog, registry);
                auto floorPositions = rectanglePositions(document, x, y, width, height, z);

                std::vector<MapCommandPayload> commands;
                commands.push_back(PaintGroundCommand{std::move(floorPositions), floor.item});
                const std::int32_t right = x + static_cast<std::int32_t>(width) - 1;
                const std::int32_t bottom = y + static_cast<std::int32_t>(height) - 1;
                const std::int32_t doorX = x + static_cast<std::int32_t>(width / 2U);

                for (std::int32_t px = x; px <= right; ++px) {
                    commands.push_back(PlaceItemCommand{{px, y, z}, wall.item, std::nullopt});
                    if (px == doorX) commands.push_back(PlaceItemCommand{{px, bottom, z}, door.item, std::nullopt});
                    else commands.push_back(PlaceItemCommand{{px, bottom, z}, wall.item, std::nullopt});
                }
                for (std::int32_t py = y + 1; py < bottom; ++py) {
                    commands.push_back(PlaceItemCommand{{x, py, z}, wall.item, std::nullopt});
                    commands.push_back(PlaceItemCommand{{right, py, z}, wall.item, std::nullopt});
                }
                addResult(report, document, requestId, "Place building", std::move(commands));
                report.messages.push_back("PLACE_BUILDING generated floor, perimeter walls and south door");
            } else if (op == "CONNECT") {
                requireTokenCount(tokens, 8, "CONNECT <x1> <y1> <x2> <y2> <z> <width> <terrain.semantic>");
                const auto x1 = parseI32(tokens[1], "x1");
                const auto y1 = parseI32(tokens[2], "y1");
                const auto x2 = parseI32(tokens[3], "x2");
                const auto y2 = parseI32(tokens[4], "y2");
                const auto z = parseFloor(tokens[5]);
                const auto width = parsePositiveU32(tokens[6], "width");
                if (width > 31U) throw std::out_of_range("CONNECT width is limited to 31 tiles in V1");
                const auto terrain = resolveGround(tokens[7], catalog, registry);

                std::set<Position> unique;
                const auto offsetStart = -static_cast<std::int32_t>(width / 2U);
                for (std::int32_t px = std::min(x1, x2); px <= std::max(x1, x2); ++px) {
                    for (std::uint32_t offset = 0; offset < width; ++offset) {
                        const Position position{px, y1 + offsetStart + static_cast<std::int32_t>(offset), z};
                        requireInside(document, position);
                        unique.insert(position);
                    }
                }
                for (std::int32_t py = std::min(y1, y2); py <= std::max(y1, y2); ++py) {
                    for (std::uint32_t offset = 0; offset < width; ++offset) {
                        const Position position{x2 + offsetStart + static_cast<std::int32_t>(offset), py, z};
                        requireInside(document, position);
                        unique.insert(position);
                    }
                }
                if (unique.size() > kMaxTilesPerGeneratedOperation) {
                    throw std::runtime_error("CONNECT exceeds V1 safety limit of 1,000,000 tiles");
                }
                std::vector<Position> positions(unique.begin(), unique.end());
                addResult(report, document, requestId, "Connect points", {
                    PaintGroundCommand{std::move(positions), terrain.item}
                });
                report.messages.push_back("CONNECT generated deterministic L-shaped route");
            } else if (op == "VALIDATE") {
                requireTokenCount(tokens, 1, "VALIDATE");
                validateDocument(document);
                report.messages.push_back("VALIDATE PASS");
            } else if (op == "SAVE") {
                requireTokenCount(tokens, 2, "SAVE <project-relative-output.otbm>");
                if (outputRoot.empty()) throw std::invalid_argument("SAVE requires an output root");
                const std::filesystem::path relative = tokens[1];
                if (!foundation::safeRelativePath(relative)) {
                    throw std::invalid_argument("SAVE path must be project-relative and cannot contain '..'");
                }
                validateDocument(document);
                const auto output = (outputRoot / relative).lexically_normal();
                mapcore::LegacyOtbmWriterConfig config;
                config.overwrite = true;
                const auto write = mapcore::LegacyOtbmWriter{}.write(output, document, config);
                if (!write.success) {
                    throw std::runtime_error(write.errors.empty() ? "OTBM writer failed" : write.errors.front());
                }
                report.savedPath = output;
                report.messages.push_back(
                    "SAVE PASS tiles=" + std::to_string(write.tileCount) +
                    " items=" + std::to_string(write.itemCount) +
                    " bytes=" + std::to_string(write.bytesWritten));
            } else {
                throw std::invalid_argument("unknown Fantasy Map Commands V1 operation: " + op);
            }

            ++report.commandsExecuted;
        }

        if (report.commandsExecuted == 0U) throw std::invalid_argument("map script contains no commands");
        report.success = true;
    } catch (const std::exception& error) {
        report.messages.push_back(std::string("ERROR: ") + error.what());
        report.success = false;
    }
    return report;
}

} // namespace fantasy::studio::mapgen
