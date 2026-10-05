#include "Shared/Protocol/FantasyProtocol.hpp"

#include <array>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace fantasy::protocol {
namespace {

constexpr std::array<std::uint8_t, 4> kMagic{'F', 'N', 'T', 'Y'};

template <typename T>
void appendUnsigned(Bytes& out, T value) {
    static_assert(std::is_unsigned_v<T>);
    for (std::size_t index = 0; index < sizeof(T); ++index) {
        out.push_back(static_cast<std::uint8_t>((value >> (index * 8u)) & static_cast<T>(0xFFu)));
    }
}

template <typename T>
T readUnsigned(std::span<const std::uint8_t> bytes, std::size_t& offset) {
    static_assert(std::is_unsigned_v<T>);
    if (offset + sizeof(T) > bytes.size()) throw std::runtime_error("Fantasy Protocol payload truncated");
    T value = 0;
    for (std::size_t index = 0; index < sizeof(T); ++index) {
        value |= static_cast<T>(bytes[offset + index]) << (index * 8u);
    }
    offset += sizeof(T);
    return value;
}

class Writer {
public:
    void u8(std::uint8_t value) { bytes_.push_back(value); }
    void u16(std::uint16_t value) { appendUnsigned(bytes_, value); }
    void u32(std::uint32_t value) { appendUnsigned(bytes_, value); }
    void u64(std::uint64_t value) { appendUnsigned(bytes_, value); }
    void i16(std::int16_t value) { u16(static_cast<std::uint16_t>(value)); }
    void i32(std::int32_t value) { u32(static_cast<std::uint32_t>(value)); }

    void string(const std::string& value) {
        if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
            throw std::runtime_error("Fantasy Protocol string too large");
        }
        u32(static_cast<std::uint32_t>(value.size()));
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }

    void bytes(const Bytes& value) {
        if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
            throw std::runtime_error("Fantasy Protocol byte array too large");
        }
        u32(static_cast<std::uint32_t>(value.size()));
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }

    Bytes finish() && { return std::move(bytes_); }

private:
    Bytes bytes_;
};

class Reader {
public:
    explicit Reader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}

    std::uint8_t u8() {
        if (offset_ >= bytes_.size()) throw std::runtime_error("Fantasy Protocol payload truncated");
        return bytes_[offset_++];
    }
    std::uint16_t u16() { return readUnsigned<std::uint16_t>(bytes_, offset_); }
    std::uint32_t u32() { return readUnsigned<std::uint32_t>(bytes_, offset_); }
    std::uint64_t u64() { return readUnsigned<std::uint64_t>(bytes_, offset_); }
    std::int16_t i16() { return static_cast<std::int16_t>(u16()); }
    std::int32_t i32() { return static_cast<std::int32_t>(u32()); }

    std::string string() {
        const auto size = static_cast<std::size_t>(u32());
        if (offset_ + size > bytes_.size()) throw std::runtime_error("Fantasy Protocol string truncated");
        const auto* begin = reinterpret_cast<const char*>(bytes_.data() + offset_);
        std::string value(begin, begin + size);
        offset_ += size;
        return value;
    }

    Bytes bytes() {
        const auto size = static_cast<std::size_t>(u32());
        if (offset_ + size > bytes_.size()) throw std::runtime_error("Fantasy Protocol byte array truncated");
        Bytes value(bytes_.begin() + static_cast<std::ptrdiff_t>(offset_),
                    bytes_.begin() + static_cast<std::ptrdiff_t>(offset_ + size));
        offset_ += size;
        return value;
    }

    void requireEnd() const {
        if (offset_ != bytes_.size()) throw std::runtime_error("Fantasy Protocol payload has trailing bytes");
    }

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t offset_ = 0;
};

void requireType(const Frame& frame, MessageType expected) {
    if (frame.messageType != expected) throw std::runtime_error("Fantasy Protocol message type mismatch");
    if (frame.protocolVersion != kProtocolVersion) throw std::runtime_error("Unsupported Fantasy Protocol version");
}

Frame frame(std::uint32_t sequence, MessageType type, Bytes payload) {
    if (payload.size() > kMaxPayloadBytes) throw std::runtime_error("Fantasy Protocol payload exceeds maximum");
    return Frame{kProtocolVersion, type, sequence, std::move(payload)};
}

MoveDirection readDirection(Reader& reader) {
    const auto rawDirection = reader.u8();
    if (!isValidMoveDirection(rawDirection)) throw std::runtime_error("Invalid Fantasy Protocol move direction");
    return static_cast<MoveDirection>(rawDirection);
}

} // namespace

bool isKnownMessageType(std::uint16_t rawType) {
    switch (static_cast<MessageType>(rawType)) {
        case MessageType::Hello:
        case MessageType::HelloAck:
        case MessageType::LoginDev:
        case MessageType::LoginOk:
        case MessageType::EnterWorld:
        case MessageType::MapChunk:
        case MessageType::MoveRequest:
        case MessageType::EntityAdd:
        case MessageType::EntityMove:
        case MessageType::EntityRemove:
        case MessageType::Error:
        case MessageType::Disconnect:
            return true;
    }
    return false;
}

