#include "MapEngine/FmapCore.hpp"
#include "MapEngine/Json.hpp"

#include <algorithm>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace fs = std::filesystem;

namespace fantasy::studio::map {
namespace {

using json::Value;

std::string readText(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Unable to open FMAP: " + path.string());
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void writeText(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Unable to write FMAP: " + path.string());
    }
    output << content;
}

std::int32_t int32Field(const Value& object, const char* key) {
    const auto& value = object.at(key);
    if (!value.isInteger()) {
        throw std::runtime_error(std::string("FMAP field must be integer: ") + key);
    }
    const auto integer = value.asInteger();
    if (integer < INT32_MIN || integer > INT32_MAX) {
        throw std::runtime_error(std::string("FMAP integer out of int32 range: ") + key);
    }
    return static_cast<std::int32_t>(integer);
}

std::int16_t int16Field(const Value& object, const char* key) {
    const auto value = int32Field(object, key);
    if (value < INT16_MIN || value > INT16_MAX) {
        throw std::runtime_error(std::string("FMAP integer out of int16 range: ") + key);
    }
    return static_cast<std::int16_t>(value);
}

std::string stringField(const Value& object, const char* key) {
    const auto& value = object.at(key);
    if (!value.isString()) {
        throw std::runtime_error(std::string("FMAP field must be string: ") + key);
    }
    return value.asString();
}

Position positionFromJson(const Value& value) {
    if (!value.isObject()) throw std::runtime_error("FMAP position must be an object");
    return Position{int32Field(value, "x"), int32Field(value, "y"), int16Field(value, "z")};
}

Size sizeFromJson(const Value& value) {
    if (!value.isObject()) throw std::runtime_error("FMAP size must be an object");
    return Size{int32Field(value, "width"), int32Field(value, "height")};
}

std::vector<std::string> stringArray(const Value& value, const char* label) {
    if (!value.isArray()) {
        throw std::runtime_error(std::string("FMAP ") + label + " must be an array");
    }
    std::vector<std::string> result;
    for (const auto& entry : value.asArray()) {
        if (!entry.isString()) {
            throw std::runtime_error(std::string("FMAP ") + label + " entries must be strings");
        }
        result.push_back(entry.asString());
    }
    return result;
}

Value positionToJson(const Position& position) {
    return Value::Object{{"x", position.x}, {"y", position.y}, {"z", static_cast<int>(position.z)}};
}

Value sizeToJson(const Size& size) {
    return Value::Object{{"width", size.width}, {"height", size.height}};
}

Value stringsToJson(const std::vector<std::string>& values) {
    Value::Array array;
    array.reserve(values.size());
    for (const auto& value : values) array.emplace_back(value);
    return array;
}

bool containsSpawn(const Region& region, const Position& spawn) {
    if (spawn.x < region.origin.x || spawn.y < region.origin.y) return false;
    if (spawn.x >= region.origin.x + region.size.width || spawn.y >= region.origin.y + region.size.height) return false;
    return std::any_of(region.chunks.begin(), region.chunks.end(), [&](const Chunk& chunk) {
        return chunk.floor == spawn.z;
    });
}

std::string locationText(const std::string& regionId, const Chunk& chunk, const Tile& tile) {
    std::ostringstream out;
    out << "region=" << regionId << " chunk=" << chunk.x << ',' << chunk.y << ',' << chunk.floor
        << " tile=" << tile.x << ',' << tile.y;
    return out.str();
}

} // namespace

bool isSemanticAssetKey(const std::string& key) {
    static const std::regex pattern(R"(^[a-z0-9]+(?:[._-][a-z0-9]+)+$)");
    return std::regex_match(key, pattern);
}

std::vector<std::string> validateWorld(const World& world) {
    std::vector<std::string> errors;
    if (world.info.id.empty()) errors.emplace_back("world.id cannot be empty");
    if (world.info.name.empty()) errors.emplace_back("world.name cannot be empty");
    if (world.info.tileSize <= 0) errors.emplace_back("world.tileSize must be positive");

    std::set<std::string> regionIds;
    bool spawnCovered = false;

    for (const auto& region : world.regions) {
        if (region.id.empty()) errors.emplace_back("region.id cannot be empty");
        if (!regionIds.insert(region.id).second) errors.emplace_back("duplicate region id: " + region.id);
        if (region.size.width <= 0 || region.size.height <= 0) {
            errors.emplace_back("region size must be positive: " + region.id);
        }
        if (containsSpawn(region, world.developmentSpawn)) spawnCovered = true;

        std::set<std::tuple<std::int32_t, std::int32_t, std::int16_t>> chunkKeys;
        for (const auto& chunk : region.chunks) {
            const auto chunkKey = std::make_tuple(chunk.x, chunk.y, chunk.floor);
            if (!chunkKeys.insert(chunkKey).second) {
                errors.emplace_back("duplicate chunk in region: " + region.id);
            }

            std::set<std::pair<std::int32_t, std::int32_t>> tileKeys;
            for (const auto& tile : chunk.tiles) {
                if (!tileKeys.emplace(tile.x, tile.y).second) {
                    errors.emplace_back("duplicate tile: " + locationText(region.id, chunk, tile));
                }
                if (!isSemanticAssetKey(tile.ground)) {
                    errors.emplace_back("invalid semantic ground key '" + tile.ground + "' at " + locationText(region.id, chunk, tile));
                }

                std::set<std::string> objects;
                for (const auto& object : tile.objects) {
                    if (!isSemanticAssetKey(object)) {
                        errors.emplace_back("invalid semantic object key '" + object + "' at " + locationText(region.id, chunk, tile));
                    }
                    if (!objects.insert(object).second) {
                        errors.emplace_back("duplicate object key '" + object + "' at " + locationText(region.id, chunk, tile));
                    }
                }

                for (const auto& tag : tile.tags) {
                    if (tag.empty()) {
                        errors.emplace_back("empty tag at " + locationText(region.id, chunk, tile));
                    }
                }
            }
        }
    }

    if (!world.regions.empty() && !spawnCovered) {
        errors.emplace_back("developmentSpawn is not covered by a region/chunk floor");
    }

    return errors;
}

void requireValidWorld(const World& world) {
    const auto errors = validateWorld(world);
    if (errors.empty()) return;
    std::ostringstream message;
    message << "FMAP semantic validation failed:";
    for (const auto& error : errors) message << "\n- " << error;
    throw std::runtime_error(message.str());
}

World loadFmap(const fs::path& path) {
    const Value root = json::parse(readText(path));
    if (!root.isObject()) throw std::runtime_error("FMAP root must be an object");
    if (stringField(root, "format") != "FMAP") throw std::runtime_error("FMAP format marker missing");
    if (int32Field(root, "version") != 0) throw std::runtime_error("Unsupported FMAP version; expected 0");

    const auto& worldJson = root.at("world");
    if (!worldJson.isObject()) throw std::runtime_error("FMAP world must be an object");

    World world;
    world.info.id = stringField(worldJson, "id");
    world.info.name = stringField(worldJson, "name");
    world.info.tileSize = int32Field(worldJson, "tileSize");
    world.developmentSpawn = positionFromJson(root.at("developmentSpawn"));

    const auto& regions = root.at("regions");
    if (!regions.isArray()) throw std::runtime_error("FMAP regions must be an array");

    for (const auto& regionJson : regions.asArray()) {
        if (!regionJson.isObject()) throw std::runtime_error("FMAP region must be an object");
        Region region;
        region.id = stringField(regionJson, "id");
        region.origin = positionFromJson(regionJson.at("origin"));
        region.size = sizeFromJson(regionJson.at("size"));

        const auto& chunks = regionJson.at("chunks");
        if (!chunks.isArray()) throw std::runtime_error("FMAP chunks must be an array");
        for (const auto& chunkJson : chunks.asArray()) {
            if (!chunkJson.isObject()) throw std::runtime_error("FMAP chunk must be an object");
            Chunk chunk;
            chunk.x = int32Field(chunkJson, "x");
            chunk.y = int32Field(chunkJson, "y");
            chunk.floor = int16Field(chunkJson, "floor");

            const auto& tiles = chunkJson.at("tiles");
            if (!tiles.isArray()) throw std::runtime_error("FMAP tiles must be an array");
            for (const auto& tileJson : tiles.asArray()) {
                if (!tileJson.isObject()) throw std::runtime_error("FMAP tile must be an object");
                Tile tile;
                tile.x = int32Field(tileJson, "x");
                tile.y = int32Field(tileJson, "y");
                tile.ground = stringField(tileJson, "ground");
                tile.objects = stringArray(tileJson.at("objects"), "objects");
                tile.tags = stringArray(tileJson.at("tags"), "tags");
                chunk.tiles.push_back(std::move(tile));
            }
            region.chunks.push_back(std::move(chunk));
        }
        world.regions.push_back(std::move(region));
    }

    requireValidWorld(world);
    return world;
}

void saveFmap(const World& world, const fs::path& path) {
    requireValidWorld(world);

    Value::Array regions;
    for (const auto& region : world.regions) {
        Value::Array chunks;
        for (const auto& chunk : region.chunks) {
            Value::Array tiles;
            for (const auto& tile : chunk.tiles) {
                tiles.emplace_back(Value::Object{
                    {"ground", tile.ground},
                    {"objects", stringsToJson(tile.objects)},
                    {"tags", stringsToJson(tile.tags)},
                    {"x", tile.x},
                    {"y", tile.y}
                });
            }
            chunks.emplace_back(Value::Object{
                {"floor", static_cast<int>(chunk.floor)},
                {"tiles", std::move(tiles)},
                {"x", chunk.x},
                {"y", chunk.y}
            });
        }
        regions.emplace_back(Value::Object{
            {"chunks", std::move(chunks)},
            {"id", region.id},
            {"origin", positionToJson(region.origin)},
            {"size", sizeToJson(region.size)}
        });
    }

    Value root(Value::Object{
        {"developmentSpawn", positionToJson(world.developmentSpawn)},
        {"format", "FMAP"},
        {"regions", std::move(regions)},
        {"version", 0},
        {"world", Value::Object{
            {"id", world.info.id},
            {"name", world.info.name},
            {"tileSize", world.info.tileSize}
        }}
    });

    writeText(path, json::stringify(root, 2));
}

MapDocument::MapDocument(World world) : world_(std::move(world)) {
    requireValidWorld(world_);
}

void MapDocument::beginTransaction(std::string label) {
    if (transactionOpen_) throw std::runtime_error("Map transaction already open");
    transactionOpen_ = true;
    activeTransaction_ = Transaction{std::move(label), {}};
}

void MapDocument::commit() {
    requireTransaction();
    transactionOpen_ = false;
    if (!activeTransaction_.mutations.empty()) {
        undoStack_.push_back(std::move(activeTransaction_));
        redoStack_.clear();
    }
    activeTransaction_ = {};
    requireValidWorld(world_);
}

void MapDocument::rollback() {
    requireTransaction();
    for (auto it = activeTransaction_.mutations.rbegin(); it != activeTransaction_.mutations.rend(); ++it) {
        applyInverse(*it);
    }
    transactionOpen_ = false;
    activeTransaction_ = {};
    requireValidWorld(world_);
}

void MapDocument::undo() {
    if (transactionOpen_) throw std::runtime_error("Cannot undo while a transaction is open");
    if (undoStack_.empty()) throw std::runtime_error("Nothing to undo");
    Transaction transaction = std::move(undoStack_.back());
    undoStack_.pop_back();
    for (auto it = transaction.mutations.rbegin(); it != transaction.mutations.rend(); ++it) {
        applyInverse(*it);
    }
    redoStack_.push_back(std::move(transaction));
    requireValidWorld(world_);
}

void MapDocument::redo() {
    if (transactionOpen_) throw std::runtime_error("Cannot redo while a transaction is open");
    if (redoStack_.empty()) throw std::runtime_error("Nothing to redo");
    Transaction transaction = std::move(redoStack_.back());
    redoStack_.pop_back();
    for (const auto& mutation : transaction.mutations) {
        applyForward(mutation);
    }
    undoStack_.push_back(std::move(transaction));
    requireValidWorld(world_);
}

void MapDocument::setGround(const TileLocator& locator, const std::string& ground) {
    requireTransaction();
    if (!isSemanticAssetKey(ground)) throw std::runtime_error("Invalid semantic ground key: " + ground);
    Tile& tile = requireTile(locator);
    if (tile.ground == ground) return;
    activeTransaction_.mutations.push_back(Mutation{MutationKind::SetGround, locator, tile.ground, ground, 0});
    tile.ground = ground;
}

void MapDocument::addObject(const TileLocator& locator, const std::string& objectKey) {
    requireTransaction();
    if (!isSemanticAssetKey(objectKey)) throw std::runtime_error("Invalid semantic object key: " + objectKey);
    Tile& tile = requireTile(locator);
    if (std::find(tile.objects.begin(), tile.objects.end(), objectKey) != tile.objects.end()) {
        throw std::runtime_error("Object already present on tile: " + objectKey);
    }
    const std::size_t index = tile.objects.size();
    tile.objects.push_back(objectKey);
    activeTransaction_.mutations.push_back(Mutation{MutationKind::AddObject, locator, {}, objectKey, index});
}

void MapDocument::removeObject(const TileLocator& locator, const std::string& objectKey) {
    requireTransaction();
    Tile& tile = requireTile(locator);
    const auto it = std::find(tile.objects.begin(), tile.objects.end(), objectKey);
    if (it == tile.objects.end()) throw std::runtime_error("Object not present on tile: " + objectKey);
    const std::size_t index = static_cast<std::size_t>(std::distance(tile.objects.begin(), it));
    tile.objects.erase(it);
    activeTransaction_.mutations.push_back(Mutation{MutationKind::RemoveObject, locator, objectKey, {}, index});
}

Tile& MapDocument::requireTile(const TileLocator& locator) {
    for (auto& region : world_.regions) {
        if (region.id != locator.regionId) continue;
        for (auto& chunk : region.chunks) {
            if (chunk.x != locator.chunkX || chunk.y != locator.chunkY || chunk.floor != locator.floor) continue;
            for (auto& tile : chunk.tiles) {
                if (tile.x == locator.tileX && tile.y == locator.tileY) return tile;
            }
        }
    }
    throw std::runtime_error("Tile locator not found");
}

const Tile& MapDocument::requireTile(const TileLocator& locator) const {
    return const_cast<MapDocument*>(this)->requireTile(locator);
}

void MapDocument::applyForward(const Mutation& mutation) {
    Tile& tile = requireTile(mutation.locator);
    switch (mutation.kind) {
        case MutationKind::SetGround:
            tile.ground = mutation.afterValue;
            break;
        case MutationKind::AddObject:
            if (mutation.index > tile.objects.size()) throw std::runtime_error("Redo object index out of range");
            tile.objects.insert(tile.objects.begin() + static_cast<std::ptrdiff_t>(mutation.index), mutation.afterValue);
            break;
        case MutationKind::RemoveObject:
            if (mutation.index >= tile.objects.size() || tile.objects[mutation.index] != mutation.beforeValue) {
                throw std::runtime_error("Redo remove object state mismatch");
            }
            tile.objects.erase(tile.objects.begin() + static_cast<std::ptrdiff_t>(mutation.index));
            break;
    }
}

void MapDocument::applyInverse(const Mutation& mutation) {
    Tile& tile = requireTile(mutation.locator);
    switch (mutation.kind) {
        case MutationKind::SetGround:
            tile.ground = mutation.beforeValue;
            break;
        case MutationKind::AddObject:
            if (mutation.index >= tile.objects.size() || tile.objects[mutation.index] != mutation.afterValue) {
                throw std::runtime_error("Undo add object state mismatch");
            }
            tile.objects.erase(tile.objects.begin() + static_cast<std::ptrdiff_t>(mutation.index));
            break;
        case MutationKind::RemoveObject:
            if (mutation.index > tile.objects.size()) throw std::runtime_error("Undo object index out of range");
            tile.objects.insert(tile.objects.begin() + static_cast<std::ptrdiff_t>(mutation.index), mutation.beforeValue);
            break;
    }
}

void MapDocument::requireTransaction() const {
    if (!transactionOpen_) throw std::runtime_error("Map mutation requires an open transaction");
}

} // namespace fantasy::studio::map
