#include "BrushEngine.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>

using namespace fantasy::studio::mapcore;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

Item item(std::uint32_t serverId, std::uint32_t clientId = 0) {
    Item result;
    result.serverId = serverId;
    result.clientId = clientId;
    return result;
}

bool hasItem(const Tile& tile, std::uint32_t serverId) {
    return std::any_of(tile.items.begin(), tile.items.end(), [&](const Item& candidate) {
        return candidate.serverId == serverId;
    });
}

AutoBorderDefinition makeBorder() {
    AutoBorderDefinition border;
    border.id = "grass-edge";
    border.pieces = {
        {BorderPieceKind::North, item(2001)},
        {BorderPieceKind::East, item(2002)},
        {BorderPieceKind::South, item(2003)},
        {BorderPieceKind::West, item(2004)},
        {BorderPieceKind::CornerNorthWest, item(2005)},
        {BorderPieceKind::CornerNorthEast, item(2006)},
        {BorderPieceKind::CornerSouthEast, item(2007)},
        {BorderPieceKind::CornerSouthWest, item(2008)},
        {BorderPieceKind::DiagonalNorthWest, item(2009)},
        {BorderPieceKind::DiagonalNorthEast, item(2010)},
        {BorderPieceKind::DiagonalSouthEast, item(2011)},
        {BorderPieceKind::DiagonalSouthWest, item(2012)},
    };
    return border;
}

BrushEngine makeEngine() {
    BrushEngine engine;
    std::string error;
    require(engine.registerAutoBorder(makeBorder(), &error), "register auto-border");

    GroundBrushDefinition grass;
    grass.id = "grass";
    grass.variants = {{item(100, 1100), 100}, {item(101, 1101), 0}};
    grass.outerBorderId = "grass-edge";
    grass.maxRadius = 8;
    require(engine.registerGroundBrush(std::move(grass), &error), "register grass brush");
    return engine;
}

BrushStroke pointStroke(Position position, bool preview = false) {
    BrushStroke stroke;
    stroke.brushId = "grass";
    stroke.centers = {position};
    stroke.shape = BrushShape::Point;
    stroke.requestId = preview ? "brush-preview" : "brush-apply";
    stroke.expectedRevision = 0;
    stroke.previewOnly = preview;
    return stroke;
}

} // namespace

int main() {
    BrushEngine engine = makeEngine();
    MapDocument document;

    BrushStroke preview = pointStroke(Position{10, 10, 7}, true);
    const BrushPlanResult previewPlan = engine.planGroundStroke(document, preview);
    require(previewPlan.success, "point brush preview plan");
    require(previewPlan.footprintTileCount == 1, "point footprint size");
    require(!previewPlan.batch.commands.empty(), "point brush emits commands");

    const CommandResult previewResult = engine.executeGroundStroke(document, preview);
    require(previewResult.status == CommandStatus::Preview, "brush preview status");
    require(document.map().findTile(Position{10, 10, 7}) == nullptr, "brush preview does not mutate map");
    require(document.revision() == 0, "brush preview does not change revision");

    BrushStroke apply = preview;
    apply.requestId = "brush-apply-1";
    apply.previewOnly = false;
    const CommandResult applyResult = engine.executeGroundStroke(document, apply);
    require(applyResult.status == CommandStatus::Applied, "point brush applied");
    require(document.revision() == 1, "point brush increments revision once");

    const Tile* isolated = document.map().findTile(Position{10, 10, 7});
    require(isolated && isolated->ground.has_value(), "isolated painted tile exists");
    require(isolated->ground->serverId == 100, "zero-weight ground variant is not selected");
    require(isolated->items.size() == 4, "isolated tile receives four outer edges");
    require(hasItem(*isolated, 2001), "isolated tile north edge");
    require(hasItem(*isolated, 2002), "isolated tile east edge");
    require(hasItem(*isolated, 2003), "isolated tile south edge");
    require(hasItem(*isolated, 2004), "isolated tile west edge");

    BrushStroke extend = pointStroke(Position{11, 10, 7}, false);
    extend.requestId = "brush-apply-2";
    extend.expectedRevision = document.revision();
    const CommandResult extendResult = engine.executeGroundStroke(document, extend);
    require(extendResult.status == CommandStatus::Applied, "adjacent brush applied");
    require(document.revision() == 2, "adjacent brush increments revision once");

    const Tile* left = document.map().findTile(Position{10, 10, 7});
    const Tile* right = document.map().findTile(Position{11, 10, 7});
    require(left && right, "adjacent painted tiles exist");
    require(!hasItem(*left, 2002), "old east edge removed after neighbor paint");
    require(!hasItem(*right, 2004), "new tile has no west edge against friendly ground");
    require(left->items.size() == 3 && right->items.size() == 3, "adjacent tiles have three exposed edges each");

    require(document.undo(), "undo adjacent brush as one action");
    require(document.revision() == 3, "brush undo increments revision once");
    require(document.map().findTile(Position{11, 10, 7}) == nullptr, "brush undo removes adjacent tile");
    isolated = document.map().findTile(Position{10, 10, 7});
    require(isolated && isolated->items.size() == 4, "brush undo restores old auto-borders");

    MapDocument squareDocument;
    BrushStroke square;
    square.brushId = "grass";
    square.centers = {Position{50, 50, 7}};
    square.shape = BrushShape::Square;
    square.radius = 1;
    square.requestId = "brush-square";
    square.expectedRevision = squareDocument.revision();

    const CommandResult squareResult = engine.executeGroundStroke(squareDocument, square);
    require(squareResult.status == CommandStatus::Applied, "square brush applied");
    require(squareDocument.map().tileCount() == 9, "square brush paints 3x3 footprint");

    const Tile* center = squareDocument.map().findTile(Position{50, 50, 7});
    const Tile* northWest = squareDocument.map().findTile(Position{49, 49, 7});
    const Tile* north = squareDocument.map().findTile(Position{50, 49, 7});
    require(center && center->items.empty(), "interior tile has no auto-border");
    require(northWest && northWest->items.size() == 1 && hasItem(*northWest, 2009), "outer corner uses northwest diagonal piece");
    require(north && north->items.size() == 1 && hasItem(*north, 2001), "top edge uses north horizontal piece");

    std::cout << "Fantasy Brush Engine tests PASS\n";
    return 0;
}
