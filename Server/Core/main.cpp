#include "Core/WorldRuntime.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

#ifndef FANTASY_SERVER_VERSION
#define FANTASY_SERVER_VERSION "dev"
#endif

#ifndef FANTASY_REPO_ROOT
#define FANTASY_REPO_ROOT "."
#endif

namespace fs = std::filesystem;

namespace fantasy {

int runSmokeTest() {
    const fs::path repoRoot = fs::weakly_canonical(fs::path(FANTASY_REPO_ROOT));
    server::WorldRuntime runtime = server::WorldRuntime::load(repoRoot / "Game/Maps/World/world.fmap.json");

    std::cout << "Fantasy Server " << FANTASY_SERVER_VERSION << "\n";
    std::cout << "state=STARTING\n";
    runtime.start();
    std::cout << "state=READY\n";
    std::cout << "world=" << runtime.world().info.id << "\n";
    std::cout << "tiles=" << runtime.indexedTileCount() << "\n";

    const auto player = runtime.spawnEntity("Smoke Player", runtime.world().developmentSpawn);
    if (!runtime.moveEntity(player, 1, 0)) {
        throw std::runtime_error("smoke player failed to move east from development spawn");
    }
    runtime.tick();

    runtime.stop();
    std::cout << "state=STOPPED\n";
    std::cout << "smoke=PASS\n";
    return 0;
}

int runServer() {
    const fs::path repoRoot = fs::weakly_canonical(fs::path(FANTASY_REPO_ROOT));
    server::WorldRuntime runtime = server::WorldRuntime::load(repoRoot / "Game/Maps/World/world.fmap.json");

    std::cout << "Fantasy Server " << FANTASY_SERVER_VERSION << "\n";
    std::cout << "state=STARTING\n";
    runtime.start();
    std::cout << "state=READY\n";
    std::cout << "world=" << runtime.world().info.name << "\n";
    std::cout << "tiles=" << runtime.indexedTileCount() << "\n";
    std::cout << "F04 world runtime slice active; networking is not enabled yet.\n";
    runtime.stop();
    std::cout << "state=STOPPED\n";
    return 0;
}

} // namespace fantasy

int main(int argc, char** argv) {
    try {
        if (argc > 1 && std::string_view{argv[1]} == "--smoke-test") {
            return fantasy::runSmokeTest();
        }
        return fantasy::runServer();
    } catch (const std::exception& error) {
        std::cerr << "Fantasy Server error: " << error.what() << '\n';
        return 1;
    }
}
