#pragma once

#include "../Core/MapDocument.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace fantasy::studio::mapcore {

struct LegacyOtbmWriterConfig {
    std::uint32_t formatVersion = 2;
    std::uint32_t itemsMajorVersion = 3;
    std::uint32_t itemsMinorVersion = 57;
    bool overwrite = false;
};

struct LegacyOtbmWriteReport {
    bool success = false;
    std::size_t tileCount = 0;
    std::size_t itemCount = 0;
    std::size_t tileAreaCount = 0;
    std::uintmax_t bytesWritten = 0;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

class LegacyOtbmWriter {
public:
    [[nodiscard]] LegacyOtbmWriteReport write(
        const std::filesystem::path& outputPath,
        const MapDocument& document,
        const LegacyOtbmWriterConfig& config = {}) const {

        LegacyOtbmWriteReport report;
        const auto tempPath = outputPath.string() + ".fantasy.tmp";
        std::error_code ignored;
        std::filesystem::remove(tempPath, ignored);

        try {
            validateDocument(document, config);
            if (std::filesystem::exists(outputPath) && !config.overwrite) {
                throw std::runtime_error("Refusing to overwrite existing OTBM output: " + outputPath.string());
            }
            if (outputPath.has_parent_path()) {
                std::filesystem::create_directories(outputPath.parent_path());
            }

            NodeStream out(tempPath);
            for (int i = 0; i < 4; ++i) out.rawByte(0U);

            out.beginNode(kRoot);
            out.propertyU32(config.formatVersion);
            out.propertyU16(static_cast<std::uint16_t>(document.metadata().width));
            out.propertyU16(static_cast<std::uint16_t>(document.metadata().height));
            out.propertyU32(config.itemsMajorVersion);
            out.propertyU32(config.itemsMinorVersion);

            out.beginNode(kMapData);
            writeMapMetadata(out, document.metadata());
            writeTileAreas(out, document, report);
            writeTowns(out, document);
            writeWaypoints(out, document);
            out.endNode();
            out.endNode();
            out.close();

            if (report.tileCount != document.map().tileCount()) {
                throw std::runtime_error(
                    "OTBM writer visited " + std::to_string(report.tileCount) +
                    " tiles but canonical map contains " + std::to_string(document.map().tileCount()) +
                    "; tiles outside metadata bounds or floors 0..15 are not allowed");
            }

            if (std::filesystem::exists(outputPath)) {
                std::filesystem::remove(outputPath);
            }
            std::filesystem::rename(tempPath, outputPath);
            report.bytesWritten = std::filesystem::file_size(outputPath);
            report.success = true;
        } catch (const std::exception& error) {
            report.errors.push_back(error.what());
            std::filesystem::remove(tempPath, ignored);
        }
        return report;
    }

private:
    static constexpr std::uint8_t kEscape = 0xFDU;
    static constexpr std::uint8_t kNodeStart = 0xFEU;
    static constexpr std::uint8_t kNodeEnd = 0xFFU;

    static constexpr std::uint8_t kRoot = 0U;
    static constexpr std::uint8_t kMapData = 2U;
    static constexpr std::uint8_t kTileArea = 4U;
    static constexpr std::uint8_t kTile = 5U;
    static constexpr std::uint8_t kItem = 6U;
    static constexpr std::uint8_t kTowns = 12U;
    static constexpr std::uint8_t kTown = 13U;
    static constexpr std::uint8_t kHouseTile = 14U;
    static constexpr std::uint8_t kWaypoints = 15U;
    static constexpr std::uint8_t kWaypoint = 16U;

