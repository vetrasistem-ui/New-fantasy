#include "CommandValidator.hpp"

#include <cstddef>
#include <type_traits>

namespace fantasy::studio::mapcore {

bool ValidationReport::ok() const noexcept {
    for (const auto& issue : issues) {
        if (issue.severity == ValidationSeverity::Error) return false;
    }
    return true;
}

std::string ValidationReport::firstError() const {
    for (const auto& issue : issues) {
        if (issue.severity == ValidationSeverity::Error) return issue.message;
    }
    return {};
}

void CommandValidator::validatePosition(ValidationReport& report, const Position& position) {
    if (position.x < 0 || position.x > 65535 || position.y < 0 || position.y > 65535) {
        report.issues.push_back({
            ValidationSeverity::Error,
            "position.out_of_otbm_bounds",
            "Position cannot be represented by the OTBM 10.98 target (x/y must be between 0 and 65535).",
            position
        });
    }

    if (position.z < 0 || position.z > 15) {
        report.issues.push_back({
            ValidationSeverity::Error,
            "position.invalid_floor",
            "Floor must be between 0 and 15 for the TFS 1.4.2 / 10.98 target.",
            position
        });
    }
}

void CommandValidator::validateItem(
    ValidationReport& report,
    const Item& item,
    const std::optional<Position>& position) {

    if (item.serverId == 0) {
        report.issues.push_back({
            ValidationSeverity::Error,
            "item.missing_server_id",
            "Item serverId must be resolved before it can be committed to the map.",
            position
        });
    }

    for (const auto& child : item.contents) validateItem(report, child, position);
}

ValidationReport CommandValidator::validate(const MapDocument& document, const MapCommand& command) const {
    ValidationReport report;

    std::visit([&](const auto& payload) {
        using T = std::decay_t<decltype(payload)>;

        if constexpr (std::is_same_v<T, PaintGroundCommand>) {
            if (payload.positions.empty()) {
                report.issues.push_back({ValidationSeverity::Error, "paint.empty", "Paint command has no target positions.", std::nullopt});
                return;
            }
            for (const auto& position : payload.positions) validatePosition(report, position);
            if (payload.ground.has_value()) validateItem(report, *payload.ground, payload.positions.front());
        } else if constexpr (std::is_same_v<T, PlaceItemCommand>) {
            validatePosition(report, payload.position);
            validateItem(report, payload.item, payload.position);

            if (payload.stackIndex.has_value()) {
                const Tile* tile = document.map().findTile(payload.position);
                const std::size_t stackSize = tile ? tile->items.size() : 0;
                if (*payload.stackIndex > stackSize) {
                    report.issues.push_back({
                        ValidationSeverity::Error,
                        "item.invalid_stack_index",
                        "Requested stack index is outside the tile item stack.",
                        payload.position
                    });
                }
            }
        } else if constexpr (std::is_same_v<T, RemoveItemCommand>) {
            validatePosition(report, payload.position);
            const Tile* tile = document.map().findTile(payload.position);
            if (!tile || payload.stackIndex >= tile->items.size()) {
                report.issues.push_back({
                    ValidationSeverity::Error,
                    "item.not_found",
                    "Requested item does not exist at the specified stack index.",
                    payload.position
                });
            }
        } else if constexpr (std::is_same_v<T, EraseTileCommand>) {
            if (payload.positions.empty()) {
                report.issues.push_back({ValidationSeverity::Error, "erase.empty", "Erase command has no target positions.", std::nullopt});
                return;
            }
            for (const auto& position : payload.positions) validatePosition(report, position);
        }
    }, command.payload);

    return report;
}

} // namespace fantasy::studio::mapcore
