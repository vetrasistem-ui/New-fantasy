#include "Shared/Assets/Legacy/DatReader.hpp"
#include "Shared/Assets/Legacy/OtbReader.hpp"
#include "Shared/Assets/Legacy/SprReader.hpp"
#include "Shared/Assets/LegacyAssetRegistryBuilder.hpp"
#include "Shared/Formats/Legacy/OtbmStreamReader.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct Arguments {
    fs::path dat;
    fs::path spr;
    fs::path otb;
    fs::path otbm;
    fs::path output;
    std::string profile = "pokefans1098";
    std::size_t topGrounds = 80;
    std::size_t topObjects = 160;
};

void usage() {
    std::cout
        << "Fantasy real asset candidate scanner\n\n"
        << "Usage:\n"
        << "  fantasy-asset-candidates --dat <Tibia.dat> --spr <Tibia.spr> --otb <items.otb>\\\n"
        << "      --otbm <global_dash.otbm> --output <asset-candidates.json>\\\n"
        << "      [--profile pokefans1098] [--top-grounds 80] [--top-objects 160]\n\n"
        << "The scanner streams the real OTBM and ranks actually-used server IDs.\n"
        << "Each candidate includes serverId, clientId, kind, spriteIds and usage count.\n";
}

std::size_t parseSize(const std::string& value, const char* label) {
    try {
        const auto parsed = std::stoull(value);
        if (parsed == 0U) throw std::invalid_argument("zero");
        return static_cast<std::size_t>(parsed);
    } catch (...) {
        throw std::invalid_argument(std::string(label) + " must be a positive integer: " + value);
    }
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
        else if (key == "--otbm") args.otbm = value();
        else if (key == "--output") args.output = value();
        else if (key == "--profile") args.profile = value();
        else if (key == "--top-grounds") args.topGrounds = parseSize(value(), "top-grounds");
        else if (key == "--top-objects") args.topObjects = parseSize(value(), "top-objects");
        else if (key == "--help" || key == "-h") {
            usage();
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown argument: " + key);
        }
    }

    if (args.dat.empty() || args.spr.empty() || args.otb.empty() || args.otbm.empty() || args.output.empty()) {
        throw std::invalid_argument("--dat, --spr, --otb, --otbm and --output are required");
    }
    return args;
}

void countItemTree(const fantasy::legacy::LegacyImportedItem& item, std::unordered_map<std::uint32_t, std::uint64_t>& counts) {
    if (item.serverId != 0U) ++counts[item.serverId];
    for (const auto& child : item.contents) countItemTree(child, counts);
}

std::string kindId(fantasy::assets::LegacyAssetKind kind) {
    switch (kind) {
        case fantasy::assets::LegacyAssetKind::Ground: return "ground";
        case fantasy::assets::LegacyAssetKind::Border: return "border";
        case fantasy::assets::LegacyAssetKind::Object: return "object";
        case fantasy::assets::LegacyAssetKind::Creature: return "creature";
        case fantasy::assets::LegacyAssetKind::Effect: return "effect";
        default: return "unknown";
    }
}

nlohmann::json ranked(
    const std::unordered_map<std::uint32_t, std::uint64_t>& counts,
    const fantasy::assets::FantasyAssetRegistry& registry,
    const fantasy::assets::legacy::SprReader& spr,
    std::size_t limit) {

    std::vector<std::pair<std::uint32_t, std::uint64_t>> values(counts.begin(), counts.end());
    std::sort(values.begin(), values.end(), [](const auto& left, const auto& right) {
        if (left.second != right.second) return left.second > right.second;
        return left.first < right.first;
    });
    if (values.size() > limit) values.resize(limit);

    nlohmann::json output = nlohmann::json::array();
    for (const auto& [serverId, uses] : values) {
        nlohmann::json row{
            {"serverId", serverId},
            {"uses", uses},
        };
        if (const auto* record = registry.findByLegacyServerId(serverId); record != nullptr) {
            row["semanticKey"] = record->semanticKey;
            row["clientId"] = record->clientId;
            row["kind"] = kindId(record->kind);
            row["spriteIds"] = record->spriteIds;

            bool spritesValid = !record->spriteIds.empty();
            for (const auto spriteId : record->spriteIds) {
                if (spriteId == 0U || !spr.hasSprite(spriteId)) {
                    spritesValid = false;
                    break;
                }
            }
            row["spritesValid"] = spritesValid;
        } else {
            row["registryMissing"] = true;
        }
        output.push_back(std::move(row));
    }
    return output;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto args = parse(argc, argv);
        const fantasy::assets::legacy::DatReader1057 dat(args.dat);
        const fantasy::assets::legacy::SprReader spr(args.spr);
        const fantasy::assets::legacy::OtbReader otb(args.otb);
        const auto registry = fantasy::assets::LegacyAssetRegistryBuilder{}.build(args.profile, otb, dat);
        if (registry.registry.size() == 0U) throw std::runtime_error("real legacy registry is empty");

        std::unordered_map<std::uint32_t, std::uint64_t> grounds;
        std::unordered_map<std::uint32_t, std::uint64_t> objects;

        fantasy::legacy::OtbmStreamCallbacks callbacks;
        callbacks.onTile = [&](fantasy::legacy::LegacyImportedTile&& tile) {
            if (tile.groundServerId.has_value() && *tile.groundServerId != 0U) {
                ++grounds[*tile.groundServerId];
            }
            for (const auto& item : tile.items) countItemTree(item, objects);
        };

        const fantasy::legacy::OtbmStreamReader map(args.otbm, std::move(callbacks));

        nlohmann::json report{
            {"schemaVersion", 1},
            {"profileId", args.profile},
            {"source", {
                {"mapWidth", map.result().header.width},
                {"mapHeight", map.result().header.height},
                {"tiles", map.result().diagnostics.tileCount},
                {"items", map.result().diagnostics.itemCount},
                {"assetRegistry", registry.registry.size()},
                {"datSignature", dat.header().signature},
                {"sprSignature", spr.info().signature},
                {"sprCount", spr.info().spriteCount},
                {"otbMajor", otb.version().major},
                {"otbMinor", otb.version().minor},
            }},
            {"grounds", ranked(grounds, registry.registry, spr, args.topGrounds)},
            {"objects", ranked(objects, registry.registry, spr, args.topObjects)},
        };

        if (args.output.has_parent_path()) fs::create_directories(args.output.parent_path());
        std::ofstream out(args.output, std::ios::binary | std::ios::trunc);
        if (!out.good()) throw std::runtime_error("unable to create candidate report: " + args.output.string());
        out << report.dump(2) << '\n';
        if (!out.good()) throw std::runtime_error("failed while writing candidate report");

        std::cout << "REAL_ASSET_CANDIDATES PASS"
                  << " tiles=" << map.result().diagnostics.tileCount
                  << " unique_grounds=" << grounds.size()
                  << " unique_objects=" << objects.size()
                  << " output=\"" << args.output.string() << "\"\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "REAL_ASSET_CANDIDATES FAIL: " << error.what() << '\n';
        usage();
        return 2;
    }
}
