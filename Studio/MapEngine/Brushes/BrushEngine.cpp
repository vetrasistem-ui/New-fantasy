#include "BrushEngine.hpp"

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

bool BrushEngine::registerGroundBrush(GroundBrushDefinition definition, std::string* error) {
    if (definition.id.empty()) {
        if (error) *error = "Ground brush id cannot be empty.";
        return false;
    }
    if (definition.variants.empty()) {
        if (error) *error = "Ground brush must define at least one variant.";
        return false;
    }

    std::uint64_t totalWeight = 0;
    for (const auto& variant : definition.variants) {
        if (variant.item.serverId == 0) {
            if (error) *error = "Ground brush variant has unresolved serverId=0.";
            return false;
        }
        if (variant.weight == 0) {
            if (error) *error = "Ground brush variant weight must be greater than zero.";
            return false;
        }
        totalWeight += variant.weight;
    }

    if (totalWeight > std::numeric_limits<std::uint32_t>::max()) {
        if (error) *error = "Ground brush total variant weight exceeds supported range.";
        return false;
    }

    if (groundBrushes_.contains(definition.id)) {
        if (error) *error = "Ground brush id already registered: " + definition.id;
        return false;
    }

    groundBrushes_.emplace(definition.id, std::move(definition));
    if (error) error->clear();
    return true;
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
        if (choice < cursor) return index;
    }
    return definition.variants.size() - 1;
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

    std::vector<std::vector<Position>> byVariant(definition->variants.size());
    for (const Position& position : footprint) {
        byVariant[selectVariant(*definition, position, stroke.seed)].push_back(position);
    }

    for (std::size_t index = 0; index < byVariant.size(); ++index) {
        if (byVariant[index].empty()) continue;
        result.batch.commands.push_back(PaintGroundCommand{
            std::move(byVariant[index]),
            definition->variants[index].item
        });
    }

    result.success = !result.batch.commands.empty();
    result.message = result.success ? "Brush stroke planned." : "Brush stroke produced no commands.";
    return result;
}

CommandResult BrushEngine::executeGroundStroke(MapDocument& document, const BrushStroke& stroke) const {
    BrushPlanResult plan = planGroundStroke(document, stroke);
    if (!plan.success) return invalidBrushResult(document, stroke, std::move(plan.message));
    return CommandExecutor{}.execute(document, plan.batch);
}

} // namespace fantasy::studio::mapcore
