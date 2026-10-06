#pragma once

#include "Shared/Formats/Legacy/LegacyMapImportModel.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace fantasy::legacy {

struct OtbmHeader {
    std::uint32_t formatVersion = 0;
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::uint32_t itemsMajorVersion = 0;
    std::uint32_t itemsMinorVersion = 0;
};

struct OtbmMetadata {
    std::string description;
    std::string spawnFile;
    std::string houseFile;
};

struct OtbmReadResult {
    OtbmHeader header;
    OtbmMetadata metadata;
    LegacyMapImportResult import;
};

// Read-only OTBM compatibility reader. It never writes or mutates the source
// file and it produces only the neutral LegacyMapImportModel.
class OtbmReader {
public:
    explicit OtbmReader(const std::filesystem::path& path);

    [[nodiscard]] const OtbmReadResult& result() const noexcept;

private:
    OtbmReadResult result_;
};

} // namespace fantasy::legacy
