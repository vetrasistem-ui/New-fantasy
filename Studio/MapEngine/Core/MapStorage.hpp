#pragma once

#include "MapTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>

namespace fantasy::studio::mapcore {

class MapStorage {
public:
    static constexpr std::int32_t ChunkSize = 64;

    struct Rect {
        std::int32_t minX = 0;
        std::int32_t minY = 0;
        std::int32_t maxX = 0;
        std::int32_t maxY = 0;
    };

    Tile& ensureTile(const Position& position);
    Tile& setTile(Tile tile);
    bool eraseTile(const Position& position);

    [[nodiscard]] Tile* findTile(const Position& position);
    [[nodiscard]] const Tile* findTile(const Position& position) const;
    [[nodiscard]] bool contains(const Position& position) const;
    [[nodiscard]] std::size_t tileCount() const noexcept { return tileCount_; }

    void forEachTileInRect(std::int16_t floor, const Rect& rect, const std::function<void(Tile&)>& visitor);
    void forEachTileInRect(std::int16_t floor, const Rect& rect, const std::function<void(const Tile&)>& visitor) const;

    std::unordered_map<std::uint32_t, Town>& towns() noexcept { return towns_; }
    const std::unordered_map<std::uint32_t, Town>& towns() const noexcept { return towns_; }

    std::unordered_map<std::uint32_t, House>& houses() noexcept { return houses_; }
    const std::unordered_map<std::uint32_t, House>& houses() const noexcept { return houses_; }

    std::unordered_map<std::string, Waypoint>& waypoints() noexcept { return waypoints_; }
    const std::unordered_map<std::string, Waypoint>& waypoints() const noexcept { return waypoints_; }

    std::vector<SpawnArea>& spawnAreas() noexcept { return spawnAreas_; }
    const std::vector<SpawnArea>& spawnAreas() const noexcept { return spawnAreas_; }

private:
    struct ChunkKey {
        std::int32_t x = 0;
        std::int32_t y = 0;
        std::int16_t z = 0;
        bool operator==(const ChunkKey&) const = default;
    };

    struct ChunkKeyHash {
        std::size_t operator()(const ChunkKey& key) const noexcept;
    };

    struct Chunk {
        std::unordered_map<std::uint32_t, Tile> tiles;
    };

    [[nodiscard]] static std::int32_t floorDiv(std::int32_t value, std::int32_t divisor) noexcept;
    [[nodiscard]] static std::int32_t positiveMod(std::int32_t value, std::int32_t divisor) noexcept;
    [[nodiscard]] static ChunkKey chunkKeyFor(const Position& position) noexcept;
    [[nodiscard]] static std::uint32_t localKeyFor(const Position& position) noexcept;

    std::unordered_map<ChunkKey, Chunk, ChunkKeyHash> chunks_;
    std::unordered_map<std::uint32_t, Town> towns_;
    std::unordered_map<std::uint32_t, House> houses_;
    std::unordered_map<std::string, Waypoint> waypoints_;
    std::vector<SpawnArea> spawnAreas_;
    std::size_t tileCount_ = 0;
};

} // namespace fantasy::studio::mapcore
