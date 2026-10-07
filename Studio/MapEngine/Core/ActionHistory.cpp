#include "ActionHistory.hpp"

#include <stdexcept>
#include <utility>

namespace fantasy::studio::mapcore {
namespace {

const std::string& emptyLabel() {
    static const std::string value;
    return value;
}

void applyTileState(MapStorage& map, const Position& position, const std::optional<Tile>& state) {
    if (state.has_value()) {
        Tile tile = *state;
        tile.position = position;
        map.setTile(std::move(tile));
    } else {
        map.eraseTile(position);
    }
}

} // namespace

void ActionHistory::pushAndApply(MapStorage& map, MapAction action) {
    if (action.changes.empty()) return;
    applyForward(map, action);
    undo_.push_back(std::move(action));
    redo_.clear();
}

bool ActionHistory::undo(MapStorage& map) {
    if (undo_.empty()) return false;
    MapAction action = std::move(undo_.back());
    undo_.pop_back();
    applyBackward(map, action);
    redo_.push_back(std::move(action));
    return true;
}

bool ActionHistory::redo(MapStorage& map) {
    if (redo_.empty()) return false;
    MapAction action = std::move(redo_.back());
    redo_.pop_back();
    applyForward(map, action);
    undo_.push_back(std::move(action));
    return true;
}

void ActionHistory::clear() noexcept {
    undo_.clear();
    redo_.clear();
}

const std::string& ActionHistory::undoLabel() const {
    return undo_.empty() ? emptyLabel() : undo_.back().label;
}

const std::string& ActionHistory::redoLabel() const {
    return redo_.empty() ? emptyLabel() : redo_.back().label;
}

void ActionHistory::applyForward(MapStorage& map, const MapAction& action) {
    for (const auto& change : action.changes) applyTileState(map, change.position, change.after);
}

void ActionHistory::applyBackward(MapStorage& map, const MapAction& action) {
    for (auto it = action.changes.rbegin(); it != action.changes.rend(); ++it) {
        applyTileState(map, it->position, it->before);
    }
}

} // namespace fantasy::studio::mapcore
