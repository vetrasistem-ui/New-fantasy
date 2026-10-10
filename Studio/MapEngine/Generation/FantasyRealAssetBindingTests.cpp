#include "FantasyRealAssetBinding.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

fantasy::assets::LegacyAssetRecord record(
    const std::string& key,
    fantasy::assets::LegacyAssetKind kind,
    std::uint32_t serverId,
    std::uint32_t clientId,
    std::uint32_t spriteId) {

    fantasy::assets::LegacyAssetRecord value;
    value.semanticKey = key;
    value.kind = kind;
    value.serverId = serverId;
    value.clientId = clientId;
    value.spriteIds = {spriteId};
    return value;
}

} // namespace

int main() {
    try {
        fantasy::assets::FantasyAssetRegistry registry;
        std::string error;
        require(registry.registerLegacyAsset(
            record("legacy.pokefans1098.item.4526", fantasy::assets::LegacyAssetKind::Ground, 4526, 4600, 12000), &error), error);
        require(registry.registerLegacyAsset(
            record("legacy.pokefans1098.item.2708", fantasy::assets::LegacyAssetKind::Object, 2708, 2782, 9000), &error), error);

        fantasy::studio::mapgen::RealAssetBindingManifest manifest;
        manifest.profileId = "pokefans1098";
        manifest.families.push_back({"terrain.grass.basic", "Grass Basic", {"terrain", "grass"}, std::nullopt});
        manifest.bindings.push_back({
            "terrain.grass", 4526, "terrain.grass.basic", "center", {"terrain", "grass"}, 1});
        manifest.bindings.push_back({
            "nature.tree", 2708, "", "", {"nature", "tree"}, 1});

        const auto bound = fantasy::studio::mapgen::FantasyRealAssetBinder::bind(manifest, registry);
        require(bound.report.success, "valid real bindings should succeed");
        require(bound.report.bound == 2U, "unexpected number of bound assets");
        require(bound.catalog.resolveLegacyServerId("terrain.grass", registry).value_or(0U) == 4526U,
                "semantic grass did not resolve back to the real server id");
        require(bound.catalog.resolveLegacyServerId("nature.tree", registry).value_or(0U) == 2708U,
                "semantic tree did not resolve back to the real server id");

        auto bad = manifest;
        bad.bindings[1].serverId = 65530U;
        const auto rejected = fantasy::studio::mapgen::FantasyRealAssetBinder::bind(bad, registry);
        require(!rejected.report.success, "missing real server id must reject the binding set");
        require(!rejected.report.errors.empty(), "missing real server id should explain the failure");

        std::cout << "FANTASY_REAL_ASSET_BINDING PASS bound=" << bound.report.bound << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FANTASY_REAL_ASSET_BINDING FAIL: " << error.what() << '\n';
        return 1;
    }
}
