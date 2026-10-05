#include "Shared/Protocol/MapChunkPayload.hpp"

#include <array>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace fantasy::protocol {
namespace {

constexpr std::array<std::uint8_t, 4> kMagic{'F', 'M', 'C', 'P'};
constexpr std::uint32_t kMaxTilesPerChunkPayload = 65536;
constexpr std::uint32_t kMaxStringsPerTileList = 1024;
constexpr std::uint32_t kMaxStringBytes = 4096;

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
    if (offset + sizeof(T) > bytes.size()) throw std::runtime_error("MapChunk payload truncated");
    T value = 0;
    for (std::size_t index = 0; index < sizeof(T); ++index) {
        value |= static_cast<T>(bytes[offset + index]) << (index * 8u);
    }
    offset += sizeof(T);
    return value;
}

class Writer {
public:
    void u16(std::uint16_t value) { appendUnsigned(bytes_, value); }
    void u32(std::uint32_t value) { appendUnsigned(bytes_, value); }
    void i32(std::int32_t value) { u32(static_cast<std::uint32_t>(value)); }

    void string(const std::string& value) {
        if (value.size() > kMaxStringBytes) throw std::runtime_error("MapChunk string exceeds limit");
        u32(static_cast<std::uint32_t>(value.size()));
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }

    void strings(const std::vector<std::string>& values) {
        if (values.size() > kMaxStringsPerTileList) throw std::runtime_error("MapChunk string list exceeds limit");
        u32(static_cast<std::uint32_t>(values.size()));
        for (const auto& value : values) string(value);
    }

    Bytes finish() && { return std::move(bytes_); }

private:
    Bytes bytes_;
};

class Reader {
public:
    explicit Reader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}

    void requireMagic() {
        if (bytes_.size() < kMagic.size()) throw std::runtime_error("MapChunk payload truncated before magic");
        for (std::size_t index = 0; index < kMagic.size(); ++index) {
            if (bytes_[index] != kMagic[index]) throw std::runtime_error("MapChunk payload magic mismatch");
        }
        offset_ = kMagic.size();
    }

    std::uint16_t u16() { return readUnsigned<std::uint16_t>(bytes_, offset_); }
    std::uint32_t u32() { return readUnsigned<std::uint32_t>(bytes_, offset_); }
    std::int32_t i32() { return static_cast<std::int32_t>(u32()); }

    std::string string() {
        const auto size = u32();
        if (size > kMaxStringBytes) throw std::runtime_error("MapChunk string exceeds limit");
        if (offset_ + size > bytes_.size()) throw std::runtime_error("MapChunk string truncated");
        const auto* begin = reinterpret_cast<const char*>(bytes_.data() + offset_);
        std::string value(begin, begin + size);
        offset_ += size;
        return value;
    }

    std::vector<std::string> strings() {
        const auto count = u32();
        if (count > kMaxStringsPerTileList) throw std::runtime_error("MapChunk string list exceeds limit");
        std::vector<std::string> values;
        values.reserve(count);
        for (std::uint32_t index = 0; index < count; ++index) values.push_back(string());
        return values;
    }

    void requireEnd() const {
        if (offset_ != bytes_.size()) throw std::runtime_error("MapChunk payload has trailing bytes");
    }

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t offset_ = 0;
};

void validateTile(const fmap::Tile& tile) {
    if (!fmap::isSemanticAssetKey(tile.ground)) {
        throw std::runtime_error("MapChunk contains invalid semantic ground key: " + tile.ground);
    }
    for (const auto& object : tile.objects) {
        if (!fmap::isSemanticAssetKey(object)) {
            throw std::runtime_error("MapChunk contains invalid semantic object key: " + object);
        }
    }
    for (const auto& tag : tile.tags) {
        if (tag.empty()) throw std::runtime_error("MapChunk contains empty tile tag");
    }
}

} // namespace

Bytes encodeFmapChunkPayload(const fmap::Chunk& chunk) {
    if (chunk.tiles.size() > kMaxTilesPerChunkPayload) throw std::runtime_error("MapChunk tile count exceeds limit");

    Writer writer;
    Bytes prefix(kMagic.begin(), kMagic.end());
    writer.u16(kMapChunkPayloadVersion);
    writer.u32(static_cast<std::uint32_t>(chunk.tiles.size()));

    for (const auto& tile : chunk.tiles) {
        validateTile(tile);
        writer.i32(tile.x);
        writer.i32(tile.y);
        writer.string(tile.ground);
        writer.strings(tile.objects);
        writer.strings(tile.tags);
    }

    Bytes body = std::move(writer).finish();
    prefix.insert(prefix.end(), body.begin(), body.end());
    if (prefix.size() > kMaxPayloadBytes) throw std::runtime_error("MapChunk semantic payload exceeds protocol maximum");
    return prefix;
}

fmap::Chunk decodeFmapChunkPayload(const MapChunk& message) {
    Reader reader(message.payload);
    reader.requireMagic();
    const auto version = reader.u16();
    if (version != kMapChunkPayloadVersion) throw std::runtime_error("Unsupported MapChunk payload version");

    const auto tileCount = reader.u32();
    if (tileCount > kMaxTilesPerChunkPayload) throw std::runtime_error("MapChunk tile count exceeds limit");

    fmap::Chunk chunk;
    chunk.x = message.chunkX;
    chunk.y = message.chunkY;
    chunk.floor = message.floor;
    chunk.tiles.reserve(tileCount);

    for (std::uint32_t index = 0; index < tileCount; ++index) {
        fmap::Tile tile;
        tile.x = reader.i32();
        tile.y = reader.i32();
        tile.ground = reader.string();
        tile.objects = reader.strings();
        tile.tags = reader.strings();
        validateTile(tile);
        chunk.tiles.push_back(std::move(tile));
    }

    reader.requireEnd();
    return chunk;
}

MapChunk makeProtocolMapChunk(const fmap::Region& region, const fmap::Chunk& chunk, std::uint32_t revision) {
    if (region.id.empty()) throw std::runtime_error("MapChunk region id cannot be empty");
    return MapChunk{
        region.id,
        region.origin.x,
        region.origin.y,
        chunk.x,
        chunk.y,
        chunk.floor,
        revision,
        encodeFmapChunkPayload(chunk)
    };
}

} // namespace fantasy::protocol
