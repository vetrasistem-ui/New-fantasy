#include "LegacyWorkspaceSession.hpp"

#include <algorithm>
#include <utility>

namespace fantasy::studio::mapcore {

Position LegacyWorkspaceSession::chooseInitialCenter(const MapDocument& document) {
    const auto& map = document.map();

    if (!map.towns().empty()) {
        const auto town = std::min_element(
            map.towns().begin(),
            map.towns().end(),
            [](const auto& left, const auto& right) { return left.first < right.first; });
        return town->second.templePosition;
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
        initialView_ = LegacyWorkspaceViewState{};
        view_ = initialView_;
        return false;
    }

    document_ = std::move(loadedDocument);
    initialView_ = LegacyWorkspaceViewState{};
    initialView_.center = chooseInitialCenter(document_);
    initialView_.floor = initialView_.center.z;
    view_ = initialView_;
    return true;
}

} // namespace fantasy::studio::mapcore
