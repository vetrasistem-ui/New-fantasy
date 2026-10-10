#pragma once

#include "MapEngine/Import/LegacyMapProjectLoader.hpp"
#include <filesystem>
#include <cstdint>

namespace fantasy::atlas {
struct AtlasExportReport {
    std::uint64_t tiles = 0;
    std::uint64_t items = 0;
    std::uint64_t assets = 0;
};

// Read-only derived index. Never parses or writes an OTBM.
class MapAtlasExporter {
public:
    AtlasExportReport exportMap(
        const studio::mapcore::MapDocument& document,
        const assets::FantasyAssetRegistry& registry,
        const studio::mapcore::LegacyMapProjectConfig& sources,
        const std::filesystem::path& directory) const;
};
}
