#include "Network/DevelopmentSession.hpp"
#include "Shared/Protocol/MapChunkPayload.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#ifndef FANTASY_REPO_ROOT
#error FANTASY_REPO_ROOT must be defined for DevelopmentSessionTests
#endif

namespace fs = std::filesystem;
using namespace fantasy::server;
using namespace fantasy::server::network;
namespace fp = fantasy::protocol;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

} // namespace

int main() {
    try {
        const fs::path repoRoot = fs::weakly_canonical(fs::path(FANTASY_REPO_ROOT));
        WorldRuntime runtime = WorldRuntime::load(repoRoot / "Game/Maps/World/world.fmap.json");
        runtime.start();
        DevelopmentSession session(runtime, 101);

        const auto helloReplies = session.handle(fp::makeFrame(1, fp::Hello{77, fp::kProtocolVersion}));
        require(helloReplies.size() == 1, "Hello should produce one reply");
        require(fp::decodeHelloAck(helloReplies.at(0)) == fp::HelloAck{101, fp::kProtocolVersion},
            "HelloAck mismatch");
        require(session.state() == SessionState::AwaitLogin, "session should await login after Hello");

        const auto loginReplies = session.handle(fp::makeFrame(2, fp::LoginDev{"Development Hero"}));
        require(loginReplies.size() == 7,
            "LoginDev should produce LoginOk + EnterWorld + 4 MapChunk + EntityAdd");
        const auto loginOk = fp::decodeLoginOk(loginReplies.at(0));
        const auto enterWorld = fp::decodeEnterWorld(loginReplies.at(1));
        const auto entityAdd = fp::decodeEntityAdd(loginReplies.at(6));
        require(loginOk.entityId == enterWorld.entityId && loginOk.entityId == entityAdd.entityId,
            "login replies must reference the same entity");
        require(enterWorld.x == 100 && enterWorld.y == 100 && enterWorld.z == 7,
            "EnterWorld must use FMAP development spawn");

        const std::int32_t expectedChunkX[4]{0, 4, 0, 4};
        const std::int32_t expectedChunkY[4]{0, 0, 4, 4};
        std::size_t streamedTiles = 0;
        for (std::size_t index = 0; index < 4; ++index) {
            const auto message = fp::decodeMapChunk(loginReplies.at(index + 2));
            require(message.regionId == "development", "MapChunk region mismatch");
            require(message.chunkX == expectedChunkX[index] && message.chunkY == expectedChunkY[index],
                "MapChunk deterministic ordering mismatch");
            require(message.floor == 7, "MapChunk floor mismatch");
            require(message.revision == 1, "MapChunk revision mismatch");
            const auto chunk = fp::decodeFmapChunkPayload(message);
            streamedTiles += chunk.tiles.size();
        }
        require(streamedTiles == 64, "login snapshot must stream all 64 development tiles");

        require(session.state() == SessionState::InWorld, "session should enter world after login");
        require(runtime.entityCount() == 1, "runtime should contain logged-in entity");

        const auto moveReplies = session.handle(fp::makeFrame(3, fp::MoveRequest{fp::MoveDirection::East}));
        require(moveReplies.size() == 1, "MoveRequest should produce one authoritative reply");
        const auto moved = fp::decodeEntityMove(moveReplies.at(0));
        require(moved.entityId == loginOk.entityId, "EntityMove id mismatch");
        require(moved.x == 101 && moved.y == 100 && moved.z == 7,
            "EntityMove must publish server-authoritative position");
        require(runtime.entity(loginOk.entityId)->position.x == 101,
            "WorldRuntime did not apply requested movement");

        const auto replayReplies = session.handle(fp::makeFrame(3, fp::MoveRequest{fp::MoveDirection::East}));
        require(replayReplies.size() == 1, "replayed sequence should produce protocol error");
        require(fp::decodeError(replayReplies.at(0)).code == 1003,
            "replayed client sequence should be rejected");
        require(runtime.entity(loginOk.entityId)->position.x == 101,
            "replayed request must not move entity twice");

        const auto disconnectReplies = session.handle(fp::makeFrame(4, fp::Disconnect{"client exit"}));
        require(disconnectReplies.size() == 1, "Disconnect should produce acknowledgement");
        require(fp::decodeDisconnect(disconnectReplies.at(0)).reason == "session closed",
            "Disconnect acknowledgement mismatch");
        require(session.state() == SessionState::Closed, "session should be closed");
        require(runtime.entityCount() == 0, "session close must remove world entity");

        runtime.stop();

        WorldRuntime mismatchRuntime = WorldRuntime::load(repoRoot / "Game/Maps/World/world.fmap.json");
        mismatchRuntime.start();
        DevelopmentSession mismatchSession(mismatchRuntime, 101);
        const auto mismatchReplies = mismatchSession.handle(fp::makeFrame(1, fp::Hello{77, 999}));
        require(fp::decodeError(mismatchReplies.at(0)).code == 1001,
            "unsupported requested protocol should be rejected");
        require(mismatchSession.state() == SessionState::Closed,
            "protocol mismatch should close session");
        mismatchRuntime.stop();

        std::cout << "DevelopmentSessionTests PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "DevelopmentSessionTests FAIL: " << error.what() << '\n';
        return 1;
    }
}
