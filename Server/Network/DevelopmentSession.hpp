#pragma once

#include "Core/WorldRuntime.hpp"
#include "Shared/Protocol/FantasyProtocol.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace fantasy::server::network {

enum class SessionState {
    AwaitHello,
    AwaitLogin,
    InWorld,
    Closed
};

class DevelopmentSession {
public:
    explicit DevelopmentSession(WorldRuntime& world, std::uint32_t serverBuild = 1);

    SessionState state() const { return state_; }
    std::optional<std::uint64_t> entityId() const { return entityId_; }
    std::uint32_t lastClientSequence() const { return lastClientSequence_; }

    std::vector<protocol::Frame> handle(const protocol::Frame& incoming);
    void close();

private:
    protocol::Frame error(std::uint32_t code, std::string message);
    std::uint32_t nextSequence();
    bool acceptClientSequence(std::uint32_t sequence);
    std::vector<protocol::Frame> handleHello(const protocol::Frame& incoming);
    std::vector<protocol::Frame> handleLogin(const protocol::Frame& incoming);
    std::vector<protocol::Frame> handleInWorld(const protocol::Frame& incoming);

    WorldRuntime& world_;
    std::uint32_t serverBuild_ = 1;
    SessionState state_ = SessionState::AwaitHello;
    std::optional<std::uint64_t> entityId_;
    std::uint32_t lastClientSequence_ = 0;
    std::uint32_t nextServerSequence_ = 1;
};

} // namespace fantasy::server::network
