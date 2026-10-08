#include "MapEngine/Core/MapDocument.hpp"
#include "MapEngine/Import/LegacyMapProjectLoader.hpp"
#include "Shared/Assets/Legacy/DatReader.hpp"
#include "Shared/Assets/Legacy/OtbReader.hpp"
#include "Shared/Assets/Legacy/SprReader.hpp"
#include "Shared/Formats/Legacy/OtbmReader.hpp"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

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
              << "  options may be combined to inspect a matching legacy pack\n";
}

void printProjectReport(
    const fantasy::studio::mapcore::MapDocument& document,
    const fantasy::studio::mapcore::LegacyMapProjectLoadResult& result) {

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

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--project") projectMode = true;
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

        if (projectMode) {
            if (otbmPath.empty() || otbPath.empty() || datPath.empty() || sprPath.empty()) {
                throw std::runtime_error("--project requires --otbm, --otb, --dat and --spr");
            }

            fantasy::studio::mapcore::LegacyMapProjectConfig config;
            config.profileId = profileId;
            config.otbmPath = otbmPath;
            config.otbPath = otbPath;
            config.datPath = datPath;
            config.sprPath = sprPath;
            if (!housePath.empty()) config.houseXmlPath = housePath;
            if (!spawnPath.empty()) config.spawnXmlPath = spawnPath;

            fantasy::studio::mapcore::MapDocument document;
            const auto result = fantasy::studio::mapcore::LegacyMapProjectLoader{}.load(document, config);
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
                      << " sprites=" << spr.info().spriteCount << '\n';
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
