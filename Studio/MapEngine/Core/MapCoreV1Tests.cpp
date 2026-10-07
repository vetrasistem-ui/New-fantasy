#include "MapDocument.hpp"
#include "SelectionModel.hpp"
#include "../Commands/CommandExecutor.hpp"
#include "../Query/MapQueryService.hpp"

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

} // namespace

int main() {
    MapDocument document;

    // Sparse/chunked storage must support normal and negative coordinates internally.
    document.map().ensureTile(Position{10, 20, 7}).ground = item(100, 200);
    document.map().ensureTile(Position{-1, -65, 7}).ground = item(101, 201);
    require(document.map().tileCount() == 2, "chunked storage tile count");
    require(document.map().findTile(Position{-1, -65, 7}) != nullptr, "negative coordinate lookup");

    std::size_t visible = 0;
    document.map().forEachTileInRect(7, MapStorage::Rect{0, 0, 64, 64}, [&](const Tile&) { ++visible; });
    require(visible == 1, "viewport query should visit only intersecting tile");

    // Selection is engine state, not a UI-only optional tile.
    document.selection().selectRect(Position{100, 100, 7}, Position{102, 101, 7});
    require(document.selection().size() == 6, "rect selection");
    document.selection().toggle(Position{101, 100, 7});
    require(document.selection().size() == 5, "selection toggle");

    CommandExecutor executor;

    MapCommand preview;
    preview.requestId = "ai-preview-001";
    preview.origin = CommandOrigin::AI;
    preview.expectedRevision = document.revision();
    preview.previewOnly = true;
    preview.payload = PaintGroundCommand{{Position{300, 400, 7}, Position{301, 400, 7}}, item(4526, 7812)};

    const CommandResult previewResult = executor.execute(document, preview);
    require(previewResult.status == CommandStatus::Preview, "AI preview status");
    require(previewResult.affectedTiles == 2, "AI preview affected tiles");
    require(document.map().findTile(Position{300, 400, 7}) == nullptr, "preview must not mutate map");

    MapCommand apply = preview;
    apply.requestId = "ai-apply-001";
    apply.previewOnly = false;
    const CommandResult applyResult = executor.execute(document, apply);
    require(applyResult.status == CommandStatus::Applied, "AI command apply");
    require(document.revision() == 1, "revision increments on apply");
    require(document.map().findTile(Position{300, 400, 7}) != nullptr, "applied command mutates map");

    // AI-facing query surface must be bounded and revision-aware.
    MapQueryService queries;
    const MapSummary summary = queries.summarize(document);
    require(summary.revision == 1, "query summary revision");
    require(summary.tileCount == 4, "query summary tile count");
    require(summary.selectedTileCount == 5, "query summary selection count");

    const RegionSnapshot region = queries.getRegion(document, 7, MapStorage::Rect{299, 399, 302, 401}, 8);
    require(region.revision == 1, "region snapshot revision");
    require(region.tiles.size() == 2, "region query tile count");
    require(!region.truncated, "region query not truncated");

    const RegionSnapshot bounded = queries.getRegion(document, 7, MapStorage::Rect{299, 399, 302, 401}, 1);
    require(bounded.tiles.size() == 1, "bounded region result size");
    require(bounded.truncated, "bounded region reports truncation");

    const auto matches = queries.findItemsByServerId(document, 4526, 7, MapStorage::Rect{299, 399, 302, 401});
    require(matches.size() == 2, "server id query finds both painted grounds");
    require(matches.front().ground, "server id query marks ground match");

    // Commands that cannot be represented by the 10.98 OTBM target are rejected.
    MapCommand invalidPosition = apply;
    invalidPosition.requestId = "ai-invalid-position";
    invalidPosition.expectedRevision = document.revision();
    invalidPosition.payload = PaintGroundCommand{{Position{-1, 500, 7}}, item(4526, 7812)};
    const CommandResult invalidPositionResult = executor.execute(document, invalidPosition);
    require(invalidPositionResult.status == CommandStatus::Invalid, "negative OTBM coordinate rejected");
    require(document.revision() == 1, "invalid command does not change revision");

    MapCommand unresolvedItem;
    unresolvedItem.requestId = "ai-unresolved-item";
    unresolvedItem.origin = CommandOrigin::AI;
    unresolvedItem.expectedRevision = document.revision();
    unresolvedItem.payload = PlaceItemCommand{Position{302, 400, 7}, item(0), std::nullopt};
    const CommandResult unresolvedItemResult = executor.execute(document, unresolvedItem);
    require(unresolvedItemResult.status == CommandStatus::Invalid, "unresolved item rejected");
    require(document.map().findTile(Position{302, 400, 7}) == nullptr, "invalid item does not mutate map");

    MapCommand stale = apply;
    stale.requestId = "ai-stale-001";
    stale.expectedRevision = 0;
    stale.payload = EraseTileCommand{{Position{300, 400, 7}}};
    const CommandResult staleResult = executor.execute(document, stale);
    require(staleResult.status == CommandStatus::RevisionConflict, "stale AI context must be rejected");
    require(document.map().findTile(Position{300, 400, 7}) != nullptr, "revision conflict must not mutate map");

    require(document.undo(), "undo applied command");
    require(document.revision() == 2, "revision increments on undo");
    require(document.map().findTile(Position{300, 400, 7}) == nullptr, "undo restores pre-command state");

    require(document.redo(), "redo applied command");
    require(document.revision() == 3, "revision increments on redo");
    require(document.map().findTile(Position{301, 400, 7}) != nullptr, "redo restores command state");

    std::cout << "Fantasy Map Core V1 tests PASS\n";
    return 0;
}
