#pragma once

#include "Shared/Formats/Legacy/OtbmReader.hpp"

#include <filesystem>
#include <functional>

namespace fantasy::legacy {

struct OtbmStreamCallbacks {
    std::function<void(LegacyImportedTile&&)> onTile;
    std::function<void(LegacyImportedTown&&)> onTown;
    std::function<void(LegacyImportedWaypoint&&)> onWaypoint;
};

struct OtbmStreamReadResult {
    OtbmHeader header;
    OtbmMetadata metadata;
    LegacyImportDiagnostics diagnostics;
};

// Incremental OTBM reader for large maps. The file bytes may be resident, but
// the nodal tree and the complete LegacyMapImportModel are never materialized.
// Tiles/items are emitted as soon as their subtree has been parsed.
class OtbmStreamReader {
public:
    explicit OtbmStreamReader(
        const std::filesystem::path& path,
        OtbmStreamCallbacks callbacks = {});

    [[nodiscard]] const OtbmStreamReadResult& result() const noexcept { return result_; }

private:
    OtbmStreamReadResult result_;
};

} // namespace fantasy::legacy
