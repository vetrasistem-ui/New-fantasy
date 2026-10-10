#include "MapClipboard.hpp"

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
    Item value;
    value.serverId = serverId;
    value.clientId = clientId;
    return value;
}

Tile tile(Position position, std::uint32_t groundId, std::uint32_t objectId = 0) {
    Tile value;
    value.position = position;
    value.ground = item(groundId, groundId + 1000);
    if (objectId != 0) value.items.push_back(item(objectId, objectId + 1000));
    return value;
}

} // namespace

int main() {
    MapDocument document;

    Tile first = tile(Position{100, 200, 7}, 100, 200);
    first.houseId = 7;
    first.flags = 1;
    first.attributes["note"] = std::string("source");
    document.map().setTile(first);

    Tile second = tile(Position{102, 201, 7}, 101, 201);
    document.map().setTile(second);

    document.selection().add(first.position);
    document.selection().add(second.position);

    MapClipboard clipboard;
    std::string error;
    require(clipboard.capture(document, &error), "capture selected tiles");
    require(error.empty(), "capture has no error");
    require(clipboard.snapshot().has_value(), "clipboard snapshot exists");
    require(clipboard.snapshot()->tiles.size() == 2, "clipboard stores two tiles");
    require(clipboard.snapshot()->anchor == Position{100, 200, 7}, "clipboard anchor is minimum copied position");

    PasteOptions previewOptions;
    previewOptions.origin = CommandOrigin::AI;
    previewOptions.requestId = "ai-paste-preview";
    previewOptions.previewOnly = true;
    previewOptions.mode = PasteMode::Replace;
    previewOptions.expectedRevision = document.revision();

    const CommandResult preview = clipboard.paste(document, Position{300, 400, 7}, previewOptions);
    require(preview.status == CommandStatus::Preview, "paste preview succeeds");
    require(preview.affectedTiles == 2, "paste preview affects two tiles");
    require(document.map().findTile(Position{300, 400, 7}) == nullptr, "paste preview does not mutate map");
    require(document.map().findTile(Position{302, 401, 7}) == nullptr, "paste preview keeps relative offset without mutation");

    PasteOptions replaceOptions = previewOptions;
    replaceOptions.requestId = "ai-paste-apply";
    replaceOptions.previewOnly = false;
    const CommandResult paste = clipboard.paste(document, Position{300, 400, 7}, replaceOptions);
    require(paste.status == CommandStatus::Applied, "replace paste applies");
    require(document.revision() == 1, "entire paste increments revision once");

    const Tile* pastedFirst = document.map().findTile(Position{300, 400, 7});
    const Tile* pastedSecond = document.map().findTile(Position{302, 401, 7});
    require(pastedFirst && pastedSecond, "relative tile layout preserved");
    require(pastedFirst->ground->serverId == 100, "replace paste keeps ground");
    require(pastedFirst->items.size() == 1 && pastedFirst->items[0].serverId == 200, "replace paste keeps item stack");
    require(pastedFirst->houseId == 7 && pastedFirst->flags == 1, "replace paste preserves house and flags by default");

    require(document.undo(), "paste undo available");
    require(document.revision() == 2, "paste undo increments revision once");
    require(document.map().findTile(Position{300, 400, 7}) == nullptr, "one undo removes first pasted tile");
    require(document.map().findTile(Position{302, 401, 7}) == nullptr, "one undo removes second pasted tile");

    // Merge mode replaces source ground, appends source items and preserves
    // destination-only metadata when the caller disables house/flag transfer.
    Tile destination = tile(Position{500, 500, 7}, 900, 901);
    destination.houseId = 99;
    destination.flags = 8;
    destination.attributes["destinationOnly"] = std::string("yes");
    document.map().setTile(destination);

    PasteOptions mergeOptions;
    mergeOptions.origin = CommandOrigin::Human;
    mergeOptions.requestId = "merge-paste";
    mergeOptions.mode = PasteMode::Merge;
    mergeOptions.preserveHouseIds = false;
    mergeOptions.preserveTileFlags = false;
    mergeOptions.expectedRevision = document.revision();

    const CommandResult merge = clipboard.paste(document, Position{500, 500, 7}, mergeOptions);
    require(merge.status == CommandStatus::Applied, "merge paste applies");
    require(document.revision() == 3, "merge paste increments revision once");

    const Tile* merged = document.map().findTile(Position{500, 500, 7});
    require(merged != nullptr, "merged tile exists");
    require(merged->ground->serverId == 100, "source ground replaces destination ground in merge mode");
    require(merged->items.size() == 2, "merge appends source item to destination stack");
    require(merged->items[0].serverId == 901 && merged->items[1].serverId == 200, "merge stack order is deterministic");
    require(merged->houseId == 99, "merge can preserve destination house id");
    require(merged->flags == 8, "merge can preserve destination flags");
    require(merged->attributes.contains("destinationOnly") && merged->attributes.contains("note"), "merge preserves destination attrs and adds source attrs");

    // Cut captures the selection first and removes all selected tiles as one action.
    document.selection().clear();
    document.selection().add(Position{100, 200, 7});
    document.selection().add(Position{102, 201, 7});
    MapClipboard cutClipboard;
    const std::uint64_t beforeCutRevision = document.revision();
    const CommandResult cut = cutClipboard.cut(document, CommandOrigin::Human, "cut-two-tiles");
    require(cut.status == CommandStatus::Applied, "cut applies");
    require(document.revision() == beforeCutRevision + 1, "cut increments revision once");
    require(document.map().findTile(Position{100, 200, 7}) == nullptr, "cut removes first source tile");
    require(document.map().findTile(Position{102, 201, 7}) == nullptr, "cut removes second source tile");
    require(cutClipboard.snapshot()->tiles.size() == 2, "cut retains copied snapshot");

    require(document.undo(), "cut undo available");
    require(document.map().findTile(Position{100, 200, 7}) != nullptr, "one undo restores first cut tile");
    require(document.map().findTile(Position{102, 201, 7}) != nullptr, "one undo restores second cut tile");

    // Target translation is still subject to OTBM bounds validation.
    PasteOptions invalidOptions;
    invalidOptions.origin = CommandOrigin::AI;
    invalidOptions.requestId = "paste-out-of-bounds";
    invalidOptions.expectedRevision = document.revision();
    const CommandResult invalid = cutClipboard.paste(document, Position{-1, 20, 7}, invalidOptions);
    require(invalid.status == CommandStatus::Invalid, "out-of-bounds paste rejected");

    std::cout << "Fantasy Map Clipboard tests PASS\n";
    return 0;
}
