#pragma once

#include "../Commands/CommandExecutor.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
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

enum class BorderPieceKind {
    North,
    East,
    South,
    West,
    CornerNorthWest,
    CornerNorthEast,
    CornerSouthEast,
    CornerSouthWest,
    DiagonalNorthWest,
    DiagonalNorthEast,
    DiagonalSouthEast,
    DiagonalSouthWest,
};

struct AutoBorderDefinition {
    std::string id;
    std::map<BorderPieceKind, Item> pieces;
};

struct GroundBrushDefinition {
    std::string id;
    std::vector<WeightedBrushItem> variants;
    std::vector<std::string> friends;
    std::optional<std::string> outerBorderId;
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

enum class WallPieceKind {
    Horizontal,
    Vertical,
    Corner,
    Pole,
};

struct WallBrushDefinition {
    std::string id;
    std::map<WallPieceKind, std::vector<WeightedBrushItem>> pieces;
    std::vector<std::string> friends;
    bool canDrag = true;
};

struct WallStroke {
    std::string brushId;
    std::vector<Position> centers;
    std::uint64_t seed = 0;
    std::string requestId = "wall-stroke";
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
    bool registerAutoBorder(AutoBorderDefinition definition, std::string* error = nullptr);
    bool registerGroundBrush(GroundBrushDefinition definition, std::string* error = nullptr);
    bool registerWallBrush(WallBrushDefinition definition, std::string* error = nullptr);

    [[nodiscard]] const AutoBorderDefinition* findAutoBorder(const std::string& id) const noexcept;
    [[nodiscard]] const GroundBrushDefinition* findGroundBrush(const std::string& id) const noexcept;
    [[nodiscard]] const WallBrushDefinition* findWallBrush(const std::string& id) const noexcept;

    [[nodiscard]] BrushPlanResult planGroundStroke(const MapDocument& document, const BrushStroke& stroke) const;
    [[nodiscard]] CommandResult executeGroundStroke(MapDocument& document, const BrushStroke& stroke) const;
    [[nodiscard]] BrushPlanResult planWallStroke(const MapDocument& document, const WallStroke& stroke) const;
    [[nodiscard]] CommandResult executeWallStroke(MapDocument& document, const WallStroke& stroke) const;

private:
    [[nodiscard]] static std::vector<Position> buildFootprint(const BrushStroke& stroke);
    [[nodiscard]] static std::vector<Position> buildWallPath(const WallStroke& stroke);
    [[nodiscard]] static std::size_t selectVariant(
        const GroundBrushDefinition& definition,
        const Position& position,
        std::uint64_t seed) noexcept;
    [[nodiscard]] static std::size_t selectWeighted(
        const std::vector<WeightedBrushItem>& variants,
        const Position& position,
        std::uint64_t seed) noexcept;

    [[nodiscard]] const GroundBrushDefinition* brushForGround(const std::optional<Item>& ground) const noexcept;
    [[nodiscard]] bool areFriendly(const GroundBrushDefinition& owner, const GroundBrushDefinition* other) const noexcept;
    [[nodiscard]] std::vector<BorderPieceKind> selectBorderPieces(
        const MapDocument& document,
        const GroundBrushDefinition& owner,
        const Position& position,
        const std::map<Position, Item>& groundOverrides) const;
    [[nodiscard]] std::optional<Item> effectiveGround(
        const MapDocument& document,
        const Position& position,
        const std::map<Position, Item>& groundOverrides) const;

    [[nodiscard]] const WallBrushDefinition* wallBrushAt(
        const MapDocument& document,
        const Position& position,
        const std::map<Position, std::string>& wallOverrides) const noexcept;
    [[nodiscard]] bool wallFriends(const WallBrushDefinition& owner, const WallBrushDefinition* other) const noexcept;
    [[nodiscard]] WallPieceKind chooseWallPiece(
        const MapDocument& document,
        const WallBrushDefinition& owner,
        const Position& position,
        const std::map<Position, std::string>& wallOverrides) const;
    [[nodiscard]] std::optional<Item> chooseWallItem(
        const WallBrushDefinition& definition,
        WallPieceKind kind,
        const Position& position,
        std::uint64_t seed) const;

    void removeManagedBorders(Tile& tile) const;
    void appendOuterBorders(
        Tile& tile,
        const GroundBrushDefinition& owner,
        const std::vector<BorderPieceKind>& pieces) const;
    void removeManagedWalls(Tile& tile) const;

    std::unordered_map<std::string, AutoBorderDefinition> autoBorders_;
    std::unordered_map<std::string, GroundBrushDefinition> groundBrushes_;
    std::unordered_map<std::uint32_t, std::string> groundServerToBrush_;
    std::set<std::uint32_t> managedBorderServerIds_;

    std::unordered_map<std::string, WallBrushDefinition> wallBrushes_;
    std::unordered_map<std::uint32_t, std::string> wallServerToBrush_;
    std::unordered_map<std::uint32_t, WallPieceKind> wallServerToPiece_;
    std::set<std::uint32_t> managedWallServerIds_;
};

} // namespace fantasy::studio::mapcore
