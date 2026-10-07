#pragma once

#include "../Core/MapTypes.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace fantasy::studio::mapcore {

enum class CommandOrigin {
    Human,
    AI,
    Automation,
};

struct PaintGroundCommand {
    std::vector<Position> positions;
    std::optional<Item> ground;
};

struct PlaceItemCommand {
    Position position;
    Item item;
    std::optional<std::size_t> stackIndex;
};

struct RemoveItemCommand {
    Position position;
    std::size_t stackIndex = 0;
};

struct EraseTileCommand {
    std::vector<Position> positions;
};

using MapCommandPayload = std::variant<
    PaintGroundCommand,
    PlaceItemCommand,
    RemoveItemCommand,
    EraseTileCommand
>;

struct MapCommand {
    std::string requestId;
    CommandOrigin origin = CommandOrigin::Human;
    std::optional<std::uint64_t> expectedRevision;
    bool previewOnly = false;
    MapCommandPayload payload;
};

struct MapCommandBatch {
    std::string requestId;
    CommandOrigin origin = CommandOrigin::Human;
    std::optional<std::uint64_t> expectedRevision;
    bool previewOnly = false;
    std::vector<MapCommandPayload> commands;
};

} // namespace fantasy::studio::mapcore
