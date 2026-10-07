#include "CommandExecutor.hpp"
#include "CommandValidator.hpp"

#include <optional>
#include <set>
#include <type_traits>
#include <utility>

namespace fantasy::studio::mapcore {
namespace {

std::optional<Tile> snapshot(const MapStorage& map, const Position& position) {
    if (const Tile* tile = map.findTile(position)) return *tile;
    return std::nullopt;
}

std::optional<Tile> normalize(Tile tile) {
    return tile.empty() ? std::nullopt : std::optional<Tile>{std::move(tile)};
}

} // namespace

CommandResult CommandExecutor::execute(MapDocument& document, const MapCommand& command) const {
    CommandResult result;
    result.requestId = command.requestId;
    result.baseRevision = document.revision();
    result.resultRevision = document.revision();

    if (command.expectedRevision.has_value() && *command.expectedRevision != document.revision()) {
        result.status = CommandStatus::RevisionConflict;
        result.message = "Map revision changed; query the affected region again before applying this command.";
        return result;
    }

    const ValidationReport validation = CommandValidator{}.validate(document, command);
    if (!validation.ok()) {
        result.status = CommandStatus::Invalid;
        result.message = validation.firstError();
        return result;
    }

    MapAction action;

    std::visit([&](const auto& payload) {
        using T = std::decay_t<decltype(payload)>;

        if constexpr (std::is_same_v<T, PaintGroundCommand>) {
            action.label = "Paint ground";
            for (const Position& position : payload.positions) {
                auto before = snapshot(document.map(), position);
                Tile after = before.value_or(Tile{});
                after.position = position;
                after.ground = payload.ground;
                action.changes.push_back(TileChange{position, std::move(before), normalize(std::move(after))});
            }
        } else if constexpr (std::is_same_v<T, PlaceItemCommand>) {
            action.label = "Place item";
            auto before = snapshot(document.map(), payload.position);
            Tile after = before.value_or(Tile{});
            after.position = payload.position;

            const std::size_t index = payload.stackIndex.value_or(after.items.size());
            after.items.insert(after.items.begin() + static_cast<std::ptrdiff_t>(index), payload.item);
            action.changes.push_back(TileChange{payload.position, std::move(before), normalize(std::move(after))});
        } else if constexpr (std::is_same_v<T, RemoveItemCommand>) {
            action.label = "Remove item";
            auto before = snapshot(document.map(), payload.position);
            Tile after = *before;
            after.items.erase(after.items.begin() + static_cast<std::ptrdiff_t>(payload.stackIndex));
            action.changes.push_back(TileChange{payload.position, std::move(before), normalize(std::move(after))});
        } else if constexpr (std::is_same_v<T, EraseTileCommand>) {
            action.label = "Erase tiles";
            for (const Position& position : payload.positions) {
                auto before = snapshot(document.map(), position);
                if (!before.has_value()) continue;
                action.changes.push_back(TileChange{position, std::move(before), std::nullopt});
            }
        }
    }, command.payload);

    result = buildResult(document, command, std::move(action));
    if (command.previewOnly || result.diff.changes.empty()) return result;

    MapAction actionToApply = result.diff;
    document.apply(std::move(actionToApply));
    result.status = CommandStatus::Applied;
    result.resultRevision = document.revision();
    result.message = "Command applied.";
    return result;
}

CommandResult CommandExecutor::buildResult(
    const MapDocument& document,
    const MapCommand& command,
    MapAction action) {

    CommandResult result;
    result.requestId = command.requestId;
    result.baseRevision = document.revision();
    result.resultRevision = document.revision();
    result.diff = std::move(action);

    std::set<Position> affected;
    for (const auto& change : result.diff.changes) affected.insert(change.position);
    result.affectedTiles = affected.size();

    result.status = CommandStatus::Preview;
    result.message = command.previewOnly ? "Preview generated; map not modified." :
        (result.diff.changes.empty() ? "Command produced no changes." : "Command validated and ready to apply.");
    return result;
}

} // namespace fantasy::studio::mapcore
