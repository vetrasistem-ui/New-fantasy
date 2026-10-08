#include "LegacyMapProjectLoader.hpp"

#include "LegacyCanonicalMapBuilder.hpp"
#include "Shared/Assets/Legacy/DatReader.hpp"
#include "Shared/Assets/Legacy/OtbReader.hpp"
#include "Shared/Assets/Legacy/SprReader.hpp"
#include "Shared/Formats/Legacy/OtbmStreamReader.hpp"

#include <exception>
#include <filesystem>
#include <string>
#include <utility>

namespace fantasy::studio::mapcore {
namespace {

void appendMessages(
    std::vector<std::string>& destination,
    const std::vector<std::string>& source,
    const std::string& prefix) {

    for (const auto& message : source) destination.push_back(prefix + message);
}

} // namespace

std::filesystem::path LegacyMapProjectLoader::resolveAuxPath(
    const std::filesystem::path& otbmPath,
    const std::optional<std::filesystem::path>& overridePath,
    const std::string& metadataPath,
    const char* fallbackSuffix) {

    if (overridePath.has_value()) return *overridePath;

    const std::filesystem::path base = otbmPath.parent_path();
    if (!metadataPath.empty()) {
        const std::filesystem::path referenced(metadataPath);
        return referenced.is_absolute() ? referenced : base / referenced;
    }

    return base / (otbmPath.stem().string() + fallbackSuffix);
}

LegacyMapProjectLoadResult LegacyMapProjectLoader::load(
    MapDocument& document,
    const LegacyMapProjectConfig& config) const {

    LegacyMapProjectLoadResult output;

    try {
        if (config.otbmPath.empty() || config.otbPath.empty() || config.datPath.empty() || config.sprPath.empty()) {
            output.report.errors.push_back("OTBM, OTB, DAT and SPR paths are required.");
            return output;
        }

        fantasy::assets::legacy::OtbReader otb(config.otbPath);
        fantasy::assets::legacy::DatReader1057 dat(config.datPath);
        fantasy::assets::legacy::SprReader spr(config.sprPath);

        output.report.datSignature = dat.header().signature;
        output.report.sprSignature = spr.info().signature;
        output.report.sprCount = spr.info().spriteCount;

        if (dat.header().signature != 0x42A3U) {
            output.report.warnings.push_back(
                "DAT signature is " + std::to_string(dat.header().signature) +
                "; the active 10.98 profile expects 0x42A3.");
        }

        if (spr.info().signature != 0x57BBD603U) {
            output.report.warnings.push_back(
                "SPR signature is " + std::to_string(spr.info().signature) +
                "; the active 10.98 profile expects 0x57BBD603.");
        }

        auto registryBuild = fantasy::assets::LegacyAssetRegistryBuilder{}.build(config.profileId, otb, dat);
        output.report.assetRegistry = registryBuild.report;
        appendMessages(output.report.warnings, registryBuild.report.warnings, "assets: ");

        LegacyCanonicalMapBuilder builder(registryBuild.registry);
        fantasy::legacy::OtbmStreamCallbacks callbacks;
        callbacks.onTile = [&builder](fantasy::legacy::LegacyImportedTile&& tile) {
            builder.addTile(std::move(tile));
        };
        callbacks.onTown = [&builder](fantasy::legacy::LegacyImportedTown&& town) {
            builder.addTown(std::move(town));
        };
        callbacks.onWaypoint = [&builder](fantasy::legacy::LegacyImportedWaypoint&& waypoint) {
            builder.addWaypoint(std::move(waypoint));
        };

        fantasy::legacy::OtbmStreamReader otbm(config.otbmPath, std::move(callbacks));
        const auto& stream = otbm.result();
        const auto& header = stream.header;

        if (header.formatVersion == 0 || header.formatVersion > 2) {
            output.report.errors.push_back(
                "OTBM header version " + std::to_string(header.formatVersion) +
                " is outside the TFS 1.4.2 vanilla gate (expected numeric version 1 or 2; editor OTBM v3 uses 2).");
            output.assets = std::move(registryBuild.registry);
            return output;
        }

        if (header.itemsMajorVersion < 3 || header.itemsMajorVersion > otb.version().major) {
            output.report.errors.push_back(
                "OTBM item major version " + std::to_string(header.itemsMajorVersion) +
                " is incompatible with loaded OTB major version " + std::to_string(otb.version().major) + ".");
            output.assets = std::move(registryBuild.registry);
            return output;
        }

        if (header.itemsMinorVersion > otb.version().minor) {
            output.report.warnings.push_back(
                "OTBM item minor version " + std::to_string(header.itemsMinorVersion) +
                " is newer than loaded OTB minor version " + std::to_string(otb.version().minor) + ".");
        }

        MapMetadata metadata;
        metadata.width = header.width;
        metadata.height = header.height;
        metadata.name = config.otbmPath.filename().string();
        metadata.description = stream.metadata.description;
        metadata.spawnFile = stream.metadata.spawnFile;
        metadata.houseFile = stream.metadata.houseFile;
        metadata.sourceProfileId = config.profileId;
        builder.setMetadata(std::move(metadata));
        builder.setItemCount(stream.diagnostics.itemCount);
        appendMessages(output.report.warnings, stream.diagnostics.warnings, "otbm: ");

        const auto housePath = resolveAuxPath(
            config.otbmPath,
            config.houseXmlPath,
            stream.metadata.houseFile,
            "-house.xml");
        const auto spawnPath = resolveAuxPath(
            config.otbmPath,
            config.spawnXmlPath,
            stream.metadata.spawnFile,
            "-spawn.xml");

        if (std::filesystem::exists(housePath)) {
            fantasy::legacy::LegacyMapImportModel housesModel;
            output.report.houses = fantasy::legacy::LegacyAuxXmlReader::loadHouses(housePath, housesModel);
            appendMessages(output.report.warnings, output.report.houses.warnings, "houses: ");
            appendMessages(output.report.errors, output.report.houses.errors, "houses: ");
            if (output.report.houses.success) {
                for (auto& house : housesModel.houses) builder.addHouse(std::move(house));
            }
        } else {
            output.report.errors.push_back("House XML not found: " + housePath.string());
        }

        if (std::filesystem::exists(spawnPath)) {
            fantasy::legacy::LegacyMapImportModel spawnsModel;
            output.report.spawns = fantasy::legacy::LegacyAuxXmlReader::loadSpawns(spawnPath, spawnsModel);
            appendMessages(output.report.warnings, output.report.spawns.warnings, "spawns: ");
            appendMessages(output.report.errors, output.report.spawns.errors, "spawns: ");
            if (output.report.spawns.success) {
                for (auto& spawn : spawnsModel.spawns) builder.addSpawn(std::move(spawn));
            }
        } else {
            output.report.errors.push_back("Spawn XML not found: " + spawnPath.string());
        }

        if (!output.report.errors.empty()) {
            output.assets = std::move(registryBuild.registry);
            return output;
        }

        LegacyMapAdaptResult adapted = builder.finish();
        output.report.map = adapted.report;
        appendMessages(output.report.warnings, output.report.map.warnings, "adapter: ");
        document.replaceMap(std::move(adapted.map), std::move(adapted.metadata));
        output.assets = std::move(registryBuild.registry);

        output.report.success = true;
        return output;
    } catch (const std::exception& error) {
        output.report.errors.push_back(error.what());
        return output;
    }
}

} // namespace fantasy::studio::mapcore
