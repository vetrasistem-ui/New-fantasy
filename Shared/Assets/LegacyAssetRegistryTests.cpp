#include "Shared/Assets/LegacyAssetRegistry.hpp"
#include "Shared/Assets/LegacyAssetRegistryBuilder.hpp"

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

    // TFS ItemGroup_t uses group 1 for ground. Other OTB item groups are
    // canonical map objects; border/doodad semantics come from brush materials.
    assert(classifyOtbItemGroup(1) == LegacyAssetKind::Ground);
    assert(classifyOtbItemGroup(2) == LegacyAssetKind::Object);
    assert(classifyOtbItemGroup(13) == LegacyAssetKind::Object);

    fantasy::assets::legacy::OtbItemRecord otbGround;
    otbGround.group = 1;
    otbGround.serverId = 600;
    otbGround.clientId = 700;

    fantasy::assets::legacy::DatAppearance appearance;
    appearance.id = 700;
    fantasy::assets::legacy::DatFrameGroup frameGroup;
    frameGroup.width = 1;
    frameGroup.height = 1;
    frameGroup.layers = 1;
    frameGroup.patternX = 1;
    frameGroup.patternY = 1;
    frameGroup.patternZ = 1;
    frameGroup.frames = 1;
    frameGroup.spriteIds = {11, 12};
    appearance.frameGroups.push_back(frameGroup);

    const LegacyAssetRecord mapped = makeLegacyAssetRecord(profile.id, otbGround, &appearance);
    assert(mapped.kind == LegacyAssetKind::Ground);
    assert(mapped.serverId == 600);
    assert(mapped.clientId == 700);
    assert(mapped.semanticKey == "legacy.pokefans1098.item.600");
    assert(mapped.spriteIds.size() == 2);
    assert(mapped.spriteIds[0] == 11 && mapped.spriteIds[1] == 12);

    return 0;
}