    static constexpr std::uint8_t kAttrDescription = 1U;
    static constexpr std::uint8_t kAttrTileFlags = 3U;
    static constexpr std::uint8_t kAttrActionId = 4U;
    static constexpr std::uint8_t kAttrUniqueId = 5U;
    static constexpr std::uint8_t kAttrText = 6U;
    static constexpr std::uint8_t kAttrDesc = 7U;
    static constexpr std::uint8_t kAttrTeleDest = 8U;
    static constexpr std::uint8_t kAttrDepotId = 10U;
    static constexpr std::uint8_t kAttrSpawnFile = 11U;
    static constexpr std::uint8_t kAttrRuneCharges = 12U;
    static constexpr std::uint8_t kAttrHouseFile = 13U;
    static constexpr std::uint8_t kAttrHouseDoorId = 14U;
    static constexpr std::uint8_t kAttrCount = 15U;
    static constexpr std::uint8_t kAttrDuration = 16U;
    static constexpr std::uint8_t kAttrDecayingState = 17U;
    static constexpr std::uint8_t kAttrWrittenDate = 18U;
    static constexpr std::uint8_t kAttrWrittenBy = 19U;
    static constexpr std::uint8_t kAttrSleeperGuid = 20U;
    static constexpr std::uint8_t kAttrSleepStart = 21U;
    static constexpr std::uint8_t kAttrCharges = 22U;

    class NodeStream {
    public:
        explicit NodeStream(const std::filesystem::path& path)
            : stream_(path, std::ios::binary | std::ios::trunc) {
            if (!stream_) throw std::runtime_error("Unable to open OTBM output: " + path.string());
            stream_.exceptions(std::ios::badbit | std::ios::failbit);
        }

        void rawByte(std::uint8_t value) {
            buffer_[bufferSize_++] = static_cast<char>(value);
            if (bufferSize_ == buffer_.size()) flush();
        }

        void propertyByte(std::uint8_t value) {
            if (value == kEscape || value == kNodeStart || value == kNodeEnd) rawByte(kEscape);
            rawByte(value);
        }

        void beginNode(std::uint8_t type) {
            rawByte(kNodeStart);
            rawByte(type);
        }

        void endNode() { rawByte(kNodeEnd); }

        void propertyU8(std::uint8_t value) { propertyByte(value); }

