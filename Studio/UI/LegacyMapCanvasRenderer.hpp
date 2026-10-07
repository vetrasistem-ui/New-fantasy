#pragma once

#include "MapEngine/Core/MapDocument.hpp"
#include "Rendering/LegacySpriteTextureCache.hpp"

#include "imgui.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace fantasy::studio::ui {

struct LegacyMapCanvasView {
    std::int32_t centerTileX = 0;
    std::int32_t centerTileY = 0;
    std::int16_t floor = 7;
    float tilePixels = 32.0f;
    bool showGrid = true;
};

struct LegacyMapCanvasStats {
    std::size_t visitedTiles = 0;
    std::size_t renderedGrounds = 0;
    std::size_t renderedItems = 0;
    std::size_t missingTextures = 0;
};

class LegacyMapCanvasRenderer {
public:
    [[nodiscard]] LegacyMapCanvasStats draw(
        ImDrawList* drawList,
        ImVec2 canvasOrigin,
        ImVec2 canvasSize,
        const fantasy::studio::mapcore::MapDocument& document,
        fantasy::studio::rendering::LegacySpriteTextureCache& textures,
        const LegacyMapCanvasView& view,
        const std::optional<fantasy::studio::mapcore::Position>& selected = std::nullopt) const;
};

} // namespace fantasy::studio::ui
