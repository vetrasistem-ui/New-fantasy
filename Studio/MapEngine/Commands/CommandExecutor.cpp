#include "CommandExecutor.hpp"
#include "CommandValidator.hpp"

#include <map>
#include <optional>
#include <set>
#include <type_traits>
#include <utility>

namespace fantasy::studio::mapcore {
namespace {

using TileOverlay = std::map<Position, std::optional<Tile>>;

std::optional<Tile> snapshot(
    const MapStorage& map,
    const TileOverlay& overlay,
    const Position& position) {

    const auto overlayIt = overlay.find(position);
    if (overlayIt != overlay.end()) return overlayIt->second;
    if (const Tile* tile = map.findTile(position)) return *tile;
    return std::nullopt;
}

std::optional<Tile> normalize(Tile tile) {
    return tile.empty() ? std::nullopt : std::optional<Tile>{std::move(tile)};
}

void appendChange(
    MapAction& action,
    TileOverlay& overlay,
    const Position& position,
    std::optional<Tile> before,
    std::optional<Tile> after) {

    if (before == after) {
        overlay[position] = std::move(after);
        return;
    }

    action.changes.push_back(TileChange{position, std::move(before), after});
    overlay[position] = std::move(after);
}

std::string labelFor(const MapCommandPayload& payload) {
    return std::visit([](const auto& command) -> std::string {
        using T = std::decay_t<decltype(command)>;
        if constexpr (std::is_same_v<T, PaintGroundCommand>) return "Paint ground";
        if constexpr (std::is_same_v<T, PlaceItemCommand>) return "Place item";
        if constexpr (std::is_same_v<T, RemoveItemCommand>) return "Remove item";
        if constexpr (std::is_same_v<T, EraseTileCommand>) return "Erase tiles";
        if constexpr (std::is_same_v<T, ReplaceTileCommand>) return "Replace tile";
        return "Map edit";
    }, payload);
}

bool appendPayload(
    const MapStorage& map,
    TileOverlay& overlay,
    const MapCommandPayload& payload,
    MapAction& action,
    std::string& error) {

    bool ok = true;

    std::visit([&](const auto& command) {
        using T = std::decay_t<decltype(command)>;

        if constexpr (std::is_same_v<T, PaintGroundCommand>) {
            for (const Position& position : command.positions) {
                auto before = snapshot(map, overlay, position);
                Tile after = before.value_or(Tile{});
                after.position = position;
                after.ground = command.ground;
                appendChange(action, overlay, position, std::move(before), normalize(std::move(after)));
            }
        } else if constexpr (std::is_same_v<T, PlaceItemCommand>) {
            auto before = snapshot(map, overlay, command.position);
            Tile after = before.value_or(Tile{});
            after.position = command.position;

            const std::size_t index = command.stackIndex.value_or(after.items.size());
            if (index > after.items.size()) {
                ok = false;
                error = "Requested stack index is outside the tile item stack.";
                return;
            }

            after.items.insert(after.items.begin() + static_cast<std::ptrdiff_t>(index), command.item);
            appendChange(action, overlay, command.position, std::move(before), normalize(std::move(after)));
        } else if constexpr (std::is_same_v<T, RemoveItemCommand>) {
            auto before = snapshot(map, overlay, command.position);
            if (!before.has_value() || command.stackIndex >= before->items.size()) {
                ok = false;
                error = "Requested item does not exist at the specified stack index.";
                return;
            }

            Tile after = *before;
            after.items.erase(after.items.begin() + static_cast<std::ptrdiff_t>(command.stackIndex));
            appendChange(action, overlay, command.position, std::move(before), normalize(std::move(after)));
        } else if constexpr (std::is_same_v<T, EraseTileCommand>) {
            for (const Position& position : command.positions) {
                auto before = snapshot(map, overlay, position);
                if (!before.has_value()) continue;
                appendChange(action, overlay, position, std::move(before), std::nullopt);
            }
        } else if constexpr (std::is_same_v<T, ReplaceTileCommand>) {
            auto before = snapshot(map, overlay, command.position);
            std::optional<Tile> after;
            if (command.tile.has_value()) {
                Tile replacement = *command.tile;
                replacement.position = command.position;
                after = normalize(std::move(replacement));
            }
            appendChange(action, overlay, command.position, std::move(before), std::move(after));
        }
    }, payload);

    return ok;
}

CommandResult conflictResult(
    const MapDocument& document,
    const std::string& requestId) {

    CommandResult result;
    result.requestId = requestId;
    result.baseRevision = document.revision();
    result.resultRevision = document.revision();
    result.status = CommandStatus::RevisionConflict;
    result.message = "Map revision changed; query the affected region again before applying this command.";
    return result;
}

CommandResult invalidResult(
    const MapDocument& document,
    const std::string& requestId,
    std::string message) {

    CommandResult result;
    result.requestId = requestId;
    result.baseRevision = document.revision();
    result.resultRevision = document.revision();
    result.status = CommandStatus::Invalid;
    result.message = std::move(message);
    return result;
}

} // namespace

CommandResult CommandExecutor::execute(MapDocument& document, const MapCommand& command) const {
    if (command.expectedRevision.has_value() && *command.expectedRevision != document.revision()) {
        return conflictResult(document, command.requestId);
    }

    const ValidationReport validation = CommandValidator{}.validate(document, command);
    if (!validation.ok()) return invalidResult(document, command.requestId, validation.firstError());

    MapAction action;
    action.label = labelFor(command.payload);
    TileOverlay overlay;
    std::string error;

    if (!appendPayload(document.map(), overlay, command.payload, action, error)) {
        return invalidResult(document, command.requestId, std::move(error));
    }

    CommandResult result = buildResult(document, command.requestId, command.previewOnly, std::move(action));
    if (command.previewOnly || result.diff.changes.empty()) return result;

    MapAction actionToApply = result.diff;
    document.apply(std::move(actionToApply));
    result.status = CommandStatus::Applied;
    result.resultRevision = document.revision();
    result.message = "Command applied.";
    return result;
}

CommandResult CommandExecutor::execute(MapDocument& document, const MapCommandBatch& batch) const {
    if (batch.expectedRevision.has_value() && *batch.expectedRevision != document.revision()) {
        return conflictResult(document, batch.requestId);
    }

    if (batch.commands.empty()) {
        return invalidResult(document, batch.requestId, "Command batch is empty.");
    }

    MapAction action;
    action.label = "Batch edit";
    TileOverlay overlay;
    std::string error;

    for (const auto& payload : batch.commands) {
        MapCommand structural;
        structural.requestId = batch.requestId;
        structural.origin = batch.origin;
        structural.expectedRevision = batch.expectedRevision;
        structural.previewOnly = batch.previewOnly;
        structural.payload = payload;

        const ValidationReport validation = CommandValidator{}.validate(document, structural);
        if (!validation.ok()) return invalidResult(document, batch.requestId, validation.firstError());

        if (!appendPayload(document.map(), overlay, payload, action, error)) {
            return invalidResult(document, batch.requestId, std::move(error));
        }
    }

    CommandResult result = buildResult(document, batch.requestId, batch.previewOnly, std::move(action));
    if (batch.previewOnly || result.diff.changes.empty()) return result;

    MapAction actionToApply = result.diff;
    document.apply(std::move(actionToApply));
    result.status = CommandStatus::Applied;
    result.resultRevision = document.revision();
    result.message = "Command batch applied atomically.";
    return result;
}

CommandResult CommandExecutor::buildResult(
    const MapDocument& document,
    std::string requestId,
    bool previewOnly,
    MapAction action) {

    CommandResult result;
    result.requestId = std::move(requestId);
    result.baseRevision = document.revision();
    result.resultRevision = document.revision();
    result.diff = std::move(action);

    std::set<Position> affected;
    for (const auto& change : result.diff.changes) affected.insert(change.position);
    result.affectedTiles = affected.size();

    result.status = CommandStatus::Preview;
    result.message = previewOnly ? "Preview generated; map not modified." :
        (result.diff.changes.empty() ? "Command produced no changes." : "Command validated and ready to apply.");
    return result;
}

} // namespace fantasy::studio::mapcore
