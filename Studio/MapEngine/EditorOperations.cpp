#include "MapEngine/EditorOperations.hpp"

#include <algorithm>
#include <deque>
#include <map>
#include <set>
#include <stdexcept>
#include <type_traits>
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

struct IndexedTile {
    const Tile* tile = nullptr;
    TileLocator locator;
};

using LocalPoint = std::pair<std::int32_t, std::int32_t>;

std::map<LocalPoint, IndexedTile> indexRegionFloor(const World& world, const TileLocator& locator) {
    std::map<LocalPoint, IndexedTile> result;
    bool regionFound = false;

    for (const auto& region : world.regions) {
        if (region.id != locator.regionId) continue;
        regionFound = true;

        for (const auto& chunk : region.chunks) {
            if (chunk.floor != locator.floor) continue;

            for (const auto& tile : chunk.tiles) {
                const LocalPoint point{chunk.x + tile.x, chunk.y + tile.y};
                const auto [it, inserted] = result.emplace(
                    point,
                    IndexedTile{
                        &tile,
                        TileLocator{region.id, chunk.x, chunk.y, chunk.floor, tile.x, tile.y}
                    });
                if (!inserted) {
                    throw std::runtime_error(
                        "FMAP coordinate collision across chunks at region-local tile " +
                        std::to_string(point.first) + "," + std::to_string(point.second));
                }
            }
        }
    }

    if (!regionFound) {
        throw std::runtime_error("Editor operation region not found: " + locator.regionId);
    }
    return result;
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

    const auto tiles = indexRegionFloor(world, locator);
    const LocalPoint start{locator.chunkX + locator.tileX, locator.chunkY + locator.tileY};

    std::deque<LocalPoint> pending;
    std::set<LocalPoint> visited;
    std::vector<TileLocator> connected;
    pending.push_back(start);

    constexpr LocalPoint directions[] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}
    };

    while (!pending.empty()) {
        const auto point = pending.front();
        pending.pop_front();
        if (!visited.insert(point).second) continue;

        const auto it = tiles.find(point);
        if (it == tiles.end() || it->second.tile->ground != sourceGround) continue;

        connected.push_back(it->second.locator);
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
