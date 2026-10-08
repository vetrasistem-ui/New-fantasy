#include "MapEngine/Brushes/BrushEngine.hpp"
#include "MapEngine/Clipboard/MapClipboard.hpp"
#include "MapEngine/Core/MapDocument.hpp"
#include "MapEngine/Import/LegacyMapProjectLoader.hpp"
#include "MapEngine/Import/LegacyWorkspaceSession.hpp"
#include "Shared/Assets/Legacy/DatReader.hpp"
#include "Shared/Assets/Legacy/OtbReader.hpp"
#include "Shared/Assets/Legacy/SprReader.hpp"
#include "Shared/Formats/Legacy/OtbmReader.hpp"

#include <array>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace fs = std::filesystem;
using namespace fantasy::studio::mapcore;

namespace {

void printUsage() {
    std::cout << "Usage:\n"
              << "  fantasy-legacy-inspect --dat <Tibia.dat>\n"
              << "  fantasy-legacy-inspect --dat <Tibia.dat> --item <client-id>\n"
              << "  fantasy-legacy-inspect --spr <file.spr>\n"
              << "  fantasy-legacy-inspect --spr <file.spr> --sprite <id>\n"
              << "  fantasy-legacy-inspect --otb <items.otb>\n"
              << "  fantasy-legacy-inspect --otbm <map.otbm>\n"
              << "  fantasy-legacy-inspect --project --otbm <map.otbm> --otb <items.otb> --dat <Tibia.dat> --spr <Tibia.spr>\n"
              << "      [--house <map-house.xml>] [--spawn <map-spawn.xml>] [--profile <id>]\n"
              << "      [--edit-smoke]\n"
              << "  options may be combined to inspect a matching legacy pack\n";
}

void printProjectReport(
    const MapDocument& document,
    const LegacyMapProjectLoadResult& result) {

    const auto& report = result.report;
    std::cout << "PROJECT success=" << (report.success ? "yes" : "no")
              << " profile=\"" << document.metadata().sourceProfileId << "\""
              << " map=\"" << document.metadata().name << "\""
              << " size=" << document.metadata().width << 'x' << document.metadata().height
              << " canonical_tiles=" << document.map().tileCount()
              << " houses=" << document.map().houses().size()
              << " spawn_areas=" << document.map().spawnAreas().size()
              << " towns=" << document.map().towns().size()
              << " waypoints=" << document.map().waypoints().size()
              << " asset_registry=" << result.assets.size() << '\n';

    std::cout << "PROJECT dat_signature=0x" << std::hex << report.datSignature
              << " spr_signature=0x" << report.sprSignature << std::dec
              << " spr_count=" << report.sprCount
              << " warnings=" << report.warnings.size()
              << " errors=" << report.errors.size() << '\n';

    for (const auto& warning : report.warnings) std::cout << "WARN " << warning << '\n';
    for (const auto& error : report.errors) std::cout << "ERROR " << error << '\n';
}

struct EditCandidate {
    Position position;
    Tile tile;
};

std::pair<EditCandidate, EditCandidate> findGroundCandidates(
    const MapDocument& document,
    const Position& center) {

    const auto& map = document.map();
    const std::array<std::int32_t, 7> radii{8, 16, 32, 64, 128, 256, 512};

    for (const std::int32_t radius : radii) {
        std::optional<EditCandidate> first;
        std::optional<EditCandidate> different;
        map.forEachTileInRect(
            center.z,
            MapStorage::Rect{center.x - radius, center.y - radius, center.x + radius, center.y + radius},
            [&](const Tile& tile) {
                if (different.has_value() || !tile.ground.has_value() || tile.houseId != 0) return;
                if (!first.has_value()) {
                    first = EditCandidate{tile.position, tile};
                    return;
                }
                if (tile.position != first->position &&
                    tile.ground->serverId != first->tile.ground->serverId) {
                    different = EditCandidate{tile.position, tile};
                }
            });

        if (first.has_value() && different.has_value()) {
            return {*first, *different};
        }
    }

    throw std::runtime_error("Unable to find two nearby non-house ground tiles with different server ids");
}

void requireEdit(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("EDIT_SMOKE failed: " + message);
}

int runEditSmoke(const LegacyMapProjectConfig& config) {
    LegacyWorkspaceSession session;
    requireEdit(session.open(config), "real project did not load");

    MapDocument& document = session.document();
    const auto [target, alternate] = findGroundCandidates(document, session.view().center);
    requireEdit(target.tile.ground.has_value(), "target has no ground");
    requireEdit(alternate.tile.ground.has_value(), "alternate has no ground");

    const Tile originalTarget = target.tile;
    const Tile originalAlternate = alternate.tile;
    const std::uint64_t initialRevision = document.revision();

    document.selection().selectOnly(target.position);
    requireEdit(document.selection().size() == 1U, "selection size is not one");
    requireEdit(document.selection().contains(target.position), "selected real tile is not present in selection");

    BrushEngine brushes;
    GroundBrushDefinition brush;
    brush.id = "real-edit-smoke-ground";
    brush.variants.push_back(WeightedBrushItem{*alternate.tile.ground, 1U});
    brush.maxRadius = 0;
    brush.canDrag = false;
    std::string brushError;
    requireEdit(brushes.registerGroundBrush(std::move(brush), &brushError), "brush registration: " + brushError);

    BrushStroke previewStroke;
    previewStroke.brushId = "real-edit-smoke-ground";
    previewStroke.centers = {target.position};
    previewStroke.shape = BrushShape::Point;
    previewStroke.radius = 0;
    previewStroke.seed = 1098U;
    previewStroke.requestId = "real-edit-preview";
    previewStroke.expectedRevision = document.revision();
    previewStroke.previewOnly = true;

    const CommandResult preview = brushes.executeGroundStroke(document, previewStroke);
    requireEdit(preview.status == CommandStatus::Preview, "ground brush preview did not return Preview");
    requireEdit(document.revision() == initialRevision, "preview changed document revision");
    requireEdit(*document.map().findTile(target.position) == originalTarget, "preview mutated target tile");

    BrushStroke applyStroke = previewStroke;
    applyStroke.previewOnly = false;
    applyStroke.requestId = "real-edit-apply";
    applyStroke.expectedRevision = document.revision();
    const CommandResult applied = brushes.executeGroundStroke(document, applyStroke);
    requireEdit(applied.status == CommandStatus::Applied, "ground brush apply did not return Applied");
    const Tile* painted = document.map().findTile(target.position);
    requireEdit(painted != nullptr && painted->ground.has_value(), "painted tile disappeared");
    requireEdit(
        painted->ground->serverId == alternate.tile.ground->serverId,
        "painted ground does not match alternate real ground");

    requireEdit(document.undo(), "brush undo returned false");
    requireEdit(*document.map().findTile(target.position) == originalTarget, "brush undo did not restore original tile");
    requireEdit(document.redo(), "brush redo returned false");
    painted = document.map().findTile(target.position);
    requireEdit(
        painted != nullptr && painted->ground.has_value() && painted->ground->serverId == alternate.tile.ground->serverId,
        "brush redo did not reapply ground");
    requireEdit(document.undo(), "second brush undo returned false");
    requireEdit(*document.map().findTile(target.position) == originalTarget, "second brush undo did not restore original tile");

    MapClipboard clipboard;
    std::string clipboardError;
    requireEdit(clipboard.capture(document, &clipboardError), "clipboard capture: " + clipboardError);

    PasteOptions pasteOptions;
    pasteOptions.mode = PasteMode::Replace;
    pasteOptions.expectedRevision = document.revision();
    pasteOptions.requestId = "real-edit-paste";
    const CommandResult pasted = clipboard.paste(document, alternate.position, pasteOptions);
    requireEdit(pasted.status == CommandStatus::Applied, "clipboard paste did not return Applied");

    Tile expectedPaste = originalTarget;
    expectedPaste.position = alternate.position;
    requireEdit(*document.map().findTile(alternate.position) == expectedPaste, "clipboard paste did not replace target tile exactly");
    requireEdit(document.undo(), "clipboard paste undo returned false");
    requireEdit(*document.map().findTile(alternate.position) == originalAlternate, "clipboard paste undo did not restore target");
    requireEdit(document.redo(), "clipboard paste redo returned false");
    requireEdit(*document.map().findTile(alternate.position) == expectedPaste, "clipboard paste redo did not restore pasted tile");
    requireEdit(document.undo(), "second clipboard paste undo returned false");
    requireEdit(*document.map().findTile(alternate.position) == originalAlternate, "second paste undo did not restore target");

    document.selection().selectOnly(target.position);
    const CommandResult cut = clipboard.cut(document, CommandOrigin::Human, "real-edit-cut", false);
    requireEdit(cut.status == CommandStatus::Applied, "clipboard cut did not return Applied");
    requireEdit(document.map().findTile(target.position) == nullptr, "cut did not remove selected tile");
    requireEdit(document.undo(), "clipboard cut undo returned false");
    requireEdit(*document.map().findTile(target.position) == originalTarget, "cut undo did not restore source tile");

    std::cout << "EDIT_SMOKE PASS"
              << " center=" << session.view().center.x << ',' << session.view().center.y << ',' << session.view().center.z
              << " target=" << target.position.x << ',' << target.position.y << ',' << target.position.z
              << " ground_from=" << originalTarget.ground->serverId
              << " ground_to=" << originalAlternate.ground->serverId
              << " paste_target=" << alternate.position.x << ',' << alternate.position.y << ',' << alternate.position.z
              << " initial_revision=" << initialRevision
              << " final_revision=" << document.revision()
              << " tiles=" << document.map().tileCount() << '\n';
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            printUsage();
            return 2;
        }

