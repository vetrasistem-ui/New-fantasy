#pragma once

#include "MapTypes.hpp"

#include <cstddef>
#include <optional>
#include <set>

namespace fantasy::studio::mapcore {

class SelectionModel {
public:
    void clear() noexcept;
    void selectOnly(const Position& position);
    void add(const Position& position);
    void remove(const Position& position);
    void toggle(const Position& position);
    void selectRect(const Position& start, const Position& end, bool append = false);

    [[nodiscard]] bool contains(const Position& position) const;
    [[nodiscard]] bool empty() const noexcept { return positions_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return positions_.size(); }
    [[nodiscard]] const std::set<Position>& positions() const noexcept { return positions_; }
    [[nodiscard]] std::optional<Position> minPosition() const;
    [[nodiscard]] std::optional<Position> maxPosition() const;

private:
    std::set<Position> positions_;
};

} // namespace fantasy::studio::mapcore
