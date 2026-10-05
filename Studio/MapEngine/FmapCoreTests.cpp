#include "MapEngine/FmapCore.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#ifndef FANTASY_REPO_ROOT
#error FANTASY_REPO_ROOT must be defined for FmapCoreTests
#endif

namespace fs = std::filesystem;
using namespace fantasy::studio::map;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

Tile& firstTile(World& world) {
    return world.regions.at(0).chunks.at(0).tiles.at(0);
}

} // namespace

int main() {
    try {
        const fs::path repoRoot = fs::weakly_canonical(fs::path(FANTASY_REPO_ROOT));
        const fs::path source = repoRoot / "Game/Maps/World/world.fmap.json";
        World world = loadFmap(source);

        require(world.info.id == "fantasy-world", "world id mismatch");
        require(world.regions.size() == 1, "expected one fixture region");
        require(isSemanticAssetKey("terrain.grass.basic"), "valid semantic key rejected");
        require(!isSemanticAssetKey("12345"), "numeric-only legacy style key should be rejected");
        require(validateWorld(world).empty(), "fixture should be semantically valid");

        const fs::path tempRoot = fs::temp_directory_path() / "fantasy-fmap-core-tests";
        const fs::path roundtrip = tempRoot / "roundtrip.fmap.json";
        std::error_code ec;
        fs::remove_all(tempRoot, ec);
        fs::create_directories(tempRoot);

        saveFmap(world, roundtrip);
        const World reopened = loadFmap(roundtrip);
        require(reopened == world, "FMAP semantic roundtrip mismatch");

        MapDocument document(world);
        const TileLocator locator{"development", 0, 0, 7, 4, 4};
        const std::string originalGround = firstTile(document.world()).ground;

        document.beginTransaction("decorate spawn");
        document.setGround(locator, "terrain.stone.basic");
        document.addObject(locator, "nature.flower.blue");
        document.commit();

        require(firstTile(document.world()).ground == "terrain.stone.basic", "ground mutation failed");
        require(firstTile(document.world()).objects.size() == 1, "object add failed");
        require(document.canUndo(), "undo should be available after commit");

        document.undo();
        require(firstTile(document.world()).ground == originalGround, "undo ground failed");
        require(firstTile(document.world()).objects.empty(), "undo object failed");
        require(document.canRedo(), "redo should be available after undo");

        document.redo();
        require(firstTile(document.world()).ground == "terrain.stone.basic", "redo ground failed");
        require(firstTile(document.world()).objects.at(0) == "nature.flower.blue", "redo object failed");

        document.beginTransaction("temporary edit");
        document.removeObject(locator, "nature.flower.blue");
        document.rollback();
        require(firstTile(document.world()).objects.at(0) == "nature.flower.blue", "rollback failed");

        bool rejected = false;
        try {
            document.beginTransaction("invalid key");
            document.setGround(locator, "8734");
        } catch (...) {
            rejected = true;
            document.rollback();
        }
        require(rejected, "legacy numeric asset key should be rejected by mutation API");

        fs::remove_all(tempRoot, ec);
        std::cout << "FmapCoreTests PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FmapCoreTests FAIL: " << error.what() << '\n';
        return 1;
    }
}
