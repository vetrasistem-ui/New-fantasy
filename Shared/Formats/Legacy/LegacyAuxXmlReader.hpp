#pragma once

#include "Shared/Formats/Legacy/LegacyMapImportModel.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace fantasy::legacy {

struct LegacyAuxXmlReport {
    bool success = false;
    std::size_t houseCount = 0;
    std::size_t spawnAreaCount = 0;
    std::size_t spawnEntryCount = 0;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

class LegacyAuxXmlReader {
public:
    [[nodiscard]] static LegacyAuxXmlReport loadHouses(
        const std::filesystem::path& path,
        LegacyMapImportModel& model);

    [[nodiscard]] static LegacyAuxXmlReport loadSpawns(
        const std::filesystem::path& path,
        LegacyMapImportModel& model);
};

} // namespace fantasy::legacy
