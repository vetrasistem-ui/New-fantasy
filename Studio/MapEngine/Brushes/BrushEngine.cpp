#include "BrushEngine.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <utility>

namespace fantasy::studio::mapcore {
namespace {

std::uint64_t mix64(std::uint64_t value) noexcept {
    value += 0x9E3779B97F4A7C15ULL;
    value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
    return value ^ (value >> 31U);
}

std::uint64_t positionHash(const Position& position, std::uint64_t seed) noexcept {
    std::uint64_t value = seed;
    value ^= mix64(static_cast<std::uint32_t>(position.x));
    value ^= mix64(static_cast<std::uint32_t>(position.y) + 0x517CC1B727220A95ULL);
    value ^= mix64(static_cast<std::uint16_t>(position.z) + 0x6EED0E9DA4D94A4FULL);
    return mix64(value);
}

Position offset(const Position& position, std::int32_t dx, std::int32_t dy) {
    return Position{position.x + dx, position.y + dy, position.z};
}

CommandResult invalidBrushResult(
    const MapDocument& document,
    const BrushStroke& stroke,
    std::string message) {

    CommandResult result;
    result.status = CommandStatus::Invalid;
    result.requestId = stroke.requestId;
    result.message = std::move(message);
    result.baseRevision = document.revision();
    result.resultRevision = document.revision();
    return result;
}

} // namespace

bool BrushEngine::registerAutoBorder(AutoBorderDefinition definition, std::string* error) {
    if (definition.id.empty()) {
        if (error) *error = "Auto-border id cannot be empty.";
        return false;
    }
    if (definition.pieces.empty()) {
        if (error) *error = "Auto-border must define at least one border piece.";
        return false;
    }
    if (autoBorders_.contains(definition.id)) {
        if (error) *error = "Auto-border id already registered: " + definition.id;
        return false;
    }

    for (const auto& [kind, item] : definition.pieces) {
        (void)kind;
        if (item.serverId == 0) {
            if (error) *error = "Auto-border piece has unresolved serverId=0.";
            return false;
        }
    }

    for (const auto& [kind, item] : definition.pieces) {
        (void)kind;
        managedBorderServerIds_.insert(item.serverId);
    }
    autoBorders_.emplace(definition.id, std::move(definition));
    if (error) error->clear();
    return true;
}

bool BrushEngine::registerGroundBrush(GroundBrushDefinition definition, std::string* error) {
    if (definition.id.empty()) {
        if (error) *error = "Ground brush id cannot be empty.";
        return false;
    }
    if (definition.variants.empty()) {
        if (error) *error = "Ground brush must define at least one variant.";
        return false;
    }
    if (groundBrushes_.contains(definition.id)) {
        if (error) *error = "Ground brush id already registered: " + definition.id;
        return false;
    }
    if (definition.outerBorderId.has_value() && !autoBorders_.contains(*definition.outerBorderId)) {
        if (error) *error = "Ground brush references an unknown auto-border: " + *definition.outerBorderId;
        return false;
    }

    std::uint64_t totalWeight = 0;
    for (const auto& variant : definition.variants) {
        if (variant.item.serverId == 0) {
            if (error) *error = "Ground brush variant has unresolved serverId=0.";
            return false;
        }
        const auto existing = groundServerToBrush_.find(variant.item.serverId);
        if (existing != groundServerToBrush_.end() && existing->second != definition.id) {
            if (error) *error = "Ground serverId already belongs to brush: " + existing->second;
            return false;
        }
        totalWeight += variant.weight;
    }

    if (totalWeight == 0) {
        if (error) *error = "Ground brush must have at least one variant with positive weight.";
        return false;
    }
    if (totalWeight > std::numeric_limits<std::uint32_t>::max()) {
        if (error) *error = "Ground brush total variant weight exceeds supported range.";
        return false;
    }

    const std::string brushId = definition.id;
    for (const auto& variant : definition.variants) {
        groundServerToBrush_[variant.item.serverId] = brushId;
    }
    groundBrushes_.emplace(brushId, std::move(definition));
    if (error) error->clear();
    return true;
}

const AutoBorderDefinition* BrushEngine::findAutoBorder(const std::string& id) const noexcept {
    const auto it = autoBorders_.find(id);
    return it == autoBorders_.end() ? nullptr : &it->second;
}

const GroundBrushDefinition* BrushEngine::findGroundBrush(const std::string& id) const noexcept {
    const auto it = groundBrushes_.find(id);
    return it == groundBrushes_.end() ? nullptr : &it->second;
}

std::vector<Position> BrushEngine::buildFootprint(const BrushStroke& stroke) {
    std::set<Position> unique;

    for (const Position& center : stroke.centers) {
        if (stroke.shape == BrushShape::Point || stroke.radius == 0) {
            unique.insert(center);
            continue;
        }

        const std::int32_t radius = stroke.radius;
        for (std::int32_t dy = -radius; dy <= radius; ++dy) {
            for (std::int32_t dx = -radius; dx <= radius; ++dx) {
                if (stroke.shape == BrushShape::Circle && dx * dx + dy * dy > radius * radius) continue;
                unique.insert(Position{center.x + dx, center.y + dy, center.z});
            }
        }
    }

    return {unique.begin(), unique.end()};
}

std::size_t BrushEngine::selectVariant(
    const GroundBrushDefinition& definition,
    const Position& position,
    std::uint64_t seed) noexcept {

    std::uint64_t totalWeight = 0;
    for (const auto& variant : definition.variants) totalWeight += variant.weight;
    if (totalWeight == 0) return 0;

    const std::uint64_t choice = positionHash(position, seed) % totalWeight;
    std::uint64_t cursor = 0;
    for (std::size_t index = 0; index < definition.variants.size(); ++index) {
        cursor += definition.variants[index].weight;
        if (definition.variants[index].weight != 0 && choice < cursor) return index;
    }
    return 0;
}

const GroundBrushDefinition* BrushEngine::brushForGround(const std::optional<Item>& ground) const noexcept {
    if (!ground.has_value()) return nullptr;
    const auto membership = groundServerToBrush_.find(ground->serverId);
    if (membership == groundServerToBrush_.end()) return nullptr;
    return findGroundBrush(membership->second);
}

bool BrushEngine::areFriendly(
    const GroundBrushDefinition& owner,
    const GroundBrushDefinition* other) const noexcept {

    if (!other) return false;
    if (owner.id == other->id) return true;
    if (std::find(owner.friends.begin(), owner.friends.end(), other->id) != owner.friends.end()) return true;
    return std::find(other->friends.begin(), other->friends.end(), owner.id) != other->friends.end();
}

std::optional<Item> BrushEngine::effectiveGround(
    const MapDocument& document,
    const Position& position,
    const std::map<Position, Item>& groundOverrides) const {

    const auto overrideIt = groundOverrides.find(position);
    if (overrideIt != groundOverrides.end()) return overrideIt->second;
    if (const Tile* tile = document.map().findTile(position)) return tile->ground;
    return std::nullopt;
}

std::vector<BorderPieceKind> BrushEngine::selectBorderPieces(
    const MapDocument& document,
    const GroundBrushDefinition& owner,
    const Position& position,
    const std::map<Position, Item>& groundOverrides) const {

    const auto exposed = [&](std::int32_t dx, std::int32_t dy) {
        const auto ground = effectiveGround(document, offset(position, dx, dy), groundOverrides);
        return !areFriendly(owner, brushForGround(ground));
    };

    const bool north = exposed(0, -1);
    const bool northEast = exposed(1, -1);
    const bool east = exposed(1, 0);
    const bool southEast = exposed(1, 1);
    const bool south = exposed(0, 1);
    const bool southWest = exposed(-1, 1);
    const bool west = exposed(-1, 0);
    const bool northWest = exposed(-1, -1);

    const int cardinalCount = static_cast<int>(north) + static_cast<int>(east) +
        static_cast<int>(south) + static_cast<int>(west);

    std::vector<BorderPieceKind> pieces;
    if (cardinalCount == 2 && north && west) {
        pieces.push_back(BorderPieceKind::DiagonalNorthWest);
    } else if (cardinalCount == 2 && north && east) {
        pieces.push_back(BorderPieceKind::DiagonalNorthEast);
    } else if (cardinalCount == 2 && south && east) {
        pieces.push_back(BorderPieceKind::DiagonalSouthEast);
    } else if (cardinalCount == 2 && south && west) {
        pieces.push_back(BorderPieceKind::DiagonalSouthWest);
    } else {
        if (north) pieces.push_back(BorderPieceKind::North);
        if (east) pieces.push_back(BorderPieceKind::East);
        if (south) pieces.push_back(BorderPieceKind::South);
        if (west) pieces.push_back(BorderPieceKind::West);
    }

    if (northWest && !north && !west) pieces.push_back(BorderPieceKind::CornerNorthWest);
    if (northEast && !north && !east) pieces.push_back(BorderPieceKind::CornerNorthEast);
    if (southEast && !south && !east) pieces.push_back(BorderPieceKind::CornerSouthEast);
    if (southWest && !south && !west) pieces.push_back(BorderPieceKind::CornerSouthWest);
    return pieces;
}

void BrushEngine::removeManagedBorders(Tile& tile) const {
    std::erase_if(tile.items, [&](const Item& item) {
        return managedBorderServerIds_.contains(item.serverId);
    });
}

void BrushEngine::appendOuterBorders(
    Tile& tile,
    const GroundBrushDefinition& owner,
    const std::vector<BorderPieceKind>& pieces) const {

    if (!owner.outerBorderId.has_value()) return;
    const AutoBorderDefinition* border = findAutoBorder(*owner.outerBorderId);
    if (!border) return;

    for (const BorderPieceKind kind : pieces) {
        const auto piece = border->pieces.find(kind);
        if (piece != border->pieces.end()) tile.items.push_back(piece->second);
    }
}

BrushPlanResult BrushEngine::planGroundStroke(
    const MapDocument& document,
    const BrushStroke& stroke) const {

    BrushPlanResult result;
    result.batch.requestId = stroke.requestId;
    result.batch.origin = stroke.origin;
    result.batch.expectedRevision = stroke.expectedRevision.value_or(document.revision());
    result.batch.previewOnly = stroke.previewOnly;

    const GroundBrushDefinition* definition = findGroundBrush(stroke.brushId);
    if (!definition) {
        result.message = "Ground brush is not registered: " + stroke.brushId;
        return result;
    }
    if (stroke.centers.empty()) {
        result.message = "Brush stroke has no center positions.";
        return result;
    }
    if (stroke.radius > definition->maxRadius) {
        result.message = "Brush radius exceeds the brush definition limit.";
        return result;
    }
    if (!definition->canDrag && stroke.centers.size() > 1) {
        result.message = "This brush does not support drag strokes.";
        return result;
    }

    const std::vector<Position> footprint = buildFootprint(stroke);
    result.footprintTileCount = footprint.size();
    result.batch.label = "Ground brush: " + definition->id;

    std::map<Position, Item> groundOverrides;
    for (const Position& position : footprint) {
        groundOverrides[position] = definition->variants[selectVariant(*definition, position, stroke.seed)].item;
    }

    std::set<Position> impacted;
    for (const Position& position : footprint) {
        for (std::int32_t dy = -1; dy <= 1; ++dy) {
            for (std::int32_t dx = -1; dx <= 1; ++dx) {
                impacted.insert(offset(position, dx, dy));
            }
        }
    }

    for (const Position& position : impacted) {
        const Tile* existing = document.map().findTile(position);
        Tile after = existing ? *existing : Tile{};
        after.position = position;

        const auto overrideIt = groundOverrides.find(position);
        if (overrideIt != groundOverrides.end()) after.ground = overrideIt->second;

        removeManagedBorders(after);
        if (const GroundBrushDefinition* tileBrush = brushForGround(after.ground)) {
            appendOuterBorders(after, *tileBrush, selectBorderPieces(document, *tileBrush, position, groundOverrides));
        }

        const std::optional<Tile> before = existing ? std::optional<Tile>{*existing} : std::nullopt;
        const std::optional<Tile> finalState = after.empty() ? std::nullopt : std::optional<Tile>{std::move(after)};
        if (before == finalState) continue;
        result.batch.commands.push_back(ReplaceTileCommand{position, finalState});
    }

    result.success = !result.batch.commands.empty();
    result.message = result.success ? "Brush stroke and auto-borders planned." : "Brush stroke produced no changes.";
    return result;
}

CommandResult BrushEngine::executeGroundStroke(MapDocument& document, const BrushStroke& stroke) const {
    BrushPlanResult plan = planGroundStroke(document, stroke);
    if (!plan.success) return invalidBrushResult(document, stroke, std::move(plan.message));
    return CommandExecutor{}.execute(document, plan.batch);
}

} // namespace fantasy::studio::mapcore
