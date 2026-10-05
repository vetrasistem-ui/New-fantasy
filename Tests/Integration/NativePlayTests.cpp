#include "Client/Core/DevelopmentClient.hpp"
#include "Server/Core/WorldRuntime.hpp"
#include "Server/Network/TcpDevelopmentConnection.hpp"
#include "Shared/Network/FrameStream.hpp"
#include "Shared/Network/TcpTransport.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#ifndef FANTASY_REPO_ROOT
#error FANTASY_REPO_ROOT must be defined for NativePlayTests
#endif

namespace fs = std::filesystem;
namespace fp = fantasy::protocol;
namespace fc = fantasy::client;
namespace fsrv = fantasy::server;
namespace fnet = fantasy::server::network;
namespace net = fantasy::net;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void runRoundtrip(fsrv::WorldRuntime& runtime, const std::string& character, fp::MoveDirection direction) {
    auto listener = net::TcpListener::listenLoopback(0);
    const auto port = listener.localPort();
    require(port != 0, "ephemeral listener did not receive a port");

    fnet::ConnectionStats serverStats{};
    std::exception_ptr serverError;
    std::thread serverThread([&] {
        try {
            auto stream = listener.acceptOne();
            serverStats = fnet::serveDevelopmentConnection(std::move(stream), runtime);
        } catch (...) {
            serverError = std::current_exception();
        }
    });

    std::exception_ptr clientError;
    try {
        auto client = fc::DevelopmentClient::connectIpv4("127.0.0.1", port, 501);
        client.handshake();
        require(client.handshakeComplete(), "client handshake did not complete");

        client.login(character);
        require(client.inWorld(), "client did not enter world");
        require(client.entityId().has_value(), "client entity id missing");
        require(client.position() == fantasy::fmap::Position{100, 100, 7},
            "client initial authoritative position mismatch");
        require(client.chunks().size() == 4, "client must receive four development chunks");

        std::size_t tileCount = 0;
        bool spawnTileResolved = false;
        for (const auto& received : client.chunks()) {
            require(received.regionId == "development", "client received unexpected region");
            require(received.regionOriginX == 96 && received.regionOriginY == 96,
                "client received wrong FMAP region origin");
            require(received.revision == 1, "client received unexpected chunk revision");
            tileCount += received.chunk.tiles.size();

            for (const auto& tile : received.chunk.tiles) {
                const auto global = received.globalPosition(tile);
                if (global == fantasy::fmap::Position{100, 100, 7}) {
                    require(tile.ground == "terrain.grass.basic", "spawn tile semantic ground mismatch");
                    spawnTileResolved = true;
                }
            }
        }
        require(tileCount == 64, "client must reconstruct 64 semantic FMAP tiles");
        require(spawnTileResolved, "client could not resolve FMAP spawn tile to global coordinates");

        const auto moved = client.move(direction);
        if (direction == fp::MoveDirection::East) {
            require(moved.x == 101 && moved.y == 100 && moved.z == 7,
                "east move authoritative position mismatch");
        } else if (direction == fp::MoveDirection::West) {
            require(moved.x == 99 && moved.y == 100 && moved.z == 7,
                "west move authoritative position mismatch");
        }
        require(client.position() == fantasy::fmap::Position{moved.x, moved.y, moved.z},
            "client local state did not apply EntityMove");

        client.disconnect("integration roundtrip complete");
        require(!client.inWorld(), "client remained in-world after disconnect");
    } catch (...) {
        clientError = std::current_exception();
    }

    serverThread.join();
    listener.close();

    if (clientError) std::rethrow_exception(clientError);
    if (serverError) std::rethrow_exception(serverError);

    require(serverStats.cleanDisconnect, "server did not observe clean disconnect");
    require(serverStats.framesReceived == 4, "server should receive Hello/Login/Move/Disconnect");
    require(serverStats.framesSent == 10, "server reply frame count mismatch");
    require(runtime.entityCount() == 0, "server retained entity after TCP disconnect");
}

void runAbruptDisconnectCleanup(fsrv::WorldRuntime& runtime) {
    auto listener = net::TcpListener::listenLoopback(0);
    const auto port = listener.localPort();
    std::exception_ptr serverError;

    std::thread serverThread([&] {
        try {
            auto stream = listener.acceptOne();
            (void)fnet::serveDevelopmentConnection(std::move(stream), runtime);
        } catch (...) {
            serverError = std::current_exception();
        }
    });

    auto stream = net::TcpStream::connectIpv4("127.0.0.1", port);
    net::sendFrame(stream, fp::makeFrame(1, fp::Hello{900, fp::kProtocolVersion}));
    require(fp::decodeHelloAck(net::receiveFrame(stream)).acceptedProtocol == fp::kProtocolVersion,
        "abrupt-disconnect handshake failed");

    net::sendFrame(stream, fp::makeFrame(2, fp::LoginDev{"Abrupt Hero"}));
    for (int index = 0; index < 7; ++index) {
        (void)net::receiveFrame(stream);
    }

    stream.close();
    serverThread.join();
    listener.close();

    require(serverError != nullptr, "abrupt transport close should terminate the development connection");
    require(runtime.entityCount() == 0, "abrupt TCP disconnect leaked a server entity");
}

} // namespace

int main() {
    try {
        const fs::path repoRoot = fs::weakly_canonical(fs::path(FANTASY_REPO_ROOT));
        auto runtime = fsrv::WorldRuntime::load(repoRoot / "Game/Maps/World/world.fmap.json");
        runtime.start();

        runRoundtrip(runtime, "TCP Hero", fp::MoveDirection::East);
        runRoundtrip(runtime, "Reconnect Hero", fp::MoveDirection::West);
        runAbruptDisconnectCleanup(runtime);

        runtime.stop();
        require(runtime.state() == fsrv::RuntimeState::Stopped, "runtime did not stop after native play test");

        std::cout << "NativePlayTests PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "NativePlayTests FAIL: " << error.what() << '\n';
        return 1;
    }
}
