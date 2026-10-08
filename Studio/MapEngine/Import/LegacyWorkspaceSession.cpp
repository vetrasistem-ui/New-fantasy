#include "LegacyWorkspaceSession.hpp"

#include <utility>

namespace fantasy::studio::mapcore {

Position LegacyWorkspaceSession::chooseInitialCenter(const MapDocument& document) {
    const auto& map = document.map();

    if (!map.towns().empty()) {
        return map.towns().begin()->second.templePosition;
    }

    if (!map.spawnAreas().empty()) {
        return map.spawnAreas().front().center;
    }

    Position center;
    center.x = static_cast<std::int32_t>(document.metadata().width / 2U);
    center.y = static_cast<std::int32_t>(document.metadata().height / 2U);
    center.z = 7;
    return center;
}

bool LegacyWorkspaceSession::open(LegacyMapProjectConfig config) {
    MapDocument loadedDocument;
    auto result = LegacyMapProjectLoader{}.load(loadedDocument, config);

    config_ = std::move(config);
    report_ = std::move(result.report);
    assets_ = std::move(result.assets);

    if (!report_.success) {
        document_ = MapDocument{};
        view_ = LegacyWorkspaceViewState{};
        return false;
    }

    document_ = std::move(loadedDocument);
    view_ = LegacyWorkspaceViewState{};
    view_.center = chooseInitialCenter(document_);
    view_.floor = view_.center.z;
    return true;
}

} // namespace fantasy::studio::mapcore