bool isValidMoveDirection(std::uint8_t rawDirection) {
    return rawDirection <= static_cast<std::uint8_t>(MoveDirection::West);
}

Bytes encodeFrame(const Frame& value) {
    if (value.protocolVersion != kProtocolVersion) throw std::runtime_error("Unsupported Fantasy Protocol version");
    if (!isKnownMessageType(static_cast<std::uint16_t>(value.messageType))) {
        throw std::runtime_error("Unknown Fantasy Protocol message type");
    }
    if (value.payload.size() > kMaxPayloadBytes) throw std::runtime_error("Fantasy Protocol payload exceeds maximum");

    Bytes bytes;
    bytes.reserve(kEnvelopeBytes + value.payload.size());
    bytes.insert(bytes.end(), kMagic.begin(), kMagic.end());
    appendUnsigned(bytes, value.protocolVersion);
    appendUnsigned(bytes, static_cast<std::uint16_t>(value.messageType));
    appendUnsigned(bytes, static_cast<std::uint32_t>(value.payload.size()));
    appendUnsigned(bytes, value.sequence);
    bytes.insert(bytes.end(), value.payload.begin(), value.payload.end());
    return bytes;
}

std::optional<std::size_t> expectedFrameSize(std::span<const std::uint8_t> prefix) {
    if (prefix.size() < kEnvelopeBytes) return std::nullopt;
    for (std::size_t index = 0; index < kMagic.size(); ++index) {
        if (prefix[index] != kMagic[index]) throw std::runtime_error("Fantasy Protocol magic mismatch");
    }

    std::size_t offset = 4;
    const auto version = readUnsigned<std::uint16_t>(prefix, offset);
    if (version != kProtocolVersion) throw std::runtime_error("Unsupported Fantasy Protocol version");
    const auto rawType = readUnsigned<std::uint16_t>(prefix, offset);
    if (!isKnownMessageType(rawType)) throw std::runtime_error("Unknown Fantasy Protocol message type");
    const auto payloadLength = readUnsigned<std::uint32_t>(prefix, offset);
    if (payloadLength > kMaxPayloadBytes) throw std::runtime_error("Fantasy Protocol payload exceeds maximum");
    (void)readUnsigned<std::uint32_t>(prefix, offset);
    return kEnvelopeBytes + static_cast<std::size_t>(payloadLength);
}

Frame decodeFrame(std::span<const std::uint8_t> bytes) {
    const auto expected = expectedFrameSize(bytes);
    if (!expected.has_value()) throw std::runtime_error("Fantasy Protocol envelope truncated");
    if (bytes.size() != *expected) throw std::runtime_error("Fantasy Protocol frame length mismatch");

    std::size_t offset = 4;
    const auto version = readUnsigned<std::uint16_t>(bytes, offset);
    const auto rawType = readUnsigned<std::uint16_t>(bytes, offset);
    const auto payloadLength = readUnsigned<std::uint32_t>(bytes, offset);
    const auto sequence = readUnsigned<std::uint32_t>(bytes, offset);

    Bytes payload(bytes.begin() + static_cast<std::ptrdiff_t>(kEnvelopeBytes), bytes.end());
    if (payload.size() != payloadLength) throw std::runtime_error("Fantasy Protocol payload length mismatch");
    return Frame{version, static_cast<MessageType>(rawType), sequence, std::move(payload)};
}

Frame makeFrame(std::uint32_t sequence, const Hello& message) {
    Writer writer;
    writer.u32(message.clientBuild);
    writer.u16(message.requestedProtocol);
    return frame(sequence, MessageType::Hello, std::move(writer).finish());
}

Frame makeFrame(std::uint32_t sequence, const HelloAck& message) {
    Writer writer;
    writer.u32(message.serverBuild);
    writer.u16(message.acceptedProtocol);
    return frame(sequence, MessageType::HelloAck, std::move(writer).finish());
}

Frame makeFrame(std::uint32_t sequence, const LoginDev& message) {
    Writer writer;
    writer.string(message.characterName);
    return frame(sequence, MessageType::LoginDev, std::move(writer).finish());
}

Frame makeFrame(std::uint32_t sequence, const LoginOk& message) {
    Writer writer;
    writer.u64(message.entityId);
    return frame(sequence, MessageType::LoginOk, std::move(writer).finish());
}

Frame makeFrame(std::uint32_t sequence, const EnterWorld& message) {
    Writer writer;
    writer.u64(message.entityId);
    writer.i32(message.x);
    writer.i32(message.y);
    writer.i16(message.z);
    return frame(sequence, MessageType::EnterWorld, std::move(writer).finish());
}

Frame makeFrame(std::uint32_t sequence, const MapChunk& message) {
    Writer writer;
    writer.string(message.regionId);
    writer.i32(message.regionOriginX);
    writer.i32(message.regionOriginY);
    writer.i32(message.chunkX);
    writer.i32(message.chunkY);
    writer.i16(message.floor);
    writer.u32(message.revision);
    writer.bytes(message.payload);
    return frame(sequence, MessageType::MapChunk, std::move(writer).finish());
}

