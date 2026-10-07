#pragma once

#include "../Core/MapTypes.hpp"

#include <cstddef>
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

// Low-level but validated domain command used by clipboard/import/advanced AI
// operations. When tile is present its embedded position is normalized to
// `position` by the executor; null removes the tile.
struct ReplaceTileCommand {
    Position position;
    std::optional<Tile> tile;
};

using MapCommandPayload = std::variant<
    PaintGroundCommand,
    PlaceItemCommand,
    RemoveItemCommand,
    EraseTileCommand,
    ReplaceTileCommand
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
