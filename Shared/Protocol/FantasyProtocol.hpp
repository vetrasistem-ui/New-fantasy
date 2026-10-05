#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace fantasy::protocol {

using Bytes = std::vector<std::uint8_t>;

inline constexpr std::uint16_t kProtocolVersion = 1;
inline constexpr std::size_t kEnvelopeBytes = 16;
inline constexpr std::uint32_t kMaxPayloadBytes = 4u * 1024u * 1024u;

enum class MessageType : std::uint16_t {
    Hello = 1,
    HelloAck = 2,
    LoginDev = 16,
    LoginOk = 17,
    EnterWorld = 32,
    MapChunk = 33,
    MoveRequest = 34,
    EntityAdd = 48,
    EntityMove = 49,
    EntityRemove = 50,
    Error = 254,
    Disconnect = 255
};

enum class MoveDirection : std::uint8_t {
    North = 0,
    East = 1,
    South = 2,
    West = 3
};

struct Frame {
    std::uint16_t protocolVersion = kProtocolVersion;
    MessageType messageType = MessageType::Error;
    std::uint32_t sequence = 0;
    Bytes payload;
    bool operator==(const Frame&) const = default;
};

struct Hello {
    std::uint32_t clientBuild = 0;
    std::uint16_t requestedProtocol = kProtocolVersion;
    bool operator==(const Hello&) const = default;
};

struct HelloAck {
    std::uint32_t serverBuild = 0;
    std::uint16_t acceptedProtocol = kProtocolVersion;
    bool operator==(const HelloAck&) const = default;
};

struct LoginDev {
    std::string characterName;
    bool operator==(const LoginDev&) const = default;
};

struct LoginOk {
    std::uint64_t entityId = 0;
    bool operator==(const LoginOk&) const = default;
};

struct EnterWorld {
    std::uint64_t entityId = 0;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int16_t z = 0;
    bool operator==(const EnterWorld&) const = default;
};

struct MapChunk {
    std::string regionId;
    std::int32_t chunkX = 0;
    std::int32_t chunkY = 0;
    std::int16_t floor = 0;
    std::uint32_t revision = 0;
    Bytes payload;
    bool operator==(const MapChunk&) const = default;
};

struct MoveRequest {
    MoveDirection direction = MoveDirection::North;
    bool operator==(const MoveRequest&) const = default;
};

struct EntityAdd {
    std::uint64_t entityId = 0;
    std::string entityType;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int16_t z = 0;
    bool operator==(const EntityAdd&) const = default;
};

struct EntityMove {
    std::uint64_t entityId = 0;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int16_t z = 0;
    MoveDirection direction = MoveDirection::North;
    bool operator==(const EntityMove&) const = default;
};

struct EntityRemove {
    std::uint64_t entityId = 0;
    bool operator==(const EntityRemove&) const = default;
};

struct ErrorMessage {
    std::uint32_t code = 0;
    std::string message;
    bool operator==(const ErrorMessage&) const = default;
};

struct Disconnect {
    std::string reason;
    bool operator==(const Disconnect&) const = default;
};

bool isKnownMessageType(std::uint16_t rawType);
bool isValidMoveDirection(std::uint8_t rawDirection);

Bytes encodeFrame(const Frame& frame);
Frame decodeFrame(std::span<const std::uint8_t> bytes);

// Returns nullopt until at least the complete 16-byte envelope is available.
// Once the envelope is available, returns the exact total frame size expected.
std::optional<std::size_t> expectedFrameSize(std::span<const std::uint8_t> prefix);

Frame makeFrame(std::uint32_t sequence, const Hello& message);
Frame makeFrame(std::uint32_t sequence, const HelloAck& message);
Frame makeFrame(std::uint32_t sequence, const LoginDev& message);
Frame makeFrame(std::uint32_t sequence, const LoginOk& message);
Frame makeFrame(std::uint32_t sequence, const EnterWorld& message);
Frame makeFrame(std::uint32_t sequence, const MapChunk& message);
Frame makeFrame(std::uint32_t sequence, const MoveRequest& message);
Frame makeFrame(std::uint32_t sequence, const EntityAdd& message);
Frame makeFrame(std::uint32_t sequence, const EntityMove& message);
Frame makeFrame(std::uint32_t sequence, const EntityRemove& message);
Frame makeFrame(std::uint32_t sequence, const ErrorMessage& message);
Frame makeFrame(std::uint32_t sequence, const Disconnect& message);

Hello decodeHello(const Frame& frame);
HelloAck decodeHelloAck(const Frame& frame);
LoginDev decodeLoginDev(const Frame& frame);
LoginOk decodeLoginOk(const Frame& frame);
EnterWorld decodeEnterWorld(const Frame& frame);
MapChunk decodeMapChunk(const Frame& frame);
MoveRequest decodeMoveRequest(const Frame& frame);
EntityAdd decodeEntityAdd(const Frame& frame);
EntityMove decodeEntityMove(const Frame& frame);
EntityRemove decodeEntityRemove(const Frame& frame);
ErrorMessage decodeError(const Frame& frame);
Disconnect decodeDisconnect(const Frame& frame);

} // namespace fantasy::protocol