        fs::path datPath;
        fs::path sprPath;
        fs::path otbPath;
        fs::path otbmPath;
        fs::path housePath;
        fs::path spawnPath;
        std::string profileId = "pokefans1098";
        std::uint32_t itemId = 0;
        std::uint32_t spriteId = 0;
        bool projectMode = false;
        bool editSmoke = false;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--project") projectMode = true;
            else if (arg == "--edit-smoke") editSmoke = true;
            else if (arg == "--dat" && i + 1 < argc) datPath = argv[++i];
            else if (arg == "--spr" && i + 1 < argc) sprPath = argv[++i];
            else if (arg == "--otb" && i + 1 < argc) otbPath = argv[++i];
            else if (arg == "--otbm" && i + 1 < argc) otbmPath = argv[++i];
            else if (arg == "--house" && i + 1 < argc) housePath = argv[++i];
            else if (arg == "--spawn" && i + 1 < argc) spawnPath = argv[++i];
            else if (arg == "--profile" && i + 1 < argc) profileId = argv[++i];
            else if (arg == "--item" && i + 1 < argc) itemId = static_cast<std::uint32_t>(std::stoul(argv[++i]));
            else if (arg == "--sprite" && i + 1 < argc) spriteId = static_cast<std::uint32_t>(std::stoul(argv[++i]));
            else throw std::runtime_error("Unknown or incomplete argument: " + arg);
        }

        if (editSmoke) projectMode = true;
        if (projectMode) {
            if (otbmPath.empty() || otbPath.empty() || datPath.empty() || sprPath.empty()) {
                throw std::runtime_error("--project requires --otbm, --otb, --dat and --spr");
            }

            LegacyMapProjectConfig config;
            config.profileId = profileId;
            config.otbmPath = otbmPath;
            config.otbPath = otbPath;
            config.datPath = datPath;
            config.sprPath = sprPath;
            if (!housePath.empty()) config.houseXmlPath = housePath;
            if (!spawnPath.empty()) config.spawnXmlPath = spawnPath;

            if (editSmoke) return runEditSmoke(config);

            MapDocument document;
            const auto result = LegacyMapProjectLoader{}.load(document, config);
            printProjectReport(document, result);
            return result.report.success ? 0 : 1;
        }

        if (datPath.empty() && sprPath.empty() && otbPath.empty() && otbmPath.empty()) {
            printUsage(); return 2;
        }
        if (itemId != 0 && datPath.empty()) throw std::runtime_error("--item requires --dat");
        if (spriteId != 0 && sprPath.empty()) throw std::runtime_error("--sprite requires --spr");

        if (!datPath.empty()) {
            const fantasy::assets::legacy::DatReader1057 dat(datPath);
            const auto& h = dat.header();
            std::cout << "DAT signature=0x" << std::hex << h.signature << std::dec
                      << " item_max_id=" << h.itemMaxId << " creatures=" << h.creatureCount
                      << " effects=" << h.effectCount << " distance_effects=" << h.distanceCount
                      << " parsed_item_creature_appearances=" << dat.appearances().size() << '\n';
            if (itemId != 0) {
                const auto* item = dat.findItem(itemId);
                if (!item) std::cout << "item=" << itemId << " present=no\n";
                else {
                    std::size_t refs = 0; for (const auto& g : item->frameGroups) refs += g.spriteIds.size();
                    std::cout << "item=" << itemId << " present=yes raw_flags=" << item->rawFlags.size()
                              << " frame_groups=" << item->frameGroups.size() << " sprite_refs=" << refs << '\n';
                }
            }
        }

        if (!sprPath.empty()) {
            const fantasy::assets::legacy::SprReader spr(sprPath);
            std::cout << "SPR signature=0x" << std::hex << spr.info().signature << std::dec
                      << " sprites=" << spr.info().spriteCount
                      << " channels=" << static_cast<unsigned>(spr.info().colorChannels) << '\n';
            if (spriteId != 0) {
                const auto sprite = spr.readSprite(spriteId);
                std::size_t opaque = 0; for (std::size_t i = 3; i < sprite.pixels.size(); i += 4) if (sprite.pixels[i] != 0) ++opaque;
                std::cout << "sprite=" << spriteId << " present=" << (spr.hasSprite(spriteId) ? "yes" : "no")
                          << " opaque_pixels=" << opaque << '\n';
            }
        }

        if (!otbPath.empty()) {
            const fantasy::assets::legacy::OtbReader otb(otbPath);
            std::size_t mapped = 0; for (const auto& item : otb.items()) if (item.serverId && item.clientId) ++mapped;
            std::cout << "OTB version=" << otb.version().major << '.' << otb.version().minor << '.' << otb.version().build
                      << " description=\"" << otb.version().description << "\" nodes=" << otb.items().size()
                      << " mapped_server_client=" << mapped << '\n';
        }

        if (!otbmPath.empty()) {
            const fantasy::legacy::OtbmReader otbm(otbmPath);
            const auto& r = otbm.result();
            std::cout << "OTBM format=" << r.header.formatVersion
                      << " size=" << r.header.width << 'x' << r.header.height
                      << " items_version=" << r.header.itemsMajorVersion << '.' << r.header.itemsMinorVersion
                      << " tiles=" << r.import.diagnostics.tileCount
                      << " items=" << r.import.diagnostics.itemCount
                      << " towns=" << r.import.diagnostics.townCount
                      << " house_tiles=";
            std::size_t houseTiles = 0; for (const auto& tile : r.import.model.tiles) if (tile.houseId) ++houseTiles;
            std::cout << houseTiles << " warnings=" << r.import.diagnostics.warnings.size() << '\n';
            std::cout << "OTBM description=\"" << r.metadata.description << "\" spawn=\"" << r.metadata.spawnFile
                      << "\" houses=\"" << r.metadata.houseFile << "\"\n";
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Fantasy legacy inspect error: " << error.what() << '\n';
        return 1;
    }
}
