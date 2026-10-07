#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fantasy::legacy {

struct LegacyPosition {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int16_t z = 7;
};

struct LegacyImportedItem {
    std::uint32_t serverId = 0;
    std::unordered_map<std::string, std::string> attributes;
};

struct LegacyImportedTile {
    LegacyPosition position;
    std::optional<std::uint32_t> groundServerId;
    std::vector<LegacyImportedItem> items;
    std::optional<std::uint32_t> houseId;
    std::unordered_map<std::string, std::string> attributes;
};

struct LegacyImportedTown {
    std::uint32_t id = 0;
    std::string name;
    LegacyPosition templePosition;
};

struct LegacyImportedWaypoint {
    std::string name;
    LegacyPosition position;
};

struct LegacyImportedHouse {
    std::uint32_t id = 0;
    std::string name;
    LegacyPosition entry;
    std::uint32_t rent = 0;
    std::uint32_t townId = 0;
    bool guildhall = false;
};

enum class LegacySpawnEntryKind {
    Monster,
    Npc,
    MonsterSet,
};

struct LegacySpawnMonsterOption {
    std::string name;
    std::uint16_t chance = 0;
};

struct LegacyImportedSpawnEntry {
    LegacySpawnEntryKind kind = LegacySpawnEntryKind::Monster;
    LegacyPosition position;
    std::string name;
    std::uint16_t direction = 0;
    std::uint32_t spawnTimeSeconds = 0;
    std::vector<LegacySpawnMonsterOption> monsters;
};

struct LegacyImportedSpawn {
    LegacyPosition center;
    std::int32_t radius = 0;
    std::vector<LegacyImportedSpawnEntry> entries;
};

struct LegacyMapImportModel {
    std::string sourceProfileId;
    std::string sourceMapName;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<LegacyImportedTile> tiles;
    std::vector<LegacyImportedTown> towns;
    std::vector<LegacyImportedWaypoint> waypoints;
    std::vector<LegacyImportedHouse> houses;
    std::vector<LegacyImportedSpawn> spawns;
};

struct LegacyImportDiagnostics {
    std::size_t tileCount = 0;
    std::size_t itemCount = 0;
    std::size_t townCount = 0;
    std::size_t waypointCount = 0;
    std::size_t houseCount = 0;
    std::size_t spawnCount = 0;
    std::size_t spawnEntryCount = 0;
    std::vector<std::uint32_t> unknownServerIds;
    std::vector<std::string> warnings;
};

struct LegacyMapImportResult {
    LegacyMapImportModel model;
    LegacyImportDiagnostics diagnostics;
};

} // namespace fantasy::legacy
