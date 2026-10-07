#pragma once

#include "../Commands/CommandExecutor.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fantasy::studio::mapcore {

enum class BrushShape {
    Point,
    Square,
    Circle,
};

struct WeightedBrushItem {
    Item item;
    std::uint32_t weight = 1;
};

struct GroundBrushDefinition {
    std::string id;
    std::vector<WeightedBrushItem> variants;
    std::uint8_t maxRadius = 16;
    bool canDrag = true;
};

struct BrushStroke {
    std::string brushId;
    std::vector<Position> centers;
    BrushShape shape = BrushShape::Point;
    std::uint8_t radius = 0;
    std::uint64_t seed = 0;
    std::string requestId = "brush-stroke";
    CommandOrigin origin = CommandOrigin::Human;
    std::optional<std::uint64_t> expectedRevision;
    bool previewOnly = false;
};

struct BrushPlanResult {
    bool success = false;
    std::string message;
    std::size_t footprintTileCount = 0;
    MapCommandBatch batch;
};

class BrushEngine {
public:
    bool registerGroundBrush(GroundBrushDefinition definition, std::string* error = nullptr);

    [[nodiscard]] const GroundBrushDefinition* findGroundBrush(const std::string& id) const noexcept;
    [[nodiscard]] BrushPlanResult planGroundStroke(const MapDocument& document, const BrushStroke& stroke) const;
    [[nodiscard]] CommandResult executeGroundStroke(MapDocument& document, const BrushStroke& stroke) const;

private:
    [[nodiscard]] static std::vector<Position> buildFootprint(const BrushStroke& stroke);
    [[nodiscard]] static std::size_t selectVariant(
        const GroundBrushDefinition& definition,
        const Position& position,
        std::uint64_t seed) noexcept;

    std::unordered_map<std::string, GroundBrushDefinition> groundBrushes_;
};

} // namespace fantasy::studio::mapcore
