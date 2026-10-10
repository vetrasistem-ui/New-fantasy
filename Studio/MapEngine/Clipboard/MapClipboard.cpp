#include "MapClipboard.hpp"

#include <algorithm>
#include <utility>

namespace fantasy::studio::mapcore {
namespace {

CommandResult invalidClipboardResult(
    const MapDocument& document,
    std::string requestId,
    std::string message) {

    CommandResult result;
    result.status = CommandStatus::Invalid;
    result.requestId = std::move(requestId);
    result.message = std::move(message);
    result.baseRevision = document.revision();
    result.resultRevision = document.revision();
    return result;
}

Position translatedPosition(
    const Position& original,
    const Position& sourceAnchor,
    const Position& targetAnchor) {

    return Position{
        targetAnchor.x + (original.x - sourceAnchor.x),
        targetAnchor.y + (original.y - sourceAnchor.y),
        static_cast<std::int16_t>(targetAnchor.z + (original.z - sourceAnchor.z))
    };
}

} // namespace

bool MapClipboard::capture(const MapDocument& document, std::string* error) {
    ClipboardSnapshot captured;
    captured.sourceRevision = document.revision();

    bool hasAnchor = false;
    for (const Position& position : document.selection().positions()) {
        const Tile* tile = document.map().findTile(position);
        if (!tile) continue;

        captured.tiles.push_back(*tile);
        if (!hasAnchor) {
            captured.anchor = position;
            hasAnchor = true;
        } else {
            captured.anchor.x = std::min(captured.anchor.x, position.x);
            captured.anchor.y = std::min(captured.anchor.y, position.y);
            captured.anchor.z = std::min(captured.anchor.z, position.z);
        }
    }

    if (captured.tiles.empty()) {
        snapshot_.reset();
        if (error) *error = "Selection contains no map tiles to copy.";
        return false;
    }

    snapshot_ = std::move(captured);
    if (error) error->clear();
    return true;
}

void MapClipboard::clear() noexcept {
    snapshot_.reset();
}

CommandResult MapClipboard::cut(
    MapDocument& document,
    CommandOrigin origin,
    std::string requestId,
    bool previewOnly) {

    std::string error;
    if (!capture(document, &error)) {
        return invalidClipboardResult(document, std::move(requestId), std::move(error));
    }

    MapCommandBatch batch;
    batch.requestId = std::move(requestId);
    batch.label = "Cut tiles";
    batch.origin = origin;
    batch.expectedRevision = document.revision();
    batch.previewOnly = previewOnly;
    batch.commands.reserve(snapshot_->tiles.size());

    for (const Tile& tile : snapshot_->tiles) {
        batch.commands.push_back(ReplaceTileCommand{tile.position, std::nullopt});
    }

    return CommandExecutor{}.execute(document, batch);
}

Tile MapClipboard::mergeTile(
    const Tile* destination,
    const Tile& source,
    const Position& targetPosition,
    const PasteOptions& options) {

    Tile merged = destination ? *destination : Tile{};
    merged.position = targetPosition;

    if (source.ground.has_value()) merged.ground = source.ground;
    merged.items.insert(merged.items.end(), source.items.begin(), source.items.end());

    for (const auto& [key, value] : source.attributes) {
        merged.attributes.insert_or_assign(key, value);
    }

    if (source.creature.has_value()) merged.creature = source.creature;
    if (source.spawn.has_value()) merged.spawn = source.spawn;

    if (options.preserveHouseIds && source.houseId != 0) merged.houseId = source.houseId;
    if (options.preserveTileFlags) merged.flags |= source.flags;

    return merged;
}

CommandResult MapClipboard::paste(
    MapDocument& document,
    const Position& targetAnchor,
    const PasteOptions& options) const {

    if (empty()) {
        return invalidClipboardResult(document, options.requestId, "Clipboard is empty.");
    }

    MapCommandBatch batch;
    batch.requestId = options.requestId;
    batch.label = options.mode == PasteMode::Merge ? "Merge pasted tiles" : "Paste tiles";
    batch.origin = options.origin;
    batch.expectedRevision = options.expectedRevision.value_or(document.revision());
    batch.previewOnly = options.previewOnly;
    batch.commands.reserve(snapshot_->tiles.size());

    for (const Tile& sourceTile : snapshot_->tiles) {
        const Position target = translatedPosition(sourceTile.position, snapshot_->anchor, targetAnchor);
        const Tile* destination = document.map().findTile(target);

        Tile pasted;
        if (options.mode == PasteMode::Merge) {
            pasted = mergeTile(destination, sourceTile, target, options);
        } else {
            pasted = sourceTile;
            pasted.position = target;
            if (!options.preserveHouseIds) pasted.houseId = destination ? destination->houseId : 0;
            if (!options.preserveTileFlags) pasted.flags = destination ? destination->flags : 0;
        }

        batch.commands.push_back(ReplaceTileCommand{target, std::move(pasted)});
    }

    return CommandExecutor{}.execute(document, batch);
}

} // namespace fantasy::studio::mapcore
