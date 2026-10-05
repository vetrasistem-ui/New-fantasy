#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fantasy::studio::map {

struct Position {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int16_t z = 0;
    bool operator==(const Position&) const = default;
};

struct Size {
    std::int32_t width = 0;
    std::int32_t height = 0;
    bool operator==(const Size&) const = default;
};

struct Tile {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::string ground;
    std::vector<std::string> objects;
    std::vector<std::string> tags;
    bool operator==(const Tile&) const = default;
};

struct Chunk {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int16_t floor = 0;
    std::vector<Tile> tiles;
    bool operator==(const Chunk&) const = default;
};

struct Region {
    std::string id;
    Position origin;
    Size size;
    std::vector<Chunk> chunks;
    bool operator==(const Region&) const = default;
};

struct WorldInfo {
    std::string id;
    std::string name;
    std::int32_t tileSize = 32;
    bool operator==(const WorldInfo&) const = default;
};

struct World {
    WorldInfo info;
    Position developmentSpawn;
    std::vector<Region> regions;
    bool operator==(const World&) const = default;
};

struct TileLocator {
    std::string regionId;
    std::int32_t chunkX = 0;
    std::int32_t chunkY = 0;
    std::int16_t floor = 0;
    std::int32_t tileX = 0;
    std::int32_t tileY = 0;
    bool operator==(const TileLocator&) const = default;
};

bool isSemanticAssetKey(const std::string& key);
std::vector<std::string> validateWorld(const World& world);
void requireValidWorld(const World& world);

World loadFmap(const std::filesystem::path& path);
void saveFmap(const World& world, const std::filesystem::path& path);

class MapDocument {
public:
    explicit MapDocument(World world);

    const World& world() const { return world_; }
    World& world() { return world_; }

    void beginTransaction(std::string label);
    void commit();
    void rollback();
    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }
    void undo();
    void redo();

    void setGround(const TileLocator& locator, const std::string& ground);
    void addObject(const TileLocator& locator, const std::string& objectKey);
    void removeObject(const TileLocator& locator, const std::string& objectKey);

private:
    enum class MutationKind { SetGround, AddObject, RemoveObject };

    struct Mutation {
        MutationKind kind{};
        TileLocator locator;
        std::string beforeValue;
        std::string afterValue;
        std::size_t index = 0;
    };

    struct Transaction {
        std::string label;
        std::vector<Mutation> mutations;
    };

    Tile& requireTile(const TileLocator& locator);
    const Tile& requireTile(const TileLocator& locator) const;
    void applyForward(const Mutation& mutation);
    void applyInverse(const Mutation& mutation);
    void requireTransaction() const;

    World world_;
    bool transactionOpen_ = false;
    Transaction activeTransaction_;
    std::vector<Transaction> undoStack_;
    std::vector<Transaction> redoStack_;
};

} // namespace fantasy::studio::map
