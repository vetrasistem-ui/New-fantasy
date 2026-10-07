#pragma once

#include "MapStorage.hpp"

#include <optional>
#include <string>
#include <vector>

namespace fantasy::studio::mapcore {

struct TileChange {
    Position position;
    std::optional<Tile> before;
    std::optional<Tile> after;
};

struct MapAction {
    std::string label;
    std::vector<TileChange> changes;
};

class ActionHistory {
public:
    void pushAndApply(MapStorage& map, MapAction action);
    bool undo(MapStorage& map);
    bool redo(MapStorage& map);
    void clear() noexcept;

    [[nodiscard]] bool canUndo() const noexcept { return !undo_.empty(); }
    [[nodiscard]] bool canRedo() const noexcept { return !redo_.empty(); }
    [[nodiscard]] const std::string& undoLabel() const;
    [[nodiscard]] const std::string& redoLabel() const;

private:
    static void applyForward(MapStorage& map, const MapAction& action);
    static void applyBackward(MapStorage& map, const MapAction& action);

    std::vector<MapAction> undo_;
    std::vector<MapAction> redo_;
};

} // namespace fantasy::studio::mapcore
