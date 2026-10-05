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
        const fs::path mapPath = repoRoot / "Game/Maps/World/world.fmap.json";
        WorldRuntime runtime = WorldRuntime::load(mapPath);

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

        bool occupiedSpawnRejected = false;
        try {
            runtime.spawnEntity("Duplicate Spawn", spawn);
        } catch (...) {
            occupiedSpawnRejected = true;
        }
        require(occupiedSpawnRejected, "occupied spawn tile must reject another entity");

        require(runtime.moveEntity(player, 1, 0), "player should move east from spawn");
        require(runtime.entity(player)->position.x == 101, "east movement position mismatch");

        const MapPosition roadEdge{99, 99, 7};
        const auto walker = runtime.spawnEntity("Chunk Walker", roadEdge);
        require(runtime.moveEntity(walker, 1, 0), "movement must cross chunk boundary");
        require(runtime.entity(walker)->position == MapPosition{100, 99, 7}, "cross chunk movement resolved wrong position");

        require(!runtime.moveEntity(walker, 1, 1), "diagonal movement is not enabled in F04 slice");
        require(!runtime.isWalkable(MapPosition{5000, 5000, 7}), "missing tile must not be walkable");

        bool scheduled = false;
        runtime.scheduleAfter(2, [&scheduled] { scheduled = true; });
        require(runtime.pendingTaskCount() == 1, "scheduled task should be pending");
        runtime.tick();
        require(!scheduled, "scheduled task fired too early");
        runtime.tick();
        require(scheduled, "scheduled task did not fire on due tick");
        require(runtime.pendingTaskCount() == 0, "completed scheduled task should be removed");
        require(runtime.tickCount() == 2, "runtime tick counter mismatch");

        bool cancelledRan = false;
        const auto cancelledTask = runtime.scheduleAfter(1, [&cancelledRan] { cancelledRan = true; });
        require(runtime.cancelTask(cancelledTask), "scheduled task should cancel successfully");
        runtime.tick();
        require(!cancelledRan, "cancelled task must not run");

        runtime.stop();
        require(runtime.state() == RuntimeState::Stopped, "runtime should stop cleanly");
        require(runtime.entityCount() == 0, "entities should be cleared on stop");
        require(runtime.pendingTaskCount() == 0, "scheduler should be cleared on stop");

        auto blockedWorld = fantasy::fmap::loadFmap(mapPath);
        blockedWorld.regions.at(0).chunks.at(0).tiles.at(0).tags.push_back("blocked");
        WorldRuntime blockedRuntime(std::move(blockedWorld));
        blockedRuntime.start();
        const MapPosition blockedPosition{96, 96, 7};
        require(!blockedRuntime.isWalkable(blockedPosition), "blocked-tagged tile must be non-walkable");
        bool blockedSpawnRejected = false;
        try {
            blockedRuntime.spawnEntity("Blocked Spawn", blockedPosition);
        } catch (...) {
            blockedSpawnRejected = true;
        }
        require(blockedSpawnRejected, "entity spawn on blocked tile must be rejected");
        blockedRuntime.stop();

        std::cout << "WorldRuntimeTests PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "WorldRuntimeTests FAIL: " << error.what() << '\n';
        return 1;
    }
}
