#include "MapDocument.hpp"

#include <utility>

namespace fantasy::studio::mapcore {

void MapDocument::replaceMap(MapStorage map, MapMetadata metadata) {
    map_ = std::move(map);
    metadata_ = std::move(metadata);
    selection_.clear();
    history_.clear();
    revision_ = 0;
}

void MapDocument::apply(MapAction action) {
    if (action.changes.empty()) return;
    history_.pushAndApply(map_, std::move(action));
    ++revision_;
}

bool MapDocument::undo() {
    if (!history_.undo(map_)) return false;
    ++revision_;
    return true;
}

bool MapDocument::redo() {
    if (!history_.redo(map_)) return false;
    ++revision_;
    return true;
}

} // namespace fantasy::studio::mapcore
