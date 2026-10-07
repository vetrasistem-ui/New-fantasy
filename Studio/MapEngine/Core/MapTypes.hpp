#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace fantasy::studio::mapcore {

struct Position {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int16_t z = 7;
    bool operator==(const Position&) const = default;
    auto operator<=>(const Position&) const = default;
};

using AttributeValue = std::variant<std::int64_t, std::string, Position>;

struct Item {
    std::uint32_t serverId = 0;
    std::uint32_t clientId = 0;
    std::uint16_t countOrSubtype = 1;
    std::map<std::string, AttributeValue> attributes;
    std::vector<Item> contents;
    bool operator==(const Item&) const = default;
};

struct CreaturePlacement {
    std::string name;
    std::uint32_t lookType = 0;
    std::uint8_t direction = 2;
    bool operator==(const CreaturePlacement&) const = default;
};

struct SpawnPlacement {
    std::uint32_t radius = 1;
    std::uint32_t intervalSeconds = 60;
    bool operator==(const SpawnPlacement&) const = default;
};

struct Tile {
    Position position;
    std::optional<Item> ground;
    std::vector<Item> items;
    std::optional<CreaturePlacement> creature;
    std::optional<SpawnPlacement> spawn;
    std::uint32_t houseId = 0;
    std::uint32_t flags = 0;

    [[nodiscard]] bool empty() const noexcept {
        return !ground.has_value() && items.empty() && !creature.has_value() && !spawn.has_value() && houseId == 0 && flags == 0;
    }

    bool operator==(const Tile&) const = default;
};

struct House {
    std::uint32_t id = 0;
    std::string name;
    Position exit;
    bool operator==(const House&) const = default;
};

struct Waypoint {
    std::string name;
    Position position;
    bool operator==(const Waypoint&) const = default;
};

} // namespace fantasy::studio::mapcore
