#pragma once

#include "../Core/MapDocument.hpp"
#include "../../Foundation/FantasyAssetCatalog.hpp"
#include "../../../Shared/Assets/LegacyAssetRegistry.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fantasy::studio::mapgen {

struct FantasyMapScriptReport {
    bool success = false;
    std::size_t commandsExecuted = 0;
    std::size_t affectedTiles = 0;
    std::optional<std::filesystem::path> savedPath;
    std::vector<std::string> messages;
};

// Small, deterministic command layer for AI/Codex and future Studio UI.
// It deliberately compiles semantic high-level operations into the existing
// canonical MapCommand/MapDocument path instead of introducing a second map
// representation. OTBM remains an export format handled by LegacyOtbmWriter.
class FantasyMapCommandsV1 {
public:
    [[nodiscard]] FantasyMapScriptReport execute(
        mapcore::MapDocument& document,
        std::string_view script,
        const foundation::FantasyAssetCatalog& catalog,
        const fantasy::assets::FantasyAssetRegistry& registry,
        const std::filesystem::path& outputRoot) const;
};

} // namespace fantasy::studio::mapgen
