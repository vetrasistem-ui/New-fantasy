#include "MapEngine/EditorOperations.hpp"

#include <algorithm>
#include <deque>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace fantasy::studio::map {
namespace {

const Tile& requireTile(const World& world, const TileLocator& locator) {
    for (const auto& region : world.regions) {
        if (region.id != locator.regionId) continue;
        for (const auto& chunk : region.chunks) {
            if (chunk.x != locator.chunkX || chunk.y != locator.chunkY || chunk.floor != locator.floor) continue;
            for (const auto& tile : chunk.tiles) {
                if (tile.x == locator.tileX && tile.y == locator.tileY) return tile;
            }
        }
    }
    throw std::runtime_error("Editor operation tile not found");
}

const Chunk& requireChunk(const World& world, const TileLocator& locator) {
    for (const auto& region : world.regions) {
        if (region.id != locator.regionId) continue;
        for (const auto& chunk : region.chunks) {
            if (chunk.x == locator.chunkX && chunk.y == locator.chunkY && chunk.floor == locator.floor) {
                return chunk;
            }
        }
    }
    throw std::runtime_error("Editor operation chunk not found");
}

template <typename Action>
auto transaction(MapDocument& document, const std::string& label, Action&& action) -> decltype(action()) {
    document.beginTransaction(label);
    try {
        if constexpr (std::is_void_v<decltype(action())>) {
            action();
            document.commit();
        } else {
            auto result = action();
            document.commit();
            return result;
        }
    } catch (...) {
        try {
            document.rollback();
        } catch (...) {
        }
        throw;
    }
}

} // namespace

void EditorOperations::paintGround(MapDocument& document, const TileLocator& locator, const std::string& ground) {
    transaction(document, "Paint ground", [&] {
        document.setGround(locator, ground);
    });
}

void EditorOperations::addObject(MapDocument& document, const TileLocator& locator, const std::string& objectKey) {
    transaction(document, "Add object", [&] {
        document.addObject(locator, objectKey);
    });
}

void EditorOperations::removeObject(MapDocument& document, const TileLocator& locator, const std::string& objectKey) {
    transaction(document, "Remove object", [&] {
        document.removeObject(locator, objectKey);
    });
}

std::size_t EditorOperations::fillConnectedGround(
    MapDocument& document,
    const TileLocator& locator,
    const std::string& replacementGround) {

    if (!isSemanticAssetKey(replacementGround)) {
        throw std::runtime_error("Invalid semantic ground key: " + replacementGround);
    }

    const auto& world = document.world();
    const auto& startTile = requireTile(world, locator);
    const std::string sourceGround = startTile.ground;
    if (sourceGround == replacementGround) return 0;

    const auto& chunk = requireChunk(world, locator);
    std::map<std::pair<std::int32_t, std::int32_t>, const Tile*> tiles;
    for (const auto& tile : chunk.tiles) {
        tiles[{tile.x, tile.y}] = &tile;
    }

    std::deque<std::pair<std::int32_t, std::int32_t>> pending;
    std::set<std::pair<std::int32_t, std::int32_t>> visited;
    std::vector<TileLocator> connected;
    pending.push_back({locator.tileX, locator.tileY});

    constexpr std::pair<std::int32_t, std::int32_t> directions[] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}
    };

    while (!pending.empty()) {
        const auto point = pending.front();
        pending.pop_front();
        if (!visited.insert(point).second) continue;

        const auto it = tiles.find(point);
        if (it == tiles.end() || it->second->ground != sourceGround) continue;

        connected.push_back(TileLocator{
            locator.regionId,
            locator.chunkX,
            locator.chunkY,
            locator.floor,
            point.first,
            point.second
        });

        for (const auto& [dx, dy] : directions) {
            pending.push_back({point.first + dx, point.second + dy});
        }
    }

    return transaction(document, "Fill connected ground", [&]() -> std::size_t {
        for (const auto& tileLocator : connected) {
            document.setGround(tileLocator, replacementGround);
        }
        return connected.size();
    });
}

std::size_t EditorOperations::eraseObjects(MapDocument& document, const TileLocator& locator) {
    const auto objects = requireTile(document.world(), locator).objects;
    if (objects.empty()) return 0;

    return transaction(document, "Erase tile objects", [&]() -> std::size_t {
        for (const auto& object : objects) {
            document.removeObject(locator, object);
        }
        return objects.size();
    });
}

} // namespace fantasy::studio::map
