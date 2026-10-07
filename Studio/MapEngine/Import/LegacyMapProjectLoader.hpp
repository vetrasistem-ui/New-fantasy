#pragma once

#include "LegacyMapAdapter.hpp"
#include "Shared/Assets/LegacyAssetRegistryBuilder.hpp"
#include "Shared/Formats/Legacy/LegacyAuxXmlReader.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fantasy::studio::mapcore {

struct LegacyMapProjectConfig {
    std::string profileId = "pokefans1098";
    std::filesystem::path otbmPath;
    std::filesystem::path otbPath;
    std::filesystem::path datPath;
    std::optional<std::filesystem::path> houseXmlPath;
    std::optional<std::filesystem::path> spawnXmlPath;
};

struct LegacyMapProjectReport {
    bool success = false;
    fantasy::assets::LegacyAssetRegistryBuildReport assetRegistry;
    fantasy::legacy::LegacyAuxXmlReport houses;
    fantasy::legacy::LegacyAuxXmlReport spawns;
    LegacyMapAdaptReport map;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

struct LegacyMapProjectLoadResult {
    fantasy::assets::FantasyAssetRegistry assets;
    LegacyMapProjectReport report;
};

class LegacyMapProjectLoader {
public:
    [[nodiscard]] LegacyMapProjectLoadResult load(
        MapDocument& document,
        const LegacyMapProjectConfig& config) const;

private:
    [[nodiscard]] static std::filesystem::path resolveAuxPath(
        const std::filesystem::path& otbmPath,
        const std::optional<std::filesystem::path>& overridePath,
        const std::string& metadataPath,
        const char* fallbackSuffix);
};

} // namespace fantasy::studio::mapcore
