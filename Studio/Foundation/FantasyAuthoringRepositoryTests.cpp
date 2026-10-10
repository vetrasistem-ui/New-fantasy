#include "Foundation/FantasyAuthoringRepository.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using namespace fantasy::studio::foundation;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("FANTASY_AUTHORING_REPOSITORY TEST FAIL: " + message);
}

void testZoneAndItemCrud(FantasyAuthoringRepository& repository) {
    ZoneDefinition zone;
    zone.id = "zone.town.center";
    zone.name = "Town Center";
    zone.tags = {"safe", "social.hub"};
    zone.rectangles.push_back({100, 100, 7, 12, 12});
    repository.save(zone);

    ItemDefinition item;
    item.id = "item.health_potion";
    item.name = "Health Potion";
    item.appearanceRef = "appearance.item.health_potion";
    item.tags = {"consumable", "healing"};
    item.components = {{"use.consume", {{"system.id", "system.item.health_potion"}}}};
    repository.save(item);

    require(repository.zones() == std::vector<std::string>{"zone.town.center"},
        "zone list must contain saved zone");
    require(repository.items() == std::vector<std::string>{"item.health_potion"},
        "item list must contain saved item");
    require(repository.loadZone(zone.id).contains({105, 105, 7}),
        "loaded zone geometry must remain valid");
    require(repository.loadItem(item.id).name == item.name,
        "loaded item must preserve its name");

    require(repository.removeZone(zone.id), "zone remove must report success");
    require(repository.removeItem(item.id), "item remove must report success");
    require(repository.zones().empty(), "zone list must be empty after remove");
    require(repository.items().empty(), "item list must be empty after remove");
    require(!repository.removeZone(zone.id), "second zone remove must report false");
}

void testModernAssetAndBrushCrud(FantasyAuthoringRepository& repository) {
    ModernAssetDefinition asset;
    asset.id = "asset.ground.grass";
    asset.kind = ModernAssetKind::Ground;
    asset.tags = {"terrain.grass", "walkable"};
    asset.layers = {{"base", 0, 0, 0, {{"visual.grass.01", 100}}}};
    repository.saveModernAsset(asset);

    BrushDefinition brush;
    brush.id = "brush.terrain.grass";
    brush.kind = BrushKind::Terrain;
    brush.requiredTags = {"terrain.grass"};
    brush.variants = {{asset.id, 1}};
    repository.saveBrush(brush);

    require(repository.modernAssets() == std::vector<std::string>{asset.id},
        "modern asset list must contain saved asset");
    require(repository.brushes() == std::vector<std::string>{brush.id},
        "brush list must contain saved brush");
    require(repository.loadModernAsset(asset.id).kind == ModernAssetKind::Ground,
        "modern asset kind must survive repository roundtrip");
    require(repository.loadBrush(brush.id).variants.front().assetRef == asset.id,
        "brush variant must preserve modern asset reference");

    require(repository.removeBrush(brush.id), "brush remove must report success");
    require(repository.removeModernAsset(asset.id), "modern asset remove must report success");
}

void testAssetCatalogCrudAndSemanticQueries(FantasyAuthoringRepository& repository) {
    FantasyAssetCatalog catalog;
    catalog.profileId = "assets.1098.test";
    catalog.families = {
        {"terrain.sand.basic", "Basic Sand", {"terrain", "sand", "ground"}, "brush.terrain.sand"},
        {"terrain.water.basic", "Basic Water", {"terrain", "water", "ground"}, "brush.terrain.water"},
        {"structure.wall.stone", "Stone Wall", {"structure", "wall", "stone"}, "brush.wall.stone"},
    };
    catalog.entries = {
        {"terrain.sand.center.a", AssetCatalogSourceKind::LegacyRegistry, "legacy.test.item.4526",
            "terrain.sand.basic", "center", {"terrain", "sand", "ground", "desert", "beach"}, 1},
        {"terrain.sand.center.b", AssetCatalogSourceKind::LegacyRegistry, "legacy.test.item.4527",
            "terrain.sand.basic", "center", {"terrain", "sand", "ground", "desert", "beach"}, 3},
        {"terrain.water.center", AssetCatalogSourceKind::LegacyRegistry, "legacy.test.item.4608",
            "terrain.water.basic", "center", {"terrain", "water", "ground"}, 1},
        {"structure.wall.stone.segment", AssetCatalogSourceKind::LegacyRegistry, "legacy.test.item.1029",
            "structure.wall.stone", "segment", {"structure", "wall", "stone"}, 1},
    };

    repository.saveAssetCatalog(catalog);
    require(repository.hasAssetCatalog(), "saved semantic asset catalog must be discoverable");

    const auto loaded = repository.loadAssetCatalog();
    require(loaded.profileId == catalog.profileId, "asset catalog profile must survive repository roundtrip");
    require(loaded.families.size() == 3U && loaded.entries.size() == 4U,
        "asset catalog families and entries must survive repository roundtrip");
    require(loaded.findByTags({"terrain", "sand"}).size() == 2U,
        "semantic tag query must find both sand variants");
    require(loaded.familyMembers("terrain.sand.basic", "center").size() == 2U,
        "family role query must find both sand center variants");

    const auto* selectedA = loaded.chooseFamilyMember("terrain.sand.basic", "center", 12345U);
    const auto* selectedB = loaded.chooseFamilyMember("terrain.sand.basic", "center", 12345U);
    require(selectedA != nullptr && selectedB != nullptr && selectedA->id == selectedB->id,
        "semantic family selection must be deterministic for the same seed");

    auto invalid = catalog;
    invalid.entries.front().familyId = "terrain.family.missing";
    bool rejectedMissingFamily = false;
    try {
        invalid.validate();
    } catch (const std::invalid_argument&) {
        rejectedMissingFamily = true;
    }
    require(rejectedMissingFamily, "asset catalog must reject entries that reference a missing family");

    require(repository.removeAssetCatalog(), "asset catalog remove must report success");
    require(!repository.hasAssetCatalog(), "asset catalog must disappear after remove");
}

void testSortedListing(FantasyAuthoringRepository& repository) {
    for (const std::string id : {"item.zeta", "item.alpha", "item.middle"}) {
        ItemDefinition item;
        item.id = id;
        item.name = id;
        repository.save(item);
    }
    require(repository.items() == std::vector<std::string>({"item.alpha", "item.middle", "item.zeta"}),
        "repository listings must be deterministic and sorted");
}

} // namespace

int main() {
    const fs::path root = fs::temp_directory_path() / "fantasy-authoring-repository-tests";
    std::error_code ignored;
    fs::remove_all(root, ignored);

    try {
        FantasyAuthoringRepository repository(root);
        testZoneAndItemCrud(repository);
        testModernAssetAndBrushCrud(repository);
        testAssetCatalogCrudAndSemanticQueries(repository);
        testSortedListing(repository);
        fs::remove_all(root, ignored);
        std::cout << "FANTASY_AUTHORING_REPOSITORY PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        fs::remove_all(root, ignored);
        return 1;
    }
}
