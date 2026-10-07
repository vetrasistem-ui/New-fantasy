#include "Shared/Assets/LegacyAssetRegistryBuilder.hpp"

#include <utility>

namespace fantasy::assets {

LegacyAssetKind classifyOtbItemGroup(std::uint8_t group) noexcept {
    // TFS ItemGroup_t: 0=None, 1=Ground, all remaining OTB item groups are
    // map objects from the editor's point of view. Border/doodad semantics are
    // layered later from materials/brush definitions, not inferred from OTB.
    if (group == 1) return LegacyAssetKind::Ground;
    return LegacyAssetKind::Object;
}

LegacyAssetRecord makeLegacyAssetRecord(
    const std::string& profileId,
    const legacy::OtbItemRecord& otbItem,
    const legacy::DatAppearance* appearance) {

    LegacyAssetRecord record;
    record.serverId = otbItem.serverId.value_or(0);
    record.clientId = otbItem.clientId.value_or(0);
    record.semanticKey = FantasyAssetRegistry::bootstrapSemanticKey(profileId, record.serverId);
    record.kind = classifyOtbItemGroup(otbItem.group);

    if (appearance != nullptr) {
        for (const auto& frameGroup : appearance->frameGroups) {
            record.spriteIds.insert(
                record.spriteIds.end(),
                frameGroup.spriteIds.begin(),
                frameGroup.spriteIds.end());
        }
    }

    return record;
}

LegacyAssetRegistryBuildResult LegacyAssetRegistryBuilder::build(
    const std::string& profileId,
    const legacy::OtbReader& otb,
    const legacy::DatReader1057& dat) const {

    LegacyAssetRegistryBuildResult result;

    for (const auto& otbItem : otb.items()) {
        if (!otbItem.serverId.has_value() || !otbItem.clientId.has_value() || *otbItem.serverId == 0 || *otbItem.clientId == 0) {
            ++result.report.missingServerOrClientId;
            result.report.warnings.push_back("Skipping OTB item without complete server/client id mapping.");
            continue;
        }

        const legacy::DatAppearance* appearance = dat.findItem(*otbItem.clientId);
        if (appearance == nullptr) {
            ++result.report.missingDatAppearance;
            result.report.warnings.push_back(
                "No DAT appearance for OTB server id " + std::to_string(*otbItem.serverId) +
                " / client id " + std::to_string(*otbItem.clientId) + ".");
        }

        LegacyAssetRecord record = makeLegacyAssetRecord(profileId, otbItem, appearance);
        std::string error;
        if (!result.registry.registerLegacyAsset(std::move(record), &error)) {
            result.report.warnings.push_back(std::move(error));
            continue;
        }
        ++result.report.registered;
    }

    return result;
}

} // namespace fantasy::assets
