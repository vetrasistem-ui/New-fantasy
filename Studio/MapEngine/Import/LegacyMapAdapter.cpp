#include "LegacyMapAdapter.hpp"

#include <algorithm>
#include <charconv>
#include <set>
#include <string_view>
#include <utility>

namespace fantasy::studio::mapcore {
namespace {

Position toPosition(const fantasy::legacy::LegacyPosition& position) {
    return Position{position.x, position.y, position.z};
}

bool parseInteger(std::string_view text, std::int64_t& value) {
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    return ec == std::errc{} && ptr == end;
}

bool parsePosition(std::string_view text, Position& value) {
    const auto first = text.find(',');
    if (first == std::string_view::npos) return false;
    const auto second = text.find(',', first + 1);
    if (second == std::string_view::npos) return false;

    std::int64_t x = 0;
    std::int64_t y = 0;
    std::int64_t z = 0;
    if (!parseInteger(text.substr(0, first), x)) return false;
    if (!parseInteger(text.substr(first + 1, second - first - 1), y)) return false;
    if (!parseInteger(text.substr(second + 1), z)) return false;

    value = Position{
        static_cast<std::int32_t>(x),
        static_cast<std::int32_t>(y),
        static_cast<std::int16_t>(z)
    };
    return true;
}

bool isIntegerAttribute(const std::string& key) {
    return key == "count" || key == "runeCharges" || key == "actionId" ||
        key == "uniqueId" || key == "depotId" || key == "houseDoorId" ||
        key == "duration" || key == "decayingState" || key == "writtenDate" ||
        key == "sleeperGuid" || key == "sleepStart" || key == "charges" ||
        key == "tileFlags";
}

AttributeValue toAttributeValue(const std::string& key, const std::string& value) {
    if (key == "teleportDestination") {
        Position position;
        if (parsePosition(value, position)) return position;
    }

    if (isIntegerAttribute(key)) {
        std::int64_t number = 0;
        if (parseInteger(value, number)) return number;
    }

    return value;
}

Item convertItem(
    const fantasy::legacy::LegacyImportedItem& source,
    const fantasy::assets::FantasyAssetRegistry& assets,
    std::set<std::uint32_t>& unresolved) {

    Item item;
    item.serverId = source.serverId;

    if (const auto* record = assets.findByLegacyServerId(source.serverId)) {
        item.clientId = record->clientId;
    } else if (source.serverId != 0) {
        unresolved.insert(source.serverId);
    }

    for (const auto& [key, value] : source.attributes) {
        item.attributes[key] = toAttributeValue(key, value);
    }

    for (const char* subtypeKey : {"count", "runeCharges", "charges"}) {
        const auto it = source.attributes.find(subtypeKey);
        if (it == source.attributes.end()) continue;
        std::int64_t subtype = 0;
        if (parseInteger(it->second, subtype) && subtype >= 0 && subtype <= 65535) {
            item.countOrSubtype = static_cast<std::uint16_t>(subtype);
        }
        break;
    }

    item.contents.reserve(source.contents.size());
    for (const auto& child : source.contents) {
        item.contents.push_back(convertItem(child, assets, unresolved));
    }

    return item;
}

bool isGround(
    std::uint32_t serverId,
    const fantasy::assets::FantasyAssetRegistry& assets) {

    const auto* record = assets.findByLegacyServerId(serverId);
    return record && record->kind == fantasy::assets::LegacyAssetKind::Ground;
}

SpawnEntryKind toSpawnEntryKind(fantasy::legacy::LegacySpawnEntryKind kind) {
    switch (kind) {
    case fantasy::legacy::LegacySpawnEntryKind::Npc:
        return SpawnEntryKind::Npc;
    case fantasy::legacy::LegacySpawnEntryKind::MonsterSet:
        return SpawnEntryKind::MonsterSet;
    case fantasy::legacy::LegacySpawnEntryKind::Monster:
    default:
        return SpawnEntryKind::Monster;
    }
}

} // namespace

LegacyMapAdaptResult LegacyMapAdapter::adapt(
    const fantasy::legacy::OtbmReadResult& source,
    const fantasy::assets::FantasyAssetRegistry& assets) const {

    LegacyMapAdaptResult result;
    std::set<std::uint32_t> unresolved;

    result.metadata.width = source.import.model.width;
    result.metadata.height = source.import.model.height;
    result.metadata.name = source.import.model.sourceMapName;
    result.metadata.description = source.metadata.description;
    result.metadata.spawnFile = source.metadata.spawnFile;
    result.metadata.houseFile = source.metadata.houseFile;
    result.metadata.sourceProfileId = source.import.model.sourceProfileId;
    result.report.itemCount = source.import.diagnostics.itemCount;

    for (const auto& sourceTile : source.import.model.tiles) {
        Tile tile;
        tile.position = toPosition(sourceTile.position);
        tile.houseId = sourceTile.houseId.value_or(0);

        for (const auto& [key, value] : sourceTile.attributes) {
            tile.attributes[key] = toAttributeValue(key, value);
        }

        if (sourceTile.groundServerId.has_value()) {
            fantasy::legacy::LegacyImportedItem groundSource;
            groundSource.serverId = *sourceTile.groundServerId;
            tile.ground = convertItem(groundSource, assets, unresolved);
        }

        for (const auto& sourceItem : sourceTile.items) {
            if (sourceItem.serverId == 0) {
                const auto flagsIt = sourceItem.attributes.find("tileFlags");
                if (flagsIt != sourceItem.attributes.end()) {
                    std::int64_t flags = 0;
                    if (parseInteger(flagsIt->second, flags) && flags >= 0 && flags <= 0xFFFFFFFFLL) {
                        tile.flags = static_cast<std::uint32_t>(flags);
                    } else {
                        result.report.warnings.push_back("Invalid tileFlags metadata at imported tile.");
                    }
                } else {
                    result.report.warnings.push_back("Encountered metadata pseudo-item without a recognized payload.");
                }
                continue;
            }

            Item item = convertItem(sourceItem, assets, unresolved);
            if (!tile.ground.has_value() && isGround(sourceItem.serverId, assets)) {
                tile.ground = std::move(item);
            } else {
                tile.items.push_back(std::move(item));
            }
        }

        result.map.setTile(std::move(tile));
        ++result.report.tileCount;
    }

    for (const auto& sourceTown : source.import.model.towns) {
        Town town;
        town.id = sourceTown.id;
        town.name = sourceTown.name;
        town.templePosition = toPosition(sourceTown.templePosition);
        result.map.towns().insert_or_assign(town.id, std::move(town));
        ++result.report.townCount;
    }

    for (const auto& sourceWaypoint : source.import.model.waypoints) {
        Waypoint waypoint;
        waypoint.name = sourceWaypoint.name;
        waypoint.position = toPosition(sourceWaypoint.position);
        result.map.waypoints().insert_or_assign(waypoint.name, std::move(waypoint));
    }

    for (const auto& sourceHouse : source.import.model.houses) {
        House house;
        house.id = sourceHouse.id;
        house.name = sourceHouse.name;
        house.exit = toPosition(sourceHouse.entry);
        house.rent = sourceHouse.rent;
        house.townId = sourceHouse.townId;
        house.guildhall = sourceHouse.guildhall;
        result.map.houses().insert_or_assign(house.id, std::move(house));
        ++result.report.houseCount;
    }

    for (const auto& sourceSpawn : source.import.model.spawns) {
        SpawnArea spawn;
        spawn.center = toPosition(sourceSpawn.center);
        spawn.radius = sourceSpawn.radius;
        spawn.entries.reserve(sourceSpawn.entries.size());

        for (const auto& sourceEntry : sourceSpawn.entries) {
            SpawnEntry entry;
            entry.kind = toSpawnEntryKind(sourceEntry.kind);
            entry.position = toPosition(sourceEntry.position);
            entry.name = sourceEntry.name;
            entry.direction = sourceEntry.direction;
            entry.intervalSeconds = sourceEntry.spawnTimeSeconds;
            entry.monsters.reserve(sourceEntry.monsters.size());
            for (const auto& sourceMonster : sourceEntry.monsters) {
                entry.monsters.push_back(SpawnMonsterOption{sourceMonster.name, sourceMonster.chance});
            }
            spawn.entries.push_back(std::move(entry));
        }

        result.map.spawnAreas().push_back(std::move(spawn));
        ++result.report.spawnCount;
    }

    result.report.unresolvedServerIds.assign(unresolved.begin(), unresolved.end());
    result.report.warnings.insert(
        result.report.warnings.end(),
        source.import.diagnostics.warnings.begin(),
        source.import.diagnostics.warnings.end());

    return result;
}

LegacyMapAdaptReport LegacyMapAdapter::load(
    MapDocument& document,
    const fantasy::legacy::OtbmReadResult& source,
    const fantasy::assets::FantasyAssetRegistry& assets) const {

    LegacyMapAdaptResult adapted = adapt(source, assets);
    LegacyMapAdaptReport report = adapted.report;
    document.replaceMap(std::move(adapted.map), std::move(adapted.metadata));
    return report;
}

} // namespace fantasy::studio::mapcore
