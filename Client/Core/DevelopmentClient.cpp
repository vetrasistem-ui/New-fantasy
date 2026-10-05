#include "Core/DevelopmentClient.hpp"

#include "Shared/Network/FrameStream.hpp"
#include "Shared/Protocol/MapChunkPayload.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace fantasy::client {

DevelopmentClient DevelopmentClient::connectIpv4(
    const std::string& host,
    std::uint16_t port,
    std::uint32_t clientBuild) {
    return DevelopmentClient(net::TcpStream::connectIpv4(host, port), clientBuild);
}

DevelopmentClient::DevelopmentClient(net::TcpStream stream, std::uint32_t clientBuild)
    : stream_(std::move(stream)), clientBuild_(clientBuild) {}

std::uint32_t DevelopmentClient::nextSequence() {
    return nextClientSequence_++;
}

protocol::Frame DevelopmentClient::receiveOrThrowError() {
    protocol::Frame frame = net::receiveFrame(stream_);
    if (frame.messageType == protocol::MessageType::Error) {
        const auto error = protocol::decodeError(frame);
        throw std::runtime_error("Fantasy Server error " + std::to_string(error.code) + ": " + error.message);
    }
    return frame;
}

void DevelopmentClient::handshake() {
    if (handshakeComplete_) throw std::runtime_error("Fantasy Client handshake already completed");
    net::sendFrame(stream_, protocol::makeFrame(nextSequence(), protocol::Hello{
        clientBuild_, protocol::kProtocolVersion
    }));

    const auto frame = receiveOrThrowError();
    if (frame.messageType != protocol::MessageType::HelloAck) {
        throw std::runtime_error("Fantasy Client expected HelloAck");
    }
    const auto ack = protocol::decodeHelloAck(frame);
    if (ack.acceptedProtocol != protocol::kProtocolVersion) {
        throw std::runtime_error("Fantasy Server accepted unexpected protocol version");
    }
    handshakeComplete_ = true;
}

void DevelopmentClient::login(const std::string& characterName) {
    if (!handshakeComplete_) throw std::runtime_error("Fantasy Client login requires handshake");
    if (inWorld_) throw std::runtime_error("Fantasy Client is already in world");

    chunks_.clear();
    entityId_.reset();
    position_ = {};

    net::sendFrame(stream_, protocol::makeFrame(nextSequence(), protocol::LoginDev{characterName}));

    bool sawLoginOk = false;
    bool sawEnterWorld = false;
    bool sawEntityAdd = false;
    std::optional<std::uint64_t> loginEntity;

    for (std::size_t frameCount = 0; frameCount < 4096 && !sawEntityAdd; ++frameCount) {
        const auto frame = receiveOrThrowError();
        switch (frame.messageType) {
            case protocol::MessageType::LoginOk: {
                if (sawLoginOk) throw std::runtime_error("Fantasy Client received duplicate LoginOk");
                const auto message = protocol::decodeLoginOk(frame);
                loginEntity = message.entityId;
                entityId_ = message.entityId;
                sawLoginOk = true;
                break;
            }
            case protocol::MessageType::EnterWorld: {
                if (!sawLoginOk) throw std::runtime_error("Fantasy Client received EnterWorld before LoginOk");
                if (sawEnterWorld) throw std::runtime_error("Fantasy Client received duplicate EnterWorld");
                const auto message = protocol::decodeEnterWorld(frame);
                if (!loginEntity.has_value() || message.entityId != *loginEntity) {
                    throw std::runtime_error("Fantasy Client EnterWorld entity mismatch");
                }
                position_ = fmap::Position{message.x, message.y, message.z};
                sawEnterWorld = true;
                break;
            }
            case protocol::MessageType::MapChunk: {
                if (!sawEnterWorld) throw std::runtime_error("Fantasy Client received MapChunk before EnterWorld");
                const auto message = protocol::decodeMapChunk(frame);
                chunks_.push_back(ReceivedChunk{
                    message.regionId,
                    message.regionOriginX,
                    message.regionOriginY,
                    message.revision,
                    protocol::decodeFmapChunkPayload(message)
                });
                break;
            }
            case protocol::MessageType::EntityAdd: {
                if (!sawEnterWorld) throw std::runtime_error("Fantasy Client received EntityAdd before EnterWorld");
                const auto message = protocol::decodeEntityAdd(frame);
                if (!loginEntity.has_value() || message.entityId != *loginEntity) {
                    throw std::runtime_error("Fantasy Client EntityAdd entity mismatch");
                }
                if (message.x != position_.x || message.y != position_.y || message.z != position_.z) {
                    throw std::runtime_error("Fantasy Client EntityAdd position mismatch");
                }
                sawEntityAdd = true;
                break;
            }
            default:
                throw std::runtime_error("Fantasy Client received unexpected message during login snapshot");
        }
    }

    if (!sawLoginOk || !sawEnterWorld || !sawEntityAdd) {
        throw std::runtime_error("Fantasy Client login snapshot did not complete");
    }
    if (chunks_.empty()) throw std::runtime_error("Fantasy Client entered world without MapChunk data");
    inWorld_ = true;
}

protocol::EntityMove DevelopmentClient::move(protocol::MoveDirection direction) {
    if (!inWorld_ || !entityId_.has_value()) throw std::runtime_error("Fantasy Client movement requires in-world state");

    net::sendFrame(stream_, protocol::makeFrame(nextSequence(), protocol::MoveRequest{direction}));
    const auto frame = receiveOrThrowError();
    if (frame.messageType != protocol::MessageType::EntityMove) {
        throw std::runtime_error("Fantasy Client expected authoritative EntityMove");
    }
    const auto message = protocol::decodeEntityMove(frame);
    if (message.entityId != *entityId_) throw std::runtime_error("Fantasy Client EntityMove entity mismatch");
    position_ = fmap::Position{message.x, message.y, message.z};
    return message;
}

void DevelopmentClient::disconnect(const std::string& reason) {
    if (!stream_.valid()) return;

    if (handshakeComplete_) {
        net::sendFrame(stream_, protocol::makeFrame(nextSequence(), protocol::Disconnect{reason}));
        const auto frame = receiveOrThrowError();
        if (frame.messageType != protocol::MessageType::Disconnect) {
            throw std::runtime_error("Fantasy Client expected Disconnect acknowledgement");
        }
        (void)protocol::decodeDisconnect(frame);
    }

    stream_.close();
    inWorld_ = false;
    entityId_.reset();
}

} // namespace fantasy::client
