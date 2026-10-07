#include "MapQueryService.hpp"

namespace fantasy::studio::mapcore {

MapSummary MapQueryService::summarize(const MapDocument& document) const {
    return MapSummary{
        document.revision(),
        document.map().tileCount(),
        document.map().towns().size(),
        document.map().houses().size(),
        document.map().waypoints().size(),
        document.selection().size()
    };
}

std::optional<Tile> MapQueryService::getTile(const MapDocument& document, const Position& position) const {
    const Tile* tile = document.map().findTile(position);
    if (!tile) return std::nullopt;
    return *tile;
}

RegionSnapshot MapQueryService::getRegion(
    const MapDocument& document,
    std::int16_t floor,
    const MapStorage::Rect& rect,
    std::size_t limit) const {

    RegionSnapshot snapshot;
    snapshot.revision = document.revision();
    snapshot.floor = floor;
    snapshot.rect = rect;

    document.map().forEachTileInRect(floor, rect, [&](const Tile& tile) {
        if (snapshot.tiles.size() >= limit) {
            snapshot.truncated = true;
            return;
        }
        snapshot.tiles.push_back(tile);
    });

    return snapshot;
}

std::vector<ItemMatch> MapQueryService::findItemsByServerId(
    const MapDocument& document,
    std::uint32_t serverId,
    std::int16_t floor,
    const MapStorage::Rect& rect,
    std::size_t limit) const {

    std::vector<ItemMatch> matches;
    document.map().forEachTileInRect(floor, rect, [&](const Tile& tile) {
        if (matches.size() >= limit) return;

        if (tile.ground.has_value() && tile.ground->serverId == serverId) {
            matches.push_back(ItemMatch{tile.position, true, 0, tile.ground->serverId, tile.ground->clientId});
            if (matches.size() >= limit) return;
        }

        for (std::size_t index = 0; index < tile.items.size() && matches.size() < limit; ++index) {
            const Item& item = tile.items[index];
            if (item.serverId != serverId) continue;
            matches.push_back(ItemMatch{tile.position, false, index, item.serverId, item.clientId});
        }
    });

    return matches;
}

} // namespace fantasy::studio::mapcore
