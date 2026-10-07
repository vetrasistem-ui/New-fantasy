#pragma once

#include "../Core/MapDocument.hpp"
#include "Shared/Assets/LegacyAssetRegistry.hpp"
#include "Shared/Formats/Legacy/OtbmReader.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

namespace fantasy::studio::mapcore {

struct LegacyMapAdaptReport {
    std::size_t tileCount = 0;
    std::size_t itemCount = 0;
    std::size_t townCount = 0;
    std::size_t houseCount = 0;
    std::size_t spawnCount = 0;
    std::vector<std::uint32_t> unresolvedServerIds;
    std::vector<std::string> warnings;
};

struct LegacyMapAdaptResult {
    MapStorage map;
    MapMetadata metadata;
    LegacyMapAdaptReport report;
};

class LegacyMapAdapter {
public:
    [[nodiscard]] LegacyMapAdaptResult adapt(
        const fantasy::legacy::OtbmReadResult& source,
        const fantasy::assets::FantasyAssetRegistry& assets) const;

    [[nodiscard]] LegacyMapAdaptReport load(
        MapDocument& document,
        const fantasy::legacy::OtbmReadResult& source,
        const fantasy::assets::FantasyAssetRegistry& assets) const;
};

} // namespace fantasy::studio::mapcore
