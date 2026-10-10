#include "MapStorage.hpp"

#include <algorithm>
#include <stdexcept>

namespace fantasy::studio::mapcore {
namespace {

std::size_t hashCombine(std::size_t seed, std::size_t value) noexcept {
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}

} // namespace

std::size_t MapStorage::ChunkKeyHash::operator()(const ChunkKey& key) const noexcept {
    std::size_t seed = std::hash<std::int32_t>{}(key.x);
    seed = hashCombine(seed, std::hash<std::int32_t>{}(key.y));
    seed = hashCombine(seed, std::hash<std::int16_t>{}(key.z));
    return seed;
}

std::int32_t MapStorage::floorDiv(std::int32_t value, std::int32_t divisor) noexcept {
    const std::int32_t quotient = value / divisor;
    const std::int32_t remainder = value % divisor;
    return (remainder != 0 && ((remainder < 0) != (divisor < 0))) ? quotient - 1 : quotient;
}

std::int32_t MapStorage::positiveMod(std::int32_t value, std::int32_t divisor) noexcept {
    const std::int32_t remainder = value % divisor;
    return remainder < 0 ? remainder + divisor : remainder;
}

MapStorage::ChunkKey MapStorage::chunkKeyFor(const Position& position) noexcept {
    return ChunkKey{
        floorDiv(position.x, ChunkSize),
        floorDiv(position.y, ChunkSize),
        position.z
    };
}

std::uint32_t MapStorage::localKeyFor(const Position& position) noexcept {
    const auto localX = static_cast<std::uint32_t>(positiveMod(position.x, ChunkSize));
    const auto localY = static_cast<std::uint32_t>(positiveMod(position.y, ChunkSize));
    return localY * static_cast<std::uint32_t>(ChunkSize) + localX;
}

Tile& MapStorage::ensureTile(const Position& position) {
    const auto chunkKey = chunkKeyFor(position);
    auto& chunk = chunks_[chunkKey];
    const auto localKey = localKeyFor(position);
    const auto [it, inserted] = chunk.tiles.try_emplace(localKey, Tile{});
    if (inserted) {
        it->second.position = position;
        ++tileCount_;
    }
    return it->second;
}

Tile& MapStorage::setTile(Tile tile) {
    const Position position = tile.position;
    const auto chunkKey = chunkKeyFor(position);
    auto& chunk = chunks_[chunkKey];
    const auto localKey = localKeyFor(position);
    const auto [it, inserted] = chunk.tiles.insert_or_assign(localKey, std::move(tile));
    if (inserted) ++tileCount_;
    return it->second;
}

bool MapStorage::eraseTile(const Position& position) {
    const auto chunkKey = chunkKeyFor(position);
    auto chunkIt = chunks_.find(chunkKey);
    if (chunkIt == chunks_.end()) return false;

    const auto erased = chunkIt->second.tiles.erase(localKeyFor(position));
    if (erased == 0) return false;

    --tileCount_;
    if (chunkIt->second.tiles.empty()) chunks_.erase(chunkIt);
    return true;
}

Tile* MapStorage::findTile(const Position& position) {
    const auto chunkIt = chunks_.find(chunkKeyFor(position));
    if (chunkIt == chunks_.end()) return nullptr;
    const auto tileIt = chunkIt->second.tiles.find(localKeyFor(position));
    return tileIt == chunkIt->second.tiles.end() ? nullptr : &tileIt->second;
}

const Tile* MapStorage::findTile(const Position& position) const {
    const auto chunkIt = chunks_.find(chunkKeyFor(position));
    if (chunkIt == chunks_.end()) return nullptr;
    const auto tileIt = chunkIt->second.tiles.find(localKeyFor(position));
    return tileIt == chunkIt->second.tiles.end() ? nullptr : &tileIt->second;
}

bool MapStorage::contains(const Position& position) const {
    return findTile(position) != nullptr;
}

void MapStorage::forEachTileInRect(
    std::int16_t floor,
    const Rect& rect,
    const std::function<void(Tile&)>& visitor) {

    const std::int32_t minChunkX = floorDiv(std::min(rect.minX, rect.maxX), ChunkSize);
    const std::int32_t maxChunkX = floorDiv(std::max(rect.minX, rect.maxX), ChunkSize);
    const std::int32_t minChunkY = floorDiv(std::min(rect.minY, rect.maxY), ChunkSize);
    const std::int32_t maxChunkY = floorDiv(std::max(rect.minY, rect.maxY), ChunkSize);

    const std::int32_t minX = std::min(rect.minX, rect.maxX);
    const std::int32_t maxX = std::max(rect.minX, rect.maxX);
    const std::int32_t minY = std::min(rect.minY, rect.maxY);
    const std::int32_t maxY = std::max(rect.minY, rect.maxY);

    for (std::int32_t chunkY = minChunkY; chunkY <= maxChunkY; ++chunkY) {
        for (std::int32_t chunkX = minChunkX; chunkX <= maxChunkX; ++chunkX) {
            auto chunkIt = chunks_.find(ChunkKey{chunkX, chunkY, floor});
            if (chunkIt == chunks_.end()) continue;
            for (auto& [_, tile] : chunkIt->second.tiles) {
                if (tile.position.x < minX || tile.position.x > maxX || tile.position.y < minY || tile.position.y > maxY) continue;
                visitor(tile);
            }
        }
    }
}

void MapStorage::forEachTileInRect(
    std::int16_t floor,
    const Rect& rect,
    const std::function<void(const Tile&)>& visitor) const {

    const std::int32_t minChunkX = floorDiv(std::min(rect.minX, rect.maxX), ChunkSize);
    const std::int32_t maxChunkX = floorDiv(std::max(rect.minX, rect.maxX), ChunkSize);
    const std::int32_t minChunkY = floorDiv(std::min(rect.minY, rect.maxY), ChunkSize);
    const std::int32_t maxChunkY = floorDiv(std::max(rect.minY, rect.maxY), ChunkSize);

    const std::int32_t minX = std::min(rect.minX, rect.maxX);
    const std::int32_t maxX = std::max(rect.minX, rect.maxX);
    const std::int32_t minY = std::min(rect.minY, rect.maxY);
    const std::int32_t maxY = std::max(rect.minY, rect.maxY);

    for (std::int32_t chunkY = minChunkY; chunkY <= maxChunkY; ++chunkY) {
        for (std::int32_t chunkX = minChunkX; chunkX <= maxChunkX; ++chunkX) {
            const auto chunkIt = chunks_.find(ChunkKey{chunkX, chunkY, floor});
            if (chunkIt == chunks_.end()) continue;
            for (const auto& [_, tile] : chunkIt->second.tiles) {
                if (tile.position.x < minX || tile.position.x > maxX || tile.position.y < minY || tile.position.y > maxY) continue;
                visitor(tile);
            }
        }
    }
}

} // namespace fantasy::studio::mapcore
