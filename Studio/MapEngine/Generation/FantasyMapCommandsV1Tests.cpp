#include "FantasyMapCommandsV1.hpp"

#include "Shared/Formats/Legacy/OtbmReader.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using fantasy::assets::FantasyAssetRegistry;
using fantasy::assets::LegacyAssetKind;
using fantasy::assets::LegacyAssetRecord;
using fantasy::studio::foundation::AssetCatalogEntry;
using fantasy::studio::foundation::AssetCatalogSourceKind;
using fantasy::studio::foundation::FantasyAssetCatalog;
using fantasy::studio::mapcore::MapDocument;
using fantasy::studio::mapcore::Position;
using fantasy::studio::mapgen::FantasyMapCommandsV1;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void registerAsset(
    FantasyAssetRegistry& registry,
    const std::string& key,
    LegacyAssetKind kind,
    std::uint32_t serverId,
    std::uint32_t clientId) {

    LegacyAssetRecord record;
    record.semanticKey = key;
    record.kind = kind;
    record.serverId = serverId;
    record.clientId = clientId;
    record.spriteIds = {clientId};
    std::string error;
    require(registry.registerLegacyAsset(std::move(record), &error), "registry insert failed: " + error);
}

AssetCatalogEntry entry(const std::string& id, const std::string& ref, const std::string& tag) {
    AssetCatalogEntry value;
    value.id = id;
    value.source = AssetCatalogSourceKind::LegacyRegistry;
    value.assetRef = ref;
    value.tags = {tag};
    return value;
}

} // namespace

int main() {
    try {
        FantasyAssetRegistry registry;
        registerAsset(registry, "legacy.grass", LegacyAssetKind::Ground, 100, 1000);
        registerAsset(registry, "legacy.water", LegacyAssetKind::Ground, 101, 1001);
        registerAsset(registry, "legacy.tree", LegacyAssetKind::Object, 102, 1002);
        registerAsset(registry, "legacy.wood.floor", LegacyAssetKind::Ground, 103, 1003);
        registerAsset(registry, "legacy.wood.wall", LegacyAssetKind::Object, 104, 1004);
        registerAsset(registry, "legacy.wood.door", LegacyAssetKind::Object, 105, 1005);
        registerAsset(registry, "legacy.dirt.road", LegacyAssetKind::Ground, 106, 1006);

        FantasyAssetCatalog catalog;
        catalog.profileId = "test.1098";
        catalog.entries = {
            entry("terrain.grass", "legacy.grass", "grass"),
            entry("terrain.water", "legacy.water", "water"),
            entry("nature.tree", "legacy.tree", "tree"),
            entry("floor.wood", "legacy.wood.floor", "floor"),
            entry("structure.wall.wood", "legacy.wood.wall", "wall"),
            entry("structure.door.wood", "legacy.wood.door", "door"),
            entry("road.dirt", "legacy.dirt.road", "road"),
        };
        catalog.validate();

        const auto root = std::filesystem::temp_directory_path() / "fantasy-map-commands-v1-test";
        std::error_code ignored;
        std::filesystem::remove_all(root, ignored);
        std::filesystem::create_directories(root);

        MapDocument document;
        const std::string script = R"MAP(
# A small AI-friendly region: grass base, water, forest, house and a road.
NEW_REGION demo_region 64 64 7 terrain.grass
PLACE_WATER 0 54 64 10 7 terrain.water
PLACE_FOREST 4 4 20 20 7 nature.tree 35 12345
PLACE_BUILDING 28 20 10 8 7 floor.wood structure.wall.wood structure.door.wood
CONNECT 33 27 33 53 7 3 road.dirt
VALIDATE
SAVE generated/demo.otbm
)MAP";

        const auto report = FantasyMapCommandsV1{}.execute(document, script, catalog, registry, root);
        if (!report.success) {
            std::string details;
            for (const auto& message : report.messages) details += message + "\n";
            throw std::runtime_error("map script failed:\n" + details);
        }

        require(report.commandsExecuted == 7U, "unexpected high-level command count");
        require(report.savedPath.has_value(), "SAVE did not return an OTBM path");
        require(std::filesystem::exists(*report.savedPath), "generated OTBM file is missing");
        require(std::filesystem::file_size(*report.savedPath) > 0U, "generated OTBM is empty");
        require(document.metadata().width == 64U && document.metadata().height == 64U, "region dimensions mismatch");
        require(document.map().tileCount() == 4096U, "NEW_REGION did not materialize the expected tile grid");

        const auto* grass = document.map().findTile(Position{0, 0, 7});
        require(grass != nullptr && grass->ground.has_value() && grass->ground->serverId == 100U, "grass terrain mismatch");

        const auto* water = document.map().findTile(Position{0, 60, 7});
        require(water != nullptr && water->ground.has_value() && water->ground->serverId == 101U, "water terrain mismatch");

        const auto* doorway = document.map().findTile(Position{33, 27, 7});
        require(doorway != nullptr && doorway->ground.has_value() && doorway->ground->serverId == 106U, "road did not connect to building entrance");
        bool foundDoor = false;
        for (const auto& item : doorway->items) {
            if (item.serverId == 105U) foundDoor = true;
        }
        require(foundDoor, "building entrance is missing its semantic door asset");

        std::size_t trees = 0;
        document.map().forEachTileInRect(7, {4, 4, 23, 23}, [&](const auto& tile) {
            for (const auto& item : tile.items) {
                if (item.serverId == 102U) ++trees;
            }
        });
        require(trees > 0U && trees < 400U, "forest density generation is outside expected range");

        const fantasy::legacy::OtbmReader reopened(*report.savedPath);
        require(reopened.result().header.width == 64U && reopened.result().header.height == 64U, "reopened OTBM dimensions mismatch");
        require(reopened.result().import.diagnostics.tileCount == 4096U, "reopened OTBM tile count mismatch");
        require(reopened.result().import.diagnostics.itemCount > 4096U, "reopened OTBM lost generated objects");

        MapDocument invalidDocument;
        const auto invalid = FantasyMapCommandsV1{}.execute(
            invalidDocument,
            "NEW_REGION bad 8 8 7 terrain.missing\n",
            catalog,
            registry,
            root);
        require(!invalid.success, "missing semantic asset should reject generation");
        require(invalidDocument.map().tileCount() == 0U, "failed NEW_REGION must not partially materialize a map");

        std::filesystem::remove_all(root, ignored);
        std::cout << "FANTASY_MAP_COMMANDS_V1 PASS\n";
        std::cout << "commands=" << report.commandsExecuted
                  << " affected=" << report.affectedTiles
                  << " trees=" << trees << "\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FANTASY_MAP_COMMANDS_V1 FAIL: " << error.what() << '\n';
        return 1;
    }
}
