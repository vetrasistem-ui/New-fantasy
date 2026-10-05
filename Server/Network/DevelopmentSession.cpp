#include "Network/DevelopmentSession.hpp"
#include "Network/WorldSnapshot.hpp"

#include <cctype>
#include <stdexcept>
#include <utility>

namespace fantasy::server::network {
namespace {

bool validCharacterName(const std::string& value) {
    if (value.empty() || value.size() > 48) return false;
    for (const unsigned char ch : value) {
        if (!std::isspace(ch)) return true;
    }
    return false;
}

std::pair<std::int32_t, std::int32_t> movementDelta(protocol::MoveDirection direction) {
    switch (direction) {
        case protocol::MoveDirection::North: return {0, -1};
        case protocol::MoveDirection::East: return {1, 0};
        case protocol::MoveDirection::South: return {0, 1};
        case protocol::MoveDirection::West: return {-1, 0};
    }
    throw std::runtime_error("Unsupported movement direction");
}

} // namespace

DevelopmentSession::DevelopmentSession(WorldRuntime& world, std::uint32_t serverBuild)
    : world_(world), serverBuild_(serverBuild) {
    if (world_.state() != RuntimeState::Ready) {
        throw std::runtime_error("DevelopmentSession requires a Ready WorldRuntime");
    }
}

std::uint32_t DevelopmentSession::nextSequence() {
    return nextServerSequence_++;
}

bool DevelopmentSession::acceptClientSequence(std::uint32_t sequence) {
    if (sequence == 0 || sequence <= lastClientSequence_) return false;
    lastClientSequence_ = sequence;
    return true;
}

protocol::Frame DevelopmentSession::error(std::uint32_t code, std::string message) {
    return protocol::makeFrame(nextSequence(), protocol::ErrorMessage{code, std::move(message)});
}

std::vector<protocol::Frame> DevelopmentSession::handle(const protocol::Frame& incoming) {
    if (state_ == SessionState::Closed) return {};
    if (incoming.protocolVersion != protocol::kProtocolVersion) {
        state_ = SessionState::Closed;
        return {error(1001, "unsupported protocol version")};
    }
    if (!acceptClientSequence(incoming.sequence)) {
        return {error(1003, "client sequence must increase monotonically")};
    }

    switch (state_) {
        case SessionState::AwaitHello: return handleHello(incoming);
        case SessionState::AwaitLogin: return handleLogin(incoming);
        case SessionState::InWorld: return handleInWorld(incoming);
        case SessionState::Closed: return {};
    }
    return {error(1099, "invalid session state")};
}

std::vector<protocol::Frame> DevelopmentSession::handleHello(const protocol::Frame& incoming) {
    if (incoming.messageType != protocol::MessageType::Hello) {
        return {error(1002, "Hello required before any other message")};
    }

    const auto hello = protocol::decodeHello(incoming);
    if (hello.requestedProtocol != protocol::kProtocolVersion) {
        state_ = SessionState::Closed;
        return {error(1001, "requested protocol is not supported")};
    }

    state_ = SessionState::AwaitLogin;
    return {protocol::makeFrame(nextSequence(), protocol::HelloAck{serverBuild_, protocol::kProtocolVersion})};
}

std::vector<protocol::Frame> DevelopmentSession::handleLogin(const protocol::Frame& incoming) {
    if (incoming.messageType != protocol::MessageType::LoginDev) {
        return {error(1100, "LoginDev required after handshake")};
    }

    const auto login = protocol::decodeLoginDev(incoming);
    if (!validCharacterName(login.characterName)) {
        return {error(1101, "invalid development character name")};
    }

    try {
        const auto id = world_.spawnEntity(login.characterName, world_.world().developmentSpawn);
        entityId_ = id;
        state_ = SessionState::InWorld;
        const auto& position = world_.world().developmentSpawn;

        std::vector<protocol::Frame> replies;
        replies.push_back(protocol::makeFrame(nextSequence(), protocol::LoginOk{id}));
        replies.push_back(protocol::makeFrame(nextSequence(), protocol::EnterWorld{
            id, position.x, position.y, position.z
        }));

        for (const auto& chunk : makeInitialMapSnapshot(world_.world(), position)) {
            replies.push_back(protocol::makeFrame(nextSequence(), chunk));
        }

        replies.push_back(protocol::makeFrame(nextSequence(), protocol::EntityAdd{
            id, "player", position.x, position.y, position.z
        }));
        return replies;
    } catch (const std::exception& exception) {
        if (entityId_.has_value()) {
            world_.removeEntity(*entityId_);
            entityId_.reset();
        }
        state_ = SessionState::AwaitLogin;
        return {error(1102, std::string("development login failed: ") + exception.what())};
    }
}

std::vector<protocol::Frame> DevelopmentSession::handleInWorld(const protocol::Frame& incoming) {
    if (incoming.messageType == protocol::MessageType::Disconnect) {
        (void)protocol::decodeDisconnect(incoming);
        close();
        return {protocol::makeFrame(nextSequence(), protocol::Disconnect{"session closed"})};
    }

    if (incoming.messageType != protocol::MessageType::MoveRequest) {
        return {error(1200, "message is not valid while in world")};
    }
    if (!entityId_.has_value()) return {error(1201, "session has no entity")};

    const auto request = protocol::decodeMoveRequest(incoming);
    const auto [dx, dy] = movementDelta(request.direction);
    if (!world_.moveEntity(*entityId_, dx, dy)) {
        return {error(2001, "movement rejected by authoritative world runtime")};
    }

    const auto* entity = world_.entity(*entityId_);
    if (entity == nullptr) return {error(1202, "session entity disappeared")};

    return {protocol::makeFrame(nextSequence(), protocol::EntityMove{
        entity->id,
        entity->position.x,
        entity->position.y,
        entity->position.z,
        request.direction
    })};
}

void DevelopmentSession::close() {
    if (entityId_.has_value()) {
        world_.removeEntity(*entityId_);
        entityId_.reset();
    }
    state_ = SessionState::Closed;
}

} // namespace fantasy::server::network
