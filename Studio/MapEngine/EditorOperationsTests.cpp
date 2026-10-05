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

const Tile& findTile(const World& world, std::int32_t x, std::int32_t y) {
    for (const auto& region : world.regions)
        for (const auto& chunk : region.chunks)
            for (const auto& tile : chunk.tiles)
                if (tile.x == x && tile.y == y) return tile;
    throw std::runtime_error("test tile not found");
}

} // namespace

int main() {
    try {
        const fs::path repoRoot = fs::weakly_canonical(fs::path(FANTASY_REPO_ROOT));
        MapDocument document(loadFmap(repoRoot / "Game/Maps/World/world.fmap.json"));
        const TileLocator spawn{"development", 0, 0, 7, 4, 4};

        const std::size_t filled = EditorOperations::fillConnectedGround(document, spawn, "terrain.stone.basic");
        require(filled == 2, "fixture should fill two connected grass tiles");
        require(findTile(document.world(), 4, 4).ground == "terrain.stone.basic", "spawn tile fill failed");
        require(findTile(document.world(), 5, 4).ground == "terrain.stone.basic", "neighbor tile fill failed");

        document.undo();
        require(findTile(document.world(), 4, 4).ground == "terrain.grass.basic", "fill undo failed at spawn");
        require(findTile(document.world(), 5, 4).ground == "terrain.grass.basic", "fill undo failed at neighbor");
        document.redo();
        require(findTile(document.world(), 4, 4).ground == "terrain.stone.basic", "fill redo failed");

        EditorOperations::addObject(document, spawn, "nature.flower.blue");
        EditorOperations::addObject(document, spawn, "nature.rock.small");
        require(findTile(document.world(), 4, 4).objects.size() == 2, "object setup failed");

        const std::size_t erased = EditorOperations::eraseObjects(document, spawn);
        require(erased == 2, "eraser should remove two objects");
        require(findTile(document.world(), 4, 4).objects.empty(), "eraser did not clear tile objects");

        document.undo();
        require(findTile(document.world(), 4, 4).objects.size() == 2, "eraser undo failed");
        document.redo();
        require(findTile(document.world(), 4, 4).objects.empty(), "eraser redo failed");

        bool rejected = false;
        try {
            EditorOperations::paintGround(document, spawn, "8734");
        } catch (...) {
            rejected = true;
        }
        require(rejected, "editor operation must reject legacy numeric-only asset keys");

        std::cout << "EditorOperationsTests PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "EditorOperationsTests FAIL: " << error.what() << '\n';
        return 1;
    }
}
