#include "SelectionModel.hpp"

#include <algorithm>

namespace fantasy::studio::mapcore {

void SelectionModel::clear() noexcept {
    positions_.clear();
}

void SelectionModel::selectOnly(const Position& position) {
    positions_.clear();
    positions_.insert(position);
}

void SelectionModel::add(const Position& position) {
    positions_.insert(position);
}

void SelectionModel::remove(const Position& position) {
    positions_.erase(position);
}

void SelectionModel::toggle(const Position& position) {
    if (positions_.contains(position)) positions_.erase(position);
    else positions_.insert(position);
}

void SelectionModel::selectRect(const Position& start, const Position& end, bool append) {
    if (!append) positions_.clear();
    if (start.z != end.z) return;

    const auto minX = std::min(start.x, end.x);
    const auto maxX = std::max(start.x, end.x);
    const auto minY = std::min(start.y, end.y);
    const auto maxY = std::max(start.y, end.y);

    for (std::int32_t y = minY; y <= maxY; ++y) {
        for (std::int32_t x = minX; x <= maxX; ++x) {
            positions_.insert(Position{x, y, start.z});
        }
    }
}

bool SelectionModel::contains(const Position& position) const {
    return positions_.contains(position);
}

std::optional<Position> SelectionModel::minPosition() const {
    if (positions_.empty()) return std::nullopt;
    Position result = *positions_.begin();
    for (const auto& position : positions_) {
        result.x = std::min(result.x, position.x);
        result.y = std::min(result.y, position.y);
        result.z = std::min(result.z, position.z);
    }
    return result;
}

std::optional<Position> SelectionModel::maxPosition() const {
    if (positions_.empty()) return std::nullopt;
    Position result = *positions_.begin();
    for (const auto& position : positions_) {
        result.x = std::max(result.x, position.x);
        result.y = std::max(result.y, position.y);
        result.z = std::max(result.z, position.z);
    }
    return result;
}

} // namespace fantasy::studio::mapcore
