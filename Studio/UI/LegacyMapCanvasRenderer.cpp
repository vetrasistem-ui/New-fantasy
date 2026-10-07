#include "LegacyMapCanvasRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace fantasy::studio::ui {
namespace {

ImTextureRef textureRef(SDL_Texture* texture) {
    return ImTextureRef(static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(texture)));
}

void drawItemTexture(
    ImDrawList* drawList,
    ImVec2 tileOrigin,
    float tilePixels,
    const fantasy::studio::rendering::LegacyTextureView& view) {

    const float scale = tilePixels / 32.0f;
    const ImVec2 size(
        static_cast<float>(view.widthPixels) * scale,
        static_cast<float>(view.heightPixels) * scale);
    const ImVec2 anchor(
        tileOrigin.x - static_cast<float>(view.anchorOffsetX) * scale,
        tileOrigin.y - static_cast<float>(view.anchorOffsetY) * scale);

    drawList->AddImage(
        textureRef(view.texture),
        anchor,
        ImVec2(anchor.x + size.x, anchor.y + size.y));
}

} // namespace

LegacyMapCanvasStats LegacyMapCanvasRenderer::draw(
    ImDrawList* drawList,
    ImVec2 canvasOrigin,
    ImVec2 canvasSize,
    const fantasy::studio::mapcore::MapDocument& document,
    fantasy::studio::rendering::LegacySpriteTextureCache& textures,
    const LegacyMapCanvasView& view,
    const std::optional<fantasy::studio::mapcore::Position>& selected) const {

    LegacyMapCanvasStats stats;
    if (drawList == nullptr || canvasSize.x <= 0.0f || canvasSize.y <= 0.0f || view.tilePixels <= 0.0f) {
        return stats;
    }

    const float halfColumns = canvasSize.x / view.tilePixels * 0.5f;
    const float halfRows = canvasSize.y / view.tilePixels * 0.5f;
    const std::int32_t minX = static_cast<std::int32_t>(std::floor(static_cast<float>(view.centerTileX) - halfColumns)) - 2;
    const std::int32_t maxX = static_cast<std::int32_t>(std::ceil(static_cast<float>(view.centerTileX) + halfColumns)) + 2;
    const std::int32_t minY = static_cast<std::int32_t>(std::floor(static_cast<float>(view.centerTileY) - halfRows)) - 2;
    const std::int32_t maxY = static_cast<std::int32_t>(std::ceil(static_cast<float>(view.centerTileY) + halfRows)) + 2;

    const ImVec2 canvasCenter(canvasOrigin.x + canvasSize.x * 0.5f, canvasOrigin.y + canvasSize.y * 0.5f);
    const auto toScreen = [&](const fantasy::studio::mapcore::Position& position) {
        return ImVec2(
            canvasCenter.x + static_cast<float>(position.x - view.centerTileX) * view.tilePixels,
            canvasCenter.y + static_cast<float>(position.y - view.centerTileY) * view.tilePixels);
    };

    drawList->PushClipRect(canvasOrigin, ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y), true);

    document.map().forEachTileInRect(
        view.floor,
        fantasy::studio::mapcore::MapStorage::Rect{minX, minY, maxX, maxY},
        [&](const fantasy::studio::mapcore::Tile& tile) {
            ++stats.visitedTiles;
            const ImVec2 tileOrigin = toScreen(tile.position);

            if (tile.ground.has_value()) {
                const auto texture = textures.textureForItem(*tile.ground, view.floor);
                if (texture.has_value()) {
                    drawItemTexture(drawList, tileOrigin, view.tilePixels, *texture);
                    ++stats.renderedGrounds;
                } else {
                    ++stats.missingTextures;
                }
            }

            for (const auto& item : tile.items) {
                const auto texture = textures.textureForItem(item, view.floor);
                if (texture.has_value()) {
                    drawItemTexture(drawList, tileOrigin, view.tilePixels, *texture);
                    ++stats.renderedItems;
                } else {
                    ++stats.missingTextures;
                }
            }

            if (selected.has_value() && *selected == tile.position) {
                drawList->AddRect(
                    tileOrigin,
                    ImVec2(tileOrigin.x + view.tilePixels, tileOrigin.y + view.tilePixels),
                    IM_COL32(34, 211, 238, 255),
                    0.0f,
                    0,
                    std::max(2.0f, view.tilePixels * 0.06f));
            }
        });

    if (view.showGrid && view.tilePixels >= 10.0f) {
        const float leftOffset = std::fmod(
            canvasCenter.x - canvasOrigin.x - static_cast<float>(view.centerTileX) * view.tilePixels,
            view.tilePixels);
        const float topOffset = std::fmod(
            canvasCenter.y - canvasOrigin.y - static_cast<float>(view.centerTileY) * view.tilePixels,
            view.tilePixels);

        for (float x = canvasOrigin.x + leftOffset; x < canvasOrigin.x + canvasSize.x; x += view.tilePixels) {
            drawList->AddLine(
                ImVec2(x, canvasOrigin.y),
                ImVec2(x, canvasOrigin.y + canvasSize.y),
                IM_COL32(226, 232, 240, 18));
        }
        for (float y = canvasOrigin.y + topOffset; y < canvasOrigin.y + canvasSize.y; y += view.tilePixels) {
            drawList->AddLine(
                ImVec2(canvasOrigin.x, y),
                ImVec2(canvasOrigin.x + canvasSize.x, y),
                IM_COL32(226, 232, 240, 18));
        }
    }

    drawList->PopClipRect();
    return stats;
}

} // namespace fantasy::studio::ui
