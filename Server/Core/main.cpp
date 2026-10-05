#include "Core/WorldRuntime.hpp"
#include "Network/TcpDevelopmentConnection.hpp"
#include "Shared/Network/TcpTransport.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#ifndef FANTASY_SERVER_VERSION
#define FANTASY_SERVER_VERSION "dev"
#endif

#ifndef FANTASY_REPO_ROOT
#define FANTASY_REPO_ROOT "."
#endif

namespace fs = std::filesystem;

namespace fantasy {

std::uint16_t parsePort(const std::string& value) {
    const int parsed = std::stoi(value);
    if (parsed < 1 || parsed > 65535) throw std::runtime_error("port must be between 1 and 65535");
    return static_cast<std::uint16_t>(parsed);
}

namespace {

fs::path developmentMapPath() {
    const fs::path repoRoot = fs::weakly_canonical(fs::path(FANTASY_REPO_ROOT));
    return repoRoot / "Game/Maps/World/world.fmap.json";
}

} // namespace

int runSmokeTest() {
    server::WorldRuntime runtime = server::WorldRuntime::load(developmentMapPath());

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

int runServeOnce(std::uint16_t port) {
    server::WorldRuntime runtime = server::WorldRuntime::load(developmentMapPath());
    runtime.start();

    auto listener = net::TcpListener::listenLoopback(port);
    std::cout << "Fantasy Server " << FANTASY_SERVER_VERSION << "\n";
    std::cout << "state=READY\n";
    std::cout << "world=" << runtime.world().info.id << "\n";
    std::cout << "listen=127.0.0.1:" << listener.localPort() << "\n";
    std::cout << "mode=serve-once\n";

    auto stream = listener.acceptOne();
    const auto stats = server::network::serveDevelopmentConnection(std::move(stream), runtime);
    listener.close();
    runtime.stop();

    std::cout << "frames_received=" << stats.framesReceived << "\n";
    std::cout << "frames_sent=" << stats.framesSent << "\n";
    std::cout << "clean_disconnect=" << (stats.cleanDisconnect ? "true" : "false") << "\n";
    std::cout << "state=STOPPED\n";
    return stats.cleanDisconnect ? 0 : 1;
}

int runServer() {
    server::WorldRuntime runtime = server::WorldRuntime::load(developmentMapPath());

    std::cout << "Fantasy Server " << FANTASY_SERVER_VERSION << "\n";
    std::cout << "state=STARTING\n";
    runtime.start();
    std::cout << "state=READY\n";
    std::cout << "world=" << runtime.world().info.name << "\n";
    std::cout << "tiles=" << runtime.indexedTileCount() << "\n";
    std::cout << "F05 native TCP path available with --serve-once <port>.\n";
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
        if (argc > 1 && std::string_view{argv[1]} == "--serve-once") {
            if (argc != 3) throw std::runtime_error("usage: fantasy-server --serve-once <port>");
            return fantasy::runServeOnce(fantasy::parsePort(argv[2]));
        }
        return fantasy::runServer();
    } catch (const std::exception& error) {
        std::cerr << "Fantasy Server error: " << error.what() << '\n';
        return 1;
    }
}
