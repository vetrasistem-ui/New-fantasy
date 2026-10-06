#include "Shared/Assets/LegacyAssetRegistry.hpp"

#include <cassert>
#include <string>

int main() {
    using namespace fantasy::assets;

    const LegacyAssetProfile profile = makePokeFans1098Profile();
    assert(profile.id == "pokefans1098");
    assert(profile.clientVersion == "10.98");
    assert(profile.usesRelativePathsOnly());

    FantasyAssetRegistry registry;
    LegacyAssetRecord ground;
    ground.semanticKey = FantasyAssetRegistry::bootstrapSemanticKey(profile.id, 4526);
    ground.kind = LegacyAssetKind::Ground;
    ground.serverId = 4526;
    ground.clientId = 4526;
    ground.spriteIds = {1001};

    std::string error;
    assert(registry.registerLegacyAsset(ground, &error));
    assert(error.empty());
    assert(registry.size() == 1);

    const LegacyAssetRecord* byKey = registry.findBySemanticKey("legacy.pokefans1098.item.4526");
    assert(byKey != nullptr);
    assert(byKey->serverId == 4526);
    assert(byKey->spriteIds.size() == 1);

    const LegacyAssetRecord* byServerId = registry.findByLegacyServerId(4526);
    assert(byServerId != nullptr);
    assert(byServerId->semanticKey == "legacy.pokefans1098.item.4526");

    error.clear();
    assert(!registry.registerLegacyAsset(ground, &error));
    assert(!error.empty());

    LegacyAssetRecord duplicateId;
    duplicateId.semanticKey = "terrain.grass.basic";
    duplicateId.kind = LegacyAssetKind::Ground;
    duplicateId.serverId = 4526;
    duplicateId.clientId = 4526;
    error.clear();
    assert(!registry.registerLegacyAsset(duplicateId, &error));
    assert(!error.empty());

    assert(FantasyAssetRegistry::bootstrapSemanticKey("PokeFans 10.98", 1515) ==
           "legacy.pokefans_10_98.item.1515");

    return 0;
}