Frame makeFrame(std::uint32_t sequence, const MoveRequest& message) {
    Writer writer;
    writer.u8(static_cast<std::uint8_t>(message.direction));
    return frame(sequence, MessageType::MoveRequest, std::move(writer).finish());
}

Frame makeFrame(std::uint32_t sequence, const EntityAdd& message) {
    Writer writer;
    writer.u64(message.entityId);
    writer.string(message.entityType);
    writer.i32(message.x);
    writer.i32(message.y);
    writer.i16(message.z);
    return frame(sequence, MessageType::EntityAdd, std::move(writer).finish());
}

Frame makeFrame(std::uint32_t sequence, const EntityMove& message) {
    Writer writer;
    writer.u64(message.entityId);
    writer.i32(message.x);
    writer.i32(message.y);
    writer.i16(message.z);
    writer.u8(static_cast<std::uint8_t>(message.direction));
    return frame(sequence, MessageType::EntityMove, std::move(writer).finish());
}

Frame makeFrame(std::uint32_t sequence, const EntityRemove& message) {
    Writer writer;
    writer.u64(message.entityId);
    return frame(sequence, MessageType::EntityRemove, std::move(writer).finish());
}

Frame makeFrame(std::uint32_t sequence, const ErrorMessage& message) {
    Writer writer;
    writer.u32(message.code);
    writer.string(message.message);
    return frame(sequence, MessageType::Error, std::move(writer).finish());
}

Frame makeFrame(std::uint32_t sequence, const Disconnect& message) {
    Writer writer;
    writer.string(message.reason);
    return frame(sequence, MessageType::Disconnect, std::move(writer).finish());
}

Hello decodeHello(const Frame& value) {
    requireType(value, MessageType::Hello);
    Reader reader(value.payload);
    Hello message{reader.u32(), reader.u16()};
    reader.requireEnd();
    return message;
}

HelloAck decodeHelloAck(const Frame& value) {
    requireType(value, MessageType::HelloAck);
    Reader reader(value.payload);
    HelloAck message{reader.u32(), reader.u16()};
    reader.requireEnd();
    return message;
}

LoginDev decodeLoginDev(const Frame& value) {
    requireType(value, MessageType::LoginDev);
    Reader reader(value.payload);
    LoginDev message{reader.string()};
    reader.requireEnd();
    return message;
}

LoginOk decodeLoginOk(const Frame& value) {
    requireType(value, MessageType::LoginOk);
    Reader reader(value.payload);
    LoginOk message{reader.u64()};
    reader.requireEnd();
    return message;
}

EnterWorld decodeEnterWorld(const Frame& value) {
    requireType(value, MessageType::EnterWorld);
    Reader reader(value.payload);
    EnterWorld message{reader.u64(), reader.i32(), reader.i32(), reader.i16()};
    reader.requireEnd();
    return message;
}

MapChunk decodeMapChunk(const Frame& value) {
    requireType(value, MessageType::MapChunk);
    Reader reader(value.payload);
    MapChunk message;
    message.regionId = reader.string();
    message.regionOriginX = reader.i32();
    message.regionOriginY = reader.i32();
    message.chunkX = reader.i32();
    message.chunkY = reader.i32();
    message.floor = reader.i16();
    message.revision = reader.u32();
    message.payload = reader.bytes();
    reader.requireEnd();
    return message;
}

MoveRequest decodeMoveRequest(const Frame& value) {
    requireType(value, MessageType::MoveRequest);
    Reader reader(value.payload);
    MoveRequest message{readDirection(reader)};
    reader.requireEnd();
    return message;
}

EntityAdd decodeEntityAdd(const Frame& value) {
    requireType(value, MessageType::EntityAdd);
    Reader reader(value.payload);
    EntityAdd message;
    message.entityId = reader.u64();
    message.entityType = reader.string();
    message.x = reader.i32();
    message.y = reader.i32();
    message.z = reader.i16();
    reader.requireEnd();
    return message;
}

EntityMove decodeEntityMove(const Frame& value) {
    requireType(value, MessageType::EntityMove);
    Reader reader(value.payload);
    EntityMove message;
    message.entityId = reader.u64();
    message.x = reader.i32();
    message.y = reader.i32();
    message.z = reader.i16();
    message.direction = readDirection(reader);
    reader.requireEnd();
    return message;
}

EntityRemove decodeEntityRemove(const Frame& value) {
    requireType(value, MessageType::EntityRemove);
    Reader reader(value.payload);
    EntityRemove message{reader.u64()};
    reader.requireEnd();
    return message;
}

ErrorMessage decodeError(const Frame& value) {
    requireType(value, MessageType::Error);
    Reader reader(value.payload);
    ErrorMessage message{reader.u32(), reader.string()};
    reader.requireEnd();
    return message;
}

Disconnect decodeDisconnect(const Frame& value) {
    requireType(value, MessageType::Disconnect);
    Reader reader(value.payload);
    Disconnect message{reader.string()};
    reader.requireEnd();
    return message;
}

} // namespace fantasy::protocol
