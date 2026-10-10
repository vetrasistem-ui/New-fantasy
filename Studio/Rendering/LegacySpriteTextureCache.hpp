#pragma once

#include "MapEngine/Core/MapTypes.hpp"
#include "Shared/Assets/Legacy/DatReader.hpp"
#include "Shared/Assets/Legacy/SprReader.hpp"

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <unordered_map>

namespace fantasy::studio::rendering {

struct LegacyTextureView {
    SDL_Texture* texture = nullptr;
    std::int32_t widthPixels = 0;
    std::int32_t heightPixels = 0;
    std::int32_t anchorOffsetX = 0;
    std::int32_t anchorOffsetY = 0;
};

class LegacySpriteTextureCache {
public:
    LegacySpriteTextureCache(
        SDL_Renderer* renderer,
        const std::filesystem::path& datPath,
        const std::filesystem::path& sprPath);
    ~LegacySpriteTextureCache();

    LegacySpriteTextureCache(const LegacySpriteTextureCache&) = delete;
    LegacySpriteTextureCache& operator=(const LegacySpriteTextureCache&) = delete;

    [[nodiscard]] std::optional<LegacyTextureView> textureForClientId(
        std::uint32_t clientId,
        std::uint8_t patternX = 0,
        std::uint8_t patternY = 0,
        std::uint8_t patternZ = 0,
        std::uint8_t frame = 0);

    [[nodiscard]] std::optional<LegacyTextureView> textureForItem(
        const fantasy::studio::mapcore::Item& item,
        std::int16_t floor,
        std::uint8_t frame = 0);

    void clear() noexcept;
    [[nodiscard]] std::size_t cachedTextureCount() const noexcept { return cache_.size(); }
    [[nodiscard]] const fantasy::assets::legacy::DatHeader& datHeader() const noexcept { return dat_.header(); }
    [[nodiscard]] const fantasy::assets::legacy::SprInfo& sprInfo() const noexcept { return spr_.info(); }

private:
    struct TextureKey {
        std::uint32_t clientId = 0;
        std::uint8_t patternX = 0;
        std::uint8_t patternY = 0;
        std::uint8_t patternZ = 0;
        std::uint8_t frame = 0;
        bool operator==(const TextureKey&) const = default;
    };

    struct TextureKeyHash {
        std::size_t operator()(const TextureKey& key) const noexcept;
    };

    struct CachedTexture {
        SDL_Texture* texture = nullptr;
        std::int32_t widthPixels = 0;
        std::int32_t heightPixels = 0;
        std::int32_t anchorOffsetX = 0;
        std::int32_t anchorOffsetY = 0;
    };

    [[nodiscard]] std::optional<CachedTexture> buildTexture(const TextureKey& key);

    SDL_Renderer* renderer_ = nullptr;
    fantasy::assets::legacy::DatReader1057 dat_;
    fantasy::assets::legacy::SprReader spr_;
    std::unordered_map<TextureKey, CachedTexture, TextureKeyHash> cache_;
};

} // namespace fantasy::studio::rendering
