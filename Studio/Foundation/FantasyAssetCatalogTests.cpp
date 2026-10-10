#include "Foundation/FantasyAssetCatalog.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using namespace fantasy::studio::foundation;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("FANTASY_ASSET_CATALOG TEST FAIL: " + message);
}

fantasy::assets::FantasyAssetRegistry makeRegistry() {
    fantasy::assets::FantasyAssetRegistry registry;
    std::string error;

    require(registry.registerLegacyAsset({"legacy.test.item.4526", fantasy::assets::LegacyAssetKind::Ground, 4526, 14526, {1001}}, &error), error);
    require(registry.registerLegacyAsset({"legacy.test.item.4527", fantasy::assets::LegacyAssetKind::Ground, 4527, 14527, {1002}}, &error), error);
    require(registry.registerLegacyAsset({"legacy.test.item.4608", fantasy::assets::LegacyAssetKind::Ground, 4608, 14608, {1003}}, &error), error);
    require(registry.registerLegacyAsset({"legacy.test.item.1029", fantasy::assets::LegacyAssetKind::Object, 1029, 11029, {1004}}, &error), error);
    return registry;
}

FantasyAssetCatalog makeCatalog() {
    FantasyAssetCatalog catalog;
    catalog.profileId = "test.1098";
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
    return catalog;
}

void testSemanticQueriesAndResolution() {
    const auto registry = makeRegistry();
    const auto catalog = makeCatalog();
    catalog.validate();

    require(catalog.findFamily("terrain.sand.basic") != nullptr, "sand family must be findable");
    require(catalog.findEntry("terrain.water.center") != nullptr, "water entry must be findable");

    const auto sand = catalog.findByTags({"terrain", "sand"});
    require(sand.size() == 2U, "terrain+sand query must return both sand variants");

    const auto centers = catalog.familyMembers("terrain.sand.basic", "center");
    require(centers.size() == 2U, "sand family center role must contain two variants");

    const auto* first = catalog.chooseFamilyMember("terrain.sand.basic", "center", 12345U);
    const auto* second = catalog.chooseFamilyMember("terrain.sand.basic", "center", 12345U);
    require(first != nullptr && second != nullptr && first->id == second->id,
        "family selection must be deterministic for the same seed");

    const auto serverId = catalog.resolveLegacyServerId("terrain.sand.center.a", registry);
    require(serverId.has_value() && *serverId == 4526U,
        "semantic sand id must resolve to the underlying legacy server id");

    require(catalog.validateLegacyReferences(registry).empty(),
        "all synthetic legacy references must resolve");
}

void testMissingLegacyReferenceIsReported() {
    const auto registry = makeRegistry();
    auto catalog = makeCatalog();
    catalog.entries.push_back({"nature.tree.missing", AssetCatalogSourceKind::LegacyRegistry,
        "legacy.test.item.9999", "", "", {"nature", "tree"}, 1});
    catalog.validate();

    const auto errors = catalog.validateLegacyReferences(registry);
    require(errors.size() == 1U, "missing legacy reference must produce one error");
    require(errors.front().find("legacy.test.item.9999") != std::string::npos,
        "missing-reference error must identify the unresolved asset reference");
}

void testValidationRejectsBrokenCatalog() {
    auto duplicate = makeCatalog();
    duplicate.entries.push_back(duplicate.entries.front());
    bool rejectedDuplicate = false;
    try {
        duplicate.validate();
    } catch (const std::invalid_argument&) {
        rejectedDuplicate = true;
    }
    require(rejectedDuplicate, "duplicate semantic entry id must be rejected");

    auto missingFamily = makeCatalog();
    missingFamily.entries.front().familyId = "terrain.family.missing";
    bool rejectedFamily = false;
    try {
        missingFamily.validate();
    } catch (const std::invalid_argument&) {
        rejectedFamily = true;
    }
    require(rejectedFamily, "entry referencing an unknown family must be rejected");
}

void testStoreRoundTripAndSchemaGuard(const fs::path& root) {
    const fs::path path = root / "Assets" / "Catalog" / "asset-catalog.json";
    const auto original = makeCatalog();
    FantasyAssetCatalogStore::save(path, original);
    require(fs::is_regular_file(path), "asset catalog store must write the catalog file");

    const auto loaded = FantasyAssetCatalogStore::load(path);
    require(loaded.profileId == original.profileId, "profile id must survive catalog roundtrip");
    require(loaded.families.size() == original.families.size(), "families must survive catalog roundtrip");
    require(loaded.entries.size() == original.entries.size(), "entries must survive catalog roundtrip");
    require(loaded.families.front().brushRef == original.families.front().brushRef,
        "family brush reference must survive catalog roundtrip");

    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        require(output.good(), "unable to rewrite schema fixture");
        output << R"({"schemaVersion":999,"profileId":"test.1098","families":[],"entries":[]})";
    }

    bool rejectedSchema = false;
    try {
        (void)FantasyAssetCatalogStore::load(path);
    } catch (const std::runtime_error&) {
        rejectedSchema = true;
    }
    require(rejectedSchema, "unsupported asset catalog schema must be rejected");
}

} // namespace

int main() {
    const fs::path root = fs::temp_directory_path() / "fantasy-asset-catalog-tests";
    std::error_code ignored;
    fs::remove_all(root, ignored);

    try {
        testSemanticQueriesAndResolution();
        testMissingLegacyReferenceIsReported();
        testValidationRejectsBrokenCatalog();
        testStoreRoundTripAndSchemaGuard(root);
        fs::remove_all(root, ignored);
        std::cout << "FANTASY_ASSET_CATALOG PASS\n";
        return 0;
    } catch (const std::exception& error) {
        fs::remove_all(root, ignored);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
