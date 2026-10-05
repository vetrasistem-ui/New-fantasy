#include "Core/WorldRuntime.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#ifndef FANTASY_REPO_ROOT
#error FANTASY_REPO_ROOT must be defined for WorldRuntimeTests
#endif

namespace fs = std::filesystem;
using namespace fantasy::server;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

} // namespace

int main() {
    try {
        const fs::path repoRoot = fs::weakly_canonical(fs::path(FANTASY_REPO_ROOT));
        WorldRuntime runtime = WorldRuntime::load(repoRoot / "Game/Maps/World/world.fmap.json");

        require(runtime.indexedTileCount() == 64, "development world must index 64 tiles");
        require(runtime.state() == RuntimeState::Stopped, "runtime should start stopped");

        runtime.start();
        require(runtime.state() == RuntimeState::Ready, "runtime should become ready");

        const auto spawn = runtime.world().developmentSpawn;
        require(spawn.x == 100 && spawn.y == 100 && spawn.z == 7, "development spawn mismatch");
        require(runtime.tileAt(spawn) != nullptr, "development spawn tile missing");
        require(runtime.isWalkable(spawn), "development spawn should be walkable");

        const auto player = runtime.spawnEntity("Development Player", spawn);
        require(runtime.entity(player) != nullptr, "spawned entity missing");
        require(runtime.moveEntity(player, 1, 0), "player should move east from spawn");
        require(runtime.entity(player)->position.x == 101, "east movement position mismatch");

        const MapPosition roadEdge{99, 99, 7};
        const auto walker = runtime.spawnEntity("Chunk Walker", roadEdge);
        require(runtime.moveEntity(walker, 1, 0), "movement must cross chunk boundary");
        require(runtime.entity(walker)->position == MapPosition{100, 99, 7}, "cross chunk movement resolved wrong position");

        require(!runtime.moveEntity(walker, 1, 1), "diagonal movement is not enabled in F04 slice");
        require(!runtime.isWalkable(MapPosition{5000, 5000, 7}), "missing tile must not be walkable");

        runtime.tick();
        runtime.tick();
        require(runtime.tickCount() == 2, "runtime tick counter mismatch");

        runtime.stop();
        require(runtime.state() == RuntimeState::Stopped, "runtime should stop cleanly");
        require(runtime.entityCount() == 0, "entities should be cleared on stop");

        std::cout << "WorldRuntimeTests PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "WorldRuntimeTests FAIL: " << error.what() << '\n';
        return 1;
    }
}
