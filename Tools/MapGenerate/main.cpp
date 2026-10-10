#include "MapEngine/Core/MapDocument.hpp"
#include "MapEngine/Generation/FantasyMapCommandsV1.hpp"
#include "MapEngine/Generation/FantasyRealAssetBinding.hpp"
#include "Foundation/FantasyAssetCatalog.hpp"
#include "Shared/Assets/Legacy/DatReader.hpp"
#include "Shared/Assets/Legacy/OtbReader.hpp"
#include "Shared/Assets/Legacy/SprReader.hpp"
#include "Shared/Assets/LegacyAssetRegistryBuilder.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace {

struct Arguments {
    fs::path dat;
    fs::path spr;
    fs::path otb;
    fs::path bindings;
    fs::path script;
    fs::path outputRoot;
    fs::path catalogOut;
    std::string profile = "pokefans1098";
};

void usage() {
    std::cout
        << "Fantasy real-asset map generator\n\n"
        << "Usage:\n"
        << "  fantasy-map-generate --dat <Tibia.dat> [--spr <Tibia.spr>] --otb <items.otb>\\\n"
        << "      --bindings <real-asset-bindings.json> --script <map.fmapcmd>\\\n"
        << "      --output-root <project-dir> [--profile pokefans1098] [--catalog-out <asset-catalog.json>]\n\n"
        << "The bindings file maps semantic ids such as terrain.grass to REAL legacy serverIds.\n"
        << "The tool validates serverId -> clientId -> spriteIds against DAT/OTB before generation.\n"
        << "--spr is optional for structural generation; physical sprite validation requires SPR.\n"
        << "Without SPR the generated map is not visually validated.\n";
}

Arguments parse(int argc, char** argv) {
    Arguments args;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument("missing value after " + key);
            return argv[++i];
        };

        if (key == "--dat") args.dat = value();
        else if (key == "--spr") args.spr = value();
        else if (key == "--otb") args.otb = value();
        else if (key == "--bindings") args.bindings = value();
        else if (key == "--script") args.script = value();
        else if (key == "--output-root") args.outputRoot = value();
        else if (key == "--catalog-out") args.catalogOut = value();
        else if (key == "--profile") args.profile = value();
        else if (key == "--help" || key == "-h") {
            usage();
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown argument: " + key);
        }
    }

    if (args.dat.empty() || args.otb.empty() || args.bindings.empty() ||
        args.script.empty() || args.outputRoot.empty()) {
        throw std::invalid_argument("--dat, --otb, --bindings, --script and --output-root are required");
    }
    return args;
}

std::string readText(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.good()) throw std::runtime_error("unable to read map command script: " + path.string());
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto args = parse(argc, argv);

        const fantasy::assets::legacy::DatReader1057 dat(args.dat);
        std::unique_ptr<fantasy::assets::legacy::SprReader> spr;
        if (!args.spr.empty()) spr = std::make_unique<fantasy::assets::legacy::SprReader>(args.spr);
        else std::cout << "SPR_VALIDATION SKIPPED reason=no_spr_supplied\n";
        const fantasy::assets::legacy::OtbReader otb(args.otb);

        const auto registryResult = fantasy::assets::LegacyAssetRegistryBuilder{}.build(args.profile, otb, dat);
        if (registryResult.registry.size() == 0U) {
            throw std::runtime_error("real legacy registry is empty");
        }

        const auto manifest = fantasy::studio::mapgen::RealAssetBindingManifestStore::load(args.bindings);
        if (manifest.profileId != args.profile) {
            throw std::runtime_error(
                "binding manifest profileId does not match --profile: " + manifest.profileId + " != " + args.profile);
        }

        const auto binding = fantasy::studio::mapgen::FantasyRealAssetBinder::bind(
            manifest, registryResult.registry, spr.get());

        std::cout << "REAL_ASSET_REGISTRY profile=" << args.profile
                  << " registered=" << registryResult.registry.size()
                  << " dat_signature=0x" << std::hex << dat.header().signature
                  << std::dec;
        if (spr) std::cout << " spr_signature=0x" << std::hex << spr->info().signature << std::dec
                          << " spr_count=" << spr->info().spriteCount;
        std::cout
                  << " otb=" << otb.version().major << '.' << otb.version().minor
                  << '\n';

        for (const auto& message : binding.report.messages) {
            std::cout << "BIND " << message << '\n';
        }
        if (!binding.report.success) {
            for (const auto& error : binding.report.errors) std::cerr << "BIND_ERROR " << error << '\n';
            return 3;
        }

        if (!args.catalogOut.empty()) {
            fantasy::studio::foundation::FantasyAssetCatalogStore::save(args.catalogOut, binding.catalog);
            std::cout << "CATALOG saved=\"" << args.catalogOut.string() << "\"\n";
        }

        fantasy::studio::mapcore::MapDocument document;
        const auto report = fantasy::studio::mapgen::FantasyMapCommandsV1{}.execute(
            document,
            readText(args.script),
            binding.catalog,
            registryResult.registry,
            args.outputRoot);

        for (const auto& message : report.messages) std::cout << "MAP " << message << '\n';
        if (!report.success) return 4;

        std::cout << "MAP_GENERATION PASS commands=" << report.commandsExecuted
                  << " affected_tiles=" << report.affectedTiles;
        if (report.savedPath.has_value()) {
            std::cout << " saved=\"" << report.savedPath->string() << "\"";
        }
        std::cout << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "MAP_GENERATION FAIL: " << error.what() << '\n';
        usage();
        return 2;
    }
}
