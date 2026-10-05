#pragma once

#include "Shared/Formats/FMAP/FmapCore.hpp"
#include "Shared/Network/TcpTransport.hpp"
#include "Shared/Protocol/FantasyProtocol.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace fantasy::client {

struct ReceivedChunk {
    std::string regionId;
    std::uint32_t revision = 0;
    fmap::Chunk chunk;
};

class DevelopmentClient {
public:
    static DevelopmentClient connectIpv4(
        const std::string& host,
        std::uint16_t port,
        std::uint32_t clientBuild = 1);

    DevelopmentClient(const DevelopmentClient&) = delete;
    DevelopmentClient& operator=(const DevelopmentClient&) = delete;
    DevelopmentClient(DevelopmentClient&&) noexcept = default;
    DevelopmentClient& operator=(DevelopmentClient&&) noexcept = default;

    void handshake();
    void login(const std::string& characterName);
    protocol::EntityMove move(protocol::MoveDirection direction);
    void disconnect(const std::string& reason = "client exit");

    bool handshakeComplete() const { return handshakeComplete_; }
    bool inWorld() const { return inWorld_; }
    std::optional<std::uint64_t> entityId() const { return entityId_; }
    const fmap::Position& position() const { return position_; }
    const std::vector<ReceivedChunk>& chunks() const { return chunks_; }

private:
    DevelopmentClient(net::TcpStream stream, std::uint32_t clientBuild);
    std::uint32_t nextSequence();
    protocol::Frame receiveOrThrowError();

    net::TcpStream stream_;
    std::uint32_t clientBuild_ = 1;
    std::uint32_t nextClientSequence_ = 1;
    bool handshakeComplete_ = false;
    bool inWorld_ = false;
    std::optional<std::uint64_t> entityId_;
    fmap::Position position_{};
    std::vector<ReceivedChunk> chunks_;
};

} // namespace fantasy::client
