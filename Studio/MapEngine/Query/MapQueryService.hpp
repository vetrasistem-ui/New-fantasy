#pragma once

#include "../Core/MapDocument.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace fantasy::studio::mapcore {

struct MapSummary {
    std::uint64_t revision = 0;
    std::size_t tileCount = 0;
    std::size_t houseCount = 0;
    std::size_t waypointCount = 0;
    std::size_t selectedTileCount = 0;
};

struct RegionSnapshot {
    std::uint64_t revision = 0;
    std::int16_t floor = 7;
    MapStorage::Rect rect;
    std::vector<Tile> tiles;
    bool truncated = false;
};

struct ItemMatch {
    Position position;
    bool ground = false;
    std::size_t stackIndex = 0;
    std::uint32_t serverId = 0;
    std::uint32_t clientId = 0;
};

class MapQueryService {
public:
    [[nodiscard]] MapSummary summarize(const MapDocument& document) const;
    [[nodiscard]] std::optional<Tile> getTile(const MapDocument& document, const Position& position) const;

    [[nodiscard]] RegionSnapshot getRegion(
        const MapDocument& document,
        std::int16_t floor,
        const MapStorage::Rect& rect,
        std::size_t limit = 4096) const;

    [[nodiscard]] std::vector<ItemMatch> findItemsByServerId(
        const MapDocument& document,
        std::uint32_t serverId,
        std::int16_t floor,
        const MapStorage::Rect& rect,
        std::size_t limit = 1024) const;
};

} // namespace fantasy::studio::mapcore
