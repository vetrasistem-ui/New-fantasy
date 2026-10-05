#include "MapEngine/EditorOperations.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#ifndef FANTASY_REPO_ROOT
#error FANTASY_REPO_ROOT must be defined for EditorOperationsTests
#endif

namespace fs = std::filesystem;
using namespace fantasy::studio::map;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

const Tile& findTileGlobal(
    const World& world,
    const std::string& regionId,
    std::int16_t floor,
    std::int32_t globalX,
    std::int32_t globalY) {

    for (const auto& region : world.regions) {
        if (region.id != regionId) continue;
        for (const auto& chunk : region.chunks) {
            if (chunk.floor != floor) continue;
            for (const auto& tile : chunk.tiles) {
                const auto x = region.origin.x + chunk.x + tile.x;
                const auto y = region.origin.y + chunk.y + tile.y;
                if (x == globalX && y == globalY) return tile;
            }
        }
    }
    throw std::runtime_error("global test tile not found");
}

void testMainWorld(const fs::path& repoRoot) {
    MapDocument document(loadFmap(repoRoot / "Game/Maps/World/world.fmap.json"));
    const TileLocator spawn{"development", 4, 4, 7, 0, 0};

    const std::size_t filled = EditorOperations::fillConnectedGround(document, spawn, "terrain.stone.basic");
    require(filled == 2, "main world should fill the two-tile grass patch");
    require(findTileGlobal(document.world(), "development", 7, 100, 100).ground == "terrain.stone.basic", "spawn tile fill failed");
    require(findTileGlobal(document.world(), "development", 7, 101, 100).ground == "terrain.stone.basic", "neighbor tile fill failed");

    document.undo();
    require(findTileGlobal(document.world(), "development", 7, 100, 100).ground == "terrain.grass.basic", "fill undo failed at spawn");
    require(findTileGlobal(document.world(), "development", 7, 101, 100).ground == "terrain.grass.basic", "fill undo failed at neighbor");
    document.redo();
    require(findTileGlobal(document.world(), "development", 7, 100, 100).ground == "terrain.stone.basic", "fill redo failed");

    EditorOperations::addObject(document, spawn, "nature.flower.blue");
    EditorOperations::addObject(document, spawn, "nature.rock.small");
    require(findTileGlobal(document.world(), "development", 7, 100, 100).objects.size() == 2, "object setup failed");

    const std::size_t erased = EditorOperations::eraseObjects(document, spawn);
    require(erased == 2, "eraser should remove two objects");
    require(findTileGlobal(document.world(), "development", 7, 100, 100).objects.empty(), "eraser did not clear tile objects");

    document.undo();
    require(findTileGlobal(document.world(), "development", 7, 100, 100).objects.size() == 2, "eraser undo failed");
    document.redo();
    require(findTileGlobal(document.world(), "development", 7, 100, 100).objects.empty(), "eraser redo failed");

    bool rejected = false;
    try {
        EditorOperations::paintGround(document, spawn, "8734");
    } catch (...) {
        rejected = true;
    }
    require(rejected, "editor operation must reject legacy numeric-only asset keys");
}

void testMultiChunkFill(const fs::path& repoRoot) {
    MapDocument document(loadFmap(repoRoot / "Game/Maps/World/multichunk-fixture.fmap.json"));
    const TileLocator edgeTile{"crossing", 0, 0, 7, 3, 2};

    const std::size_t filled = EditorOperations::fillConnectedGround(
        document,
        edgeTile,
        "terrain.sand.basic");

    require(filled == 5, "connected fill must cross the chunk boundary and change five grass tiles");
    require(
        findTileGlobal(document.world(), "crossing", 7, 104, 102).ground == "terrain.sand.basic",
        "fill did not cross into the neighboring chunk");
    require(
        findTileGlobal(document.world(), "crossing", 7, 104, 103).ground == "terrain.stone.basic",
        "fill must stop at a different ground type");

    document.undo();
    require(
        findTileGlobal(document.world(), "crossing", 7, 104, 102).ground == "terrain.grass.basic",
        "cross-chunk fill undo failed");

    document.redo();
    require(
        findTileGlobal(document.world(), "crossing", 7, 105, 102).ground == "terrain.sand.basic",
        "cross-chunk fill redo failed");
}

} // namespace

int main() {
    try {
        const fs::path repoRoot = fs::weakly_canonical(fs::path(FANTASY_REPO_ROOT));
        testMainWorld(repoRoot);
        testMultiChunkFill(repoRoot);
        std::cout << "EditorOperationsTests PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "EditorOperationsTests FAIL: " << error.what() << '\n';
        return 1;
    }
}
