#pragma once

#include "Shared/Assets/LegacyAssetRegistry.hpp"
#include "Shared/Assets/Legacy/DatReader.hpp"
#include "Shared/Assets/Legacy/OtbReader.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fantasy::assets {

struct LegacyAssetRegistryBuildReport {
    std::size_t registered = 0;
    std::size_t missingServerOrClientId = 0;
    std::size_t missingDatAppearance = 0;
    std::vector<std::string> warnings;
};

struct LegacyAssetRegistryBuildResult {
    FantasyAssetRegistry registry;
    LegacyAssetRegistryBuildReport report;
};

[[nodiscard]] LegacyAssetKind classifyOtbItemGroup(std::uint8_t group) noexcept;

[[nodiscard]] LegacyAssetRecord makeLegacyAssetRecord(
    const std::string& profileId,
    const legacy::OtbItemRecord& otbItem,
    const legacy::DatAppearance* appearance);

class LegacyAssetRegistryBuilder {
public:
    [[nodiscard]] LegacyAssetRegistryBuildResult build(
        const std::string& profileId,
        const legacy::OtbReader& otb,
        const legacy::DatReader1057& dat) const;
};

} // namespace fantasy::assets
