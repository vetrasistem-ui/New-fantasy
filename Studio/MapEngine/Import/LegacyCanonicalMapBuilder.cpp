#include "LegacyCanonicalMapBuilder.hpp"

#include <charconv>
#include <string_view>
#include <utility>

namespace fantasy::studio::mapcore {
namespace {

Position toPosition(const fantasy::legacy::LegacyPosition& source) {
    return Position{source.x, source.y, source.z};
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
        static_cast<std::int16_t>(z)};
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

LegacyCanonicalMapBuilder::LegacyCanonicalMapBuilder(
    const fantasy::assets::FantasyAssetRegistry& assets)
    : assets_(assets) {}

void LegacyCanonicalMapBuilder::setMetadata(MapMetadata metadata) {
    metadata_ = std::move(metadata);
}

void LegacyCanonicalMapBuilder::appendWarning(std::string warning) {
    report_.warnings.push_back(std::move(warning));
}

Item LegacyCanonicalMapBuilder::convertItem(
    const fantasy::legacy::LegacyImportedItem& source) {

    Item item;
    item.serverId = source.serverId;

    if (const auto* record = assets_.findByLegacyServerId(source.serverId)) {
        item.clientId = record->clientId;
    } else if (source.serverId != 0) {
        unresolved_.insert(source.serverId);
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
        item.contents.push_back(convertItem(child));
    }
    return item;
}

bool LegacyCanonicalMapBuilder::isGround(std::uint32_t serverId) const {
    const auto* record = assets_.findByLegacyServerId(serverId);
    return record && record->kind == fantasy::assets::LegacyAssetKind::Ground;
}

void LegacyCanonicalMapBuilder::addTile(fantasy::legacy::LegacyImportedTile&& source) {
    Tile tile;
    tile.position = toPosition(source.position);
    tile.houseId = source.houseId.value_or(0);

    for (const auto& [key, value] : source.attributes) {
        tile.attributes[key] = toAttributeValue(key, value);
    }

    if (source.groundServerId.has_value()) {
        fantasy::legacy::LegacyImportedItem ground;
        ground.serverId = *source.groundServerId;
        tile.ground = convertItem(ground);
    }

    for (const auto& sourceItem : source.items) {
        if (sourceItem.serverId == 0) {
            const auto flagsIt = sourceItem.attributes.find("tileFlags");
            if (flagsIt != sourceItem.attributes.end()) {
                std::int64_t flags = 0;
                if (parseInteger(flagsIt->second, flags) && flags >= 0 && flags <= 0xFFFFFFFFLL) {
                    tile.flags = static_cast<std::uint32_t>(flags);
                } else {
                    report_.warnings.push_back("Invalid tileFlags metadata at imported tile.");
                }
            }
            continue;
        }

        Item item = convertItem(sourceItem);
        if (!tile.ground.has_value() && isGround(sourceItem.serverId)) {
            tile.ground = std::move(item);
        } else {
            tile.items.push_back(std::move(item));
        }
    }

    map_.setTile(std::move(tile));
    ++report_.tileCount;
}

void LegacyCanonicalMapBuilder::addTown(fantasy::legacy::LegacyImportedTown&& source) {
    Town town;
    town.id = source.id;
    town.name = std::move(source.name);
    town.templePosition = toPosition(source.templePosition);
    map_.towns().insert_or_assign(town.id, std::move(town));
    ++report_.townCount;
}

void LegacyCanonicalMapBuilder::addWaypoint(fantasy::legacy::LegacyImportedWaypoint&& source) {
    Waypoint waypoint;
    waypoint.name = std::move(source.name);
    waypoint.position = toPosition(source.position);
    map_.waypoints().insert_or_assign(waypoint.name, std::move(waypoint));
}

void LegacyCanonicalMapBuilder::addHouse(fantasy::legacy::LegacyImportedHouse&& source) {
    House house;
    house.id = source.id;
    house.name = std::move(source.name);
    house.exit = toPosition(source.entry);
    house.rent = source.rent;
    house.townId = source.townId;
    house.guildhall = source.guildhall;
    map_.houses().insert_or_assign(house.id, std::move(house));
    ++report_.houseCount;
}

void LegacyCanonicalMapBuilder::addSpawn(fantasy::legacy::LegacyImportedSpawn&& source) {
    SpawnArea spawn;
    spawn.center = toPosition(source.center);
    spawn.radius = source.radius;
    spawn.entries.reserve(source.entries.size());

    for (auto& sourceEntry : source.entries) {
        SpawnEntry entry;
        entry.kind = toSpawnEntryKind(sourceEntry.kind);
        entry.position = toPosition(sourceEntry.position);
        entry.name = std::move(sourceEntry.name);
        entry.direction = sourceEntry.direction;
        entry.intervalSeconds = sourceEntry.spawnTimeSeconds;
        entry.monsters.reserve(sourceEntry.monsters.size());
        for (auto& sourceMonster : sourceEntry.monsters) {
            entry.monsters.push_back(SpawnMonsterOption{
                std::move(sourceMonster.name), sourceMonster.chance});
        }
        spawn.entries.push_back(std::move(entry));
    }

    map_.spawnAreas().push_back(std::move(spawn));
    ++report_.spawnCount;
}

LegacyMapAdaptResult LegacyCanonicalMapBuilder::finish() {
    report_.unresolvedServerIds.assign(unresolved_.begin(), unresolved_.end());

    LegacyMapAdaptResult result;
    result.map = std::move(map_);
    result.metadata = std::move(metadata_);
    result.report = std::move(report_);
    return result;
}

} // namespace fantasy::studio::mapcore