        void propertyU16(std::uint16_t value) {
            propertyByte(static_cast<std::uint8_t>(value & 0xFFU));
            propertyByte(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
        }

        void propertyU32(std::uint32_t value) {
            propertyByte(static_cast<std::uint8_t>(value & 0xFFU));
            propertyByte(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
            propertyByte(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
            propertyByte(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
        }

        void propertyI32(std::int32_t value) {
            propertyU32(std::bit_cast<std::uint32_t>(value));
        }

        void propertyString(const std::string& value) {
            if (value.size() > std::numeric_limits<std::uint16_t>::max()) {
                throw std::runtime_error("OTBM string exceeds uint16 length");
            }
            propertyU16(static_cast<std::uint16_t>(value.size()));
            for (const unsigned char byte : value) propertyByte(static_cast<std::uint8_t>(byte));
        }

        void close() {
            flush();
            stream_.close();
        }

    private:
        void flush() {
            if (bufferSize_ == 0U) return;
            stream_.write(buffer_.data(), static_cast<std::streamsize>(bufferSize_));
            bufferSize_ = 0U;
        }

        static constexpr std::size_t kBufferBytes = 1024U * 1024U;
        std::ofstream stream_;
        std::array<char, kBufferBytes> buffer_{};
        std::size_t bufferSize_ = 0U;
    };

    static void validateDocument(const MapDocument& document, const LegacyOtbmWriterConfig& config) {
        if (config.formatVersion != 2U) {
            throw std::runtime_error("TFS 1.4.2 writer gate only permits OTBM format version 2 (OTBM v3)");
        }
        if (document.metadata().width == 0U || document.metadata().height == 0U) {
            throw std::runtime_error("OTBM writer requires non-zero map width and height");
        }
        if (document.metadata().width > std::numeric_limits<std::uint16_t>::max() ||
            document.metadata().height > std::numeric_limits<std::uint16_t>::max()) {
            throw std::runtime_error("OTBM v3 width/height exceed uint16 range");
        }
    }

    static void writeMapMetadata(NodeStream& out, const MapMetadata& metadata) {
        if (!metadata.description.empty()) {
            out.propertyU8(kAttrDescription);
            out.propertyString(metadata.description);
        }
        if (!metadata.spawnFile.empty()) {
            out.propertyU8(kAttrSpawnFile);
            out.propertyString(metadata.spawnFile);
        }
        if (!metadata.houseFile.empty()) {
            out.propertyU8(kAttrHouseFile);
            out.propertyString(metadata.houseFile);
        }
    }

    static std::int64_t integerAttribute(const AttributeValue& value, const std::string& key) {
        if (const auto* number = std::get_if<std::int64_t>(&value)) return *number;
        throw std::runtime_error("OTBM attribute '" + key + "' requires an integer value");
    }

    static const std::string& stringAttribute(const AttributeValue& value, const std::string& key) {
        if (const auto* text = std::get_if<std::string>(&value)) return *text;
        throw std::runtime_error("OTBM attribute '" + key + "' requires a string value");
    }

    static const Position& positionAttribute(const AttributeValue& value, const std::string& key) {
        if (const auto* position = std::get_if<Position>(&value)) return *position;
        throw std::runtime_error("OTBM attribute '" + key + "' requires a position value");
    }

    template <typename T>
    static T checkedUnsigned(std::int64_t value, const std::string& key) {
        if (value < 0 || static_cast<std::uint64_t>(value) > std::numeric_limits<T>::max()) {
            throw std::runtime_error("OTBM attribute '" + key + "' is outside target integer range");
        }
        return static_cast<T>(value);
    }

    static std::int32_t checkedI32(std::int64_t value, const std::string& key) {
        if (value < std::numeric_limits<std::int32_t>::min() || value > std::numeric_limits<std::int32_t>::max()) {
            throw std::runtime_error("OTBM attribute '" + key + "' is outside int32 range");
        }
        return static_cast<std::int32_t>(value);
    }

    static void writePosition(NodeStream& out, const Position& position, const std::string& context) {
        if (position.x < 0 || position.y < 0 || position.z < 0 ||
            position.x > std::numeric_limits<std::uint16_t>::max() ||
            position.y > std::numeric_limits<std::uint16_t>::max() ||
            position.z > std::numeric_limits<std::uint8_t>::max()) {
            throw std::runtime_error("OTBM position out of range for " + context);
        }
        out.propertyU16(static_cast<std::uint16_t>(position.x));
        out.propertyU16(static_cast<std::uint16_t>(position.y));
        out.propertyU8(static_cast<std::uint8_t>(position.z));
    }

    static bool writeItemAttributes(NodeStream& out, const Item& item) {
        bool subtypeExplicit = false;
        for (const auto& [key, value] : item.attributes) {
            if (key == "count") {
                out.propertyU8(kAttrCount);
                out.propertyU8(checkedUnsigned<std::uint8_t>(integerAttribute(value, key), key));
                subtypeExplicit = true;
            } else if (key == "runeCharges") {
                out.propertyU8(kAttrRuneCharges);
                out.propertyU8(checkedUnsigned<std::uint8_t>(integerAttribute(value, key), key));
                subtypeExplicit = true;
            } else if (key == "actionId") {
                out.propertyU8(kAttrActionId);
                out.propertyU16(checkedUnsigned<std::uint16_t>(integerAttribute(value, key), key));
            } else if (key == "uniqueId") {
                out.propertyU8(kAttrUniqueId);
                out.propertyU16(checkedUnsigned<std::uint16_t>(integerAttribute(value, key), key));
            } else if (key == "text") {
                out.propertyU8(kAttrText);
                out.propertyString(stringAttribute(value, key));
            } else if (key == "description") {
                out.propertyU8(kAttrDesc);
                out.propertyString(stringAttribute(value, key));
            } else if (key == "teleportDestination") {
                out.propertyU8(kAttrTeleDest);
                writePosition(out, positionAttribute(value, key), key);
            } else if (key == "depotId") {
                out.propertyU8(kAttrDepotId);
                out.propertyU16(checkedUnsigned<std::uint16_t>(integerAttribute(value, key), key));
            } else if (key == "houseDoorId") {
                out.propertyU8(kAttrHouseDoorId);
                out.propertyU8(checkedUnsigned<std::uint8_t>(integerAttribute(value, key), key));
            } else if (key == "duration") {
                out.propertyU8(kAttrDuration);
                out.propertyI32(checkedI32(integerAttribute(value, key), key));
            } else if (key == "decayingState") {
                out.propertyU8(kAttrDecayingState);
                out.propertyU8(checkedUnsigned<std::uint8_t>(integerAttribute(value, key), key));
            } else if (key == "writtenDate") {
                out.propertyU8(kAttrWrittenDate);
                out.propertyU32(checkedUnsigned<std::uint32_t>(integerAttribute(value, key), key));
            } else if (key == "writtenBy") {
                out.propertyU8(kAttrWrittenBy);
                out.propertyString(stringAttribute(value, key));
            } else if (key == "sleeperGuid") {
                out.propertyU8(kAttrSleeperGuid);
                out.propertyU32(checkedUnsigned<std::uint32_t>(integerAttribute(value, key), key));
            } else if (key == "sleepStart") {
                out.propertyU8(kAttrSleepStart);
                out.propertyU32(checkedUnsigned<std::uint32_t>(integerAttribute(value, key), key));
            } else if (key == "charges") {
                out.propertyU8(kAttrCharges);
                out.propertyU16(checkedUnsigned<std::uint16_t>(integerAttribute(value, key), key));
                subtypeExplicit = true;
            } else {
                throw std::runtime_error("Unsupported canonical item attribute for OTBM v3 writer: " + key);
            }
        }
        return subtypeExplicit;
    }

    static void writeItem(NodeStream& out, const Item& item, LegacyOtbmWriteReport& report) {
        if (item.serverId == 0U || item.serverId > std::numeric_limits<std::uint16_t>::max()) {
            throw std::runtime_error("OTBM item server id is outside uint16 range");
        }

        out.beginNode(kItem);
        out.propertyU16(static_cast<std::uint16_t>(item.serverId));
        const bool subtypeExplicit = writeItemAttributes(out, item);
        if (!subtypeExplicit && item.countOrSubtype != 1U) {
            if (item.countOrSubtype > std::numeric_limits<std::uint8_t>::max()) {
                throw std::runtime_error("Item subtype/count exceeds OTBM ATTR_COUNT uint8 range without explicit attribute");
            }
            out.propertyU8(kAttrCount);
            out.propertyU8(static_cast<std::uint8_t>(item.countOrSubtype));
        }

        ++report.itemCount;
        for (const Item& child : item.contents) writeItem(out, child, report);
        out.endNode();
    }

    static void writeTile(
        NodeStream& out,
        const Tile& tile,
        std::uint32_t baseX,
        std::uint32_t baseY,
        LegacyOtbmWriteReport& report) {

        if (!tile.attributes.empty()) {
            throw std::runtime_error("Canonical tile attributes are not part of the initial TFS 1.4.2 OTBM writer subset");
        }
        if (tile.creature.has_value() || tile.spawn.has_value()) {
            throw std::runtime_error("Inline creature/spawn placements must be exported through spawn XML, not OTBM tile nodes");
        }
        if (tile.position.x < 0 || tile.position.y < 0 || tile.position.z < 0 || tile.position.z > 15) {
            throw std::runtime_error("OTBM tile position is outside TFS floor/coordinate range");
        }

        const auto offsetX = static_cast<std::int64_t>(tile.position.x) - static_cast<std::int64_t>(baseX);
        const auto offsetY = static_cast<std::int64_t>(tile.position.y) - static_cast<std::int64_t>(baseY);
        if (offsetX < 0 || offsetX > 255 || offsetY < 0 || offsetY > 255) {
            throw std::runtime_error("OTBM tile does not fit its 256x256 tile area");
        }

        out.beginNode(tile.houseId != 0U ? kHouseTile : kTile);
        out.propertyU8(static_cast<std::uint8_t>(offsetX));
        out.propertyU8(static_cast<std::uint8_t>(offsetY));
        if (tile.houseId != 0U) out.propertyU32(tile.houseId);
        if (tile.flags != 0U) {
            out.propertyU8(kAttrTileFlags);
            out.propertyU32(tile.flags);
        }

        if (tile.ground.has_value()) writeItem(out, *tile.ground, report);
        for (const Item& item : tile.items) writeItem(out, item, report);
        out.endNode();
        ++report.tileCount;
    }

    static void writeTileAreas(NodeStream& out, const MapDocument& document, LegacyOtbmWriteReport& report) {
        const std::int32_t width = static_cast<std::int32_t>(document.metadata().width);
        const std::int32_t height = static_cast<std::int32_t>(document.metadata().height);

        for (std::int16_t z = 0; z <= 15; ++z) {
            for (std::int32_t baseY = 0; baseY < height; baseY += 256) {
                for (std::int32_t baseX = 0; baseX < width; baseX += 256) {
                    const std::int32_t maxX = std::min(baseX + 255, width - 1);
                    const std::int32_t maxY = std::min(baseY + 255, height - 1);
                    std::vector<const Tile*> tiles;
                    document.map().forEachTileInRect(
                        z,
                        MapStorage::Rect{baseX, baseY, maxX, maxY},
                        [&](const Tile& tile) { tiles.push_back(&tile); });
                    if (tiles.empty()) continue;

                    std::sort(tiles.begin(), tiles.end(), [](const Tile* left, const Tile* right) {
                        if (left->position.y != right->position.y) return left->position.y < right->position.y;
                        return left->position.x < right->position.x;
                    });

                    out.beginNode(kTileArea);
                    out.propertyU16(static_cast<std::uint16_t>(baseX));
                    out.propertyU16(static_cast<std::uint16_t>(baseY));
                    out.propertyU8(static_cast<std::uint8_t>(z));
                    for (const Tile* tile : tiles) {
                        writeTile(out, *tile, static_cast<std::uint32_t>(baseX), static_cast<std::uint32_t>(baseY), report);
                    }
                    out.endNode();
                    ++report.tileAreaCount;
                }
            }
        }
    }

    static void writeTowns(NodeStream& out, const MapDocument& document) {
        if (document.map().towns().empty()) return;
        std::vector<const Town*> towns;
        towns.reserve(document.map().towns().size());
        for (const auto& [id, town] : document.map().towns()) {
            (void)id;
            towns.push_back(&town);
        }
        std::sort(towns.begin(), towns.end(), [](const Town* left, const Town* right) { return left->id < right->id; });

        out.beginNode(kTowns);
        for (const Town* town : towns) {
            out.beginNode(kTown);
            out.propertyU32(town->id);
            out.propertyString(town->name);
            writePosition(out, town->templePosition, "town temple");
            out.endNode();
        }
        out.endNode();
    }

    static void writeWaypoints(NodeStream& out, const MapDocument& document) {
        if (document.map().waypoints().empty()) return;
        std::vector<const Waypoint*> waypoints;
        waypoints.reserve(document.map().waypoints().size());
        for (const auto& [name, waypoint] : document.map().waypoints()) {
            (void)name;
            waypoints.push_back(&waypoint);
        }
        std::sort(waypoints.begin(), waypoints.end(), [](const Waypoint* left, const Waypoint* right) {
            return left->name < right->name;
        });

        out.beginNode(kWaypoints);
        for (const Waypoint* waypoint : waypoints) {
            out.beginNode(kWaypoint);
            out.propertyString(waypoint->name);
            writePosition(out, waypoint->position, "waypoint");
            out.endNode();
        }
        out.endNode();
    }
};

} // namespace fantasy::studio::mapcore
