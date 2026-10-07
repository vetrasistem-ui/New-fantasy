#pragma once

#include "ActionHistory.hpp"
#include "MapStorage.hpp"
#include "SelectionModel.hpp"

#include <cstdint>

namespace fantasy::studio::mapcore {

class MapDocument {
public:
    MapStorage& map() noexcept { return map_; }
    const MapStorage& map() const noexcept { return map_; }

    MapMetadata& metadata() noexcept { return metadata_; }
    const MapMetadata& metadata() const noexcept { return metadata_; }

    SelectionModel& selection() noexcept { return selection_; }
    const SelectionModel& selection() const noexcept { return selection_; }

    ActionHistory& history() noexcept { return history_; }
    const ActionHistory& history() const noexcept { return history_; }

    [[nodiscard]] std::uint64_t revision() const noexcept { return revision_; }

    void replaceMap(MapStorage map, MapMetadata metadata = {});
    void apply(MapAction action);
    bool undo();
    bool redo();

private:
    MapStorage map_;
    MapMetadata metadata_;
    SelectionModel selection_;
    ActionHistory history_;
    std::uint64_t revision_ = 0;
};

} // namespace fantasy::studio::mapcore
