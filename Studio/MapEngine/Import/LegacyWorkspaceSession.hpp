#pragma once

#include "LegacyMapProjectLoader.hpp"

#include <filesystem>
#include <optional>

namespace fantasy::studio::mapcore {

struct LegacyWorkspaceViewState {
    Position center{0, 0, 7};
    std::int16_t floor = 7;
    float tilePixels = 32.0f;
    bool showGrid = true;
};

class LegacyWorkspaceSession {
public:
    [[nodiscard]] bool open(LegacyMapProjectConfig config);

    [[nodiscard]] bool ready() const noexcept { return report_.success; }
    [[nodiscard]] MapDocument& document() noexcept { return document_; }
    [[nodiscard]] const MapDocument& document() const noexcept { return document_; }
    [[nodiscard]] fantasy::assets::FantasyAssetRegistry& assets() noexcept { return assets_; }
    [[nodiscard]] const fantasy::assets::FantasyAssetRegistry& assets() const noexcept { return assets_; }
    [[nodiscard]] const LegacyMapProjectReport& report() const noexcept { return report_; }
    [[nodiscard]] const LegacyMapProjectConfig& config() const noexcept { return config_; }
    [[nodiscard]] LegacyWorkspaceViewState& view() noexcept { return view_; }
    [[nodiscard]] const LegacyWorkspaceViewState& view() const noexcept { return view_; }

    [[nodiscard]] const std::filesystem::path& datPath() const noexcept { return config_.datPath; }
    [[nodiscard]] const std::filesystem::path& sprPath() const noexcept { return config_.sprPath; }

private:
    [[nodiscard]] static Position chooseInitialCenter(const MapDocument& document);

    LegacyMapProjectConfig config_;
    MapDocument document_;
    fantasy::assets::FantasyAssetRegistry assets_;
    LegacyMapProjectReport report_;
    LegacyWorkspaceViewState view_;
};

} // namespace fantasy::studio::mapcore
