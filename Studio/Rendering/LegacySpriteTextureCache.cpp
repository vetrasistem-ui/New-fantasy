#include "LegacySpriteTextureCache.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

namespace fantasy::studio::rendering {
namespace {

std::uint8_t normalizedIndex(std::uint8_t value, std::uint8_t count) noexcept {
    return count == 0 ? 0 : static_cast<std::uint8_t>(value % count);
}

std::size_t spriteIndex(
    const fantasy::assets::legacy::DatFrameGroup& group,
    std::uint8_t componentX,
    std::uint8_t componentY,
    std::uint8_t layer,
    std::uint8_t patternX,
    std::uint8_t patternY,
    std::uint8_t patternZ,
    std::uint8_t frame) {

    std::size_t value = frame;
    value = value * group.patternY + patternY;
    value = value * group.patternX + patternX;
    value = value * group.layers + layer;
    value = value * group.height + componentY;
    value = value * group.width + componentX;
    (void)patternZ;

    // DAT 10.57 stores pattern Z before frame in the full sprite product.
    // Rebuild the canonical index explicitly when patternZ has more than one value.
    if (group.patternZ > 1) {
        value = frame;
        value = value * group.patternZ + patternZ;
        value = value * group.patternY + patternY;
        value = value * group.patternX + patternX;
        value = value * group.layers + layer;
        value = value * group.height + componentY;
        value = value * group.width + componentX;
    }
    return value;
}

} // namespace

std::size_t LegacySpriteTextureCache::TextureKeyHash::operator()(const TextureKey& key) const noexcept {
    std::size_t value = static_cast<std::size_t>(key.clientId);
    value ^= static_cast<std::size_t>(key.patternX) << 8U;
    value ^= static_cast<std::size_t>(key.patternY) << 16U;
    value ^= static_cast<std::size_t>(key.patternZ) << 24U;
    value ^= static_cast<std::size_t>(key.frame) << 32U;
    return value;
}

LegacySpriteTextureCache::LegacySpriteTextureCache(
    SDL_Renderer* renderer,
    const std::filesystem::path& datPath,
    const std::filesystem::path& sprPath)
    : renderer_(renderer), dat_(datPath), spr_(sprPath) {

    if (renderer_ == nullptr) throw std::invalid_argument("Legacy sprite texture cache requires an SDL renderer.");
}

LegacySpriteTextureCache::~LegacySpriteTextureCache() {
    clear();
}

std::optional<LegacyTextureView> LegacySpriteTextureCache::textureForClientId(
    std::uint32_t clientId,
    std::uint8_t patternX,
    std::uint8_t patternY,
    std::uint8_t patternZ,
    std::uint8_t frame) {

    const auto* appearance = dat_.findItem(clientId);
    if (appearance == nullptr || appearance->frameGroups.empty()) return std::nullopt;
    const auto& group = appearance->frameGroups.front();
    if (group.width == 0 || group.height == 0 || group.layers == 0 ||
        group.patternX == 0 || group.patternY == 0 || group.patternZ == 0 || group.frames == 0) {
        return std::nullopt;
    }

    TextureKey key;
    key.clientId = clientId;
    key.patternX = normalizedIndex(patternX, group.patternX);
    key.patternY = normalizedIndex(patternY, group.patternY);
    key.patternZ = normalizedIndex(patternZ, group.patternZ);
    key.frame = normalizedIndex(frame, group.frames);

    auto cached = cache_.find(key);
    if (cached == cache_.end()) {
        auto built = buildTexture(key);
        if (!built.has_value()) return std::nullopt;
        cached = cache_.emplace(key, std::move(*built)).first;
    }

    return LegacyTextureView{
        cached->second.texture,
        cached->second.widthPixels,
        cached->second.heightPixels,
        cached->second.anchorOffsetX,
        cached->second.anchorOffsetY
    };
}

std::optional<LegacyTextureView> LegacySpriteTextureCache::textureForItem(
    const fantasy::studio::mapcore::Item& item,
    std::int16_t floor,
    std::uint8_t frame) {

    if (item.clientId == 0) return std::nullopt;
    const auto* appearance = dat_.findItem(item.clientId);
    if (appearance == nullptr || appearance->frameGroups.empty()) return std::nullopt;
    const auto& group = appearance->frameGroups.front();
    const std::uint8_t floorPattern = group.patternZ == 0 ? 0 :
        static_cast<std::uint8_t>(static_cast<std::uint16_t>(std::max<std::int16_t>(0, floor)) % group.patternZ);
    return textureForClientId(item.clientId, 0, 0, floorPattern, frame);
}

std::optional<LegacySpriteTextureCache::CachedTexture> LegacySpriteTextureCache::buildTexture(const TextureKey& key) {
    const auto* appearance = dat_.findItem(key.clientId);
    if (appearance == nullptr || appearance->frameGroups.empty()) return std::nullopt;
    const auto& group = appearance->frameGroups.front();

    const std::int32_t widthPixels = static_cast<std::int32_t>(group.width) * 32;
    const std::int32_t heightPixels = static_cast<std::int32_t>(group.height) * 32;
    if (widthPixels <= 0 || heightPixels <= 0) return std::nullopt;

    std::vector<std::uint8_t> pixels(
        static_cast<std::size_t>(widthPixels) * static_cast<std::size_t>(heightPixels) * 4U,
        0);

    for (std::uint8_t componentX = 0; componentX < group.width; ++componentX) {
        for (std::uint8_t componentY = 0; componentY < group.height; ++componentY) {
            for (std::uint8_t layer = 0; layer < group.layers; ++layer) {
                const std::size_t index = spriteIndex(
                    group,
                    componentX,
                    componentY,
                    layer,
                    key.patternX,
                    key.patternY,
                    key.patternZ,
                    key.frame);
                if (index >= group.spriteIds.size()) return std::nullopt;

                const std::uint32_t spriteId = group.spriteIds[index];
                if (spriteId == 0 || !spr_.hasSprite(spriteId)) continue;
                const auto sprite = spr_.readSprite(spriteId);

                const std::int32_t destinationX = (static_cast<std::int32_t>(group.width) - 1 - componentX) * 32;
                const std::int32_t destinationY = (static_cast<std::int32_t>(group.height) - 1 - componentY) * 32;

                for (std::int32_t y = 0; y < 32; ++y) {
                    for (std::int32_t x = 0; x < 32; ++x) {
                        const std::size_t source = (static_cast<std::size_t>(y) * 32U + static_cast<std::size_t>(x)) * 4U;
                        if (sprite.pixels[source + 3] == 0) continue;

                        const std::size_t destination =
                            (static_cast<std::size_t>(destinationY + y) * static_cast<std::size_t>(widthPixels) +
                             static_cast<std::size_t>(destinationX + x)) * 4U;
                        pixels[destination + 0] = sprite.pixels[source + 0];
                        pixels[destination + 1] = sprite.pixels[source + 1];
                        pixels[destination + 2] = sprite.pixels[source + 2];
                        pixels[destination + 3] = sprite.pixels[source + 3];
                    }
                }
            }
        }
    }

    SDL_Surface* surface = SDL_CreateSurface(widthPixels, heightPixels, SDL_PIXELFORMAT_RGBA32);
    if (surface == nullptr) {
        throw std::runtime_error(std::string("Unable to create legacy sprite surface: ") + SDL_GetError());
    }

    for (std::int32_t row = 0; row < heightPixels; ++row) {
        std::memcpy(
            static_cast<std::uint8_t*>(surface->pixels) + static_cast<std::size_t>(row) * surface->pitch,
            pixels.data() + static_cast<std::size_t>(row) * static_cast<std::size_t>(widthPixels) * 4U,
            static_cast<std::size_t>(widthPixels) * 4U);
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_DestroySurface(surface);
    if (texture == nullptr) {
        throw std::runtime_error(std::string("Unable to create legacy sprite texture: ") + SDL_GetError());
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    CachedTexture result;
    result.texture = texture;
    result.widthPixels = widthPixels;
    result.heightPixels = heightPixels;
    result.anchorOffsetX = (static_cast<std::int32_t>(group.width) - 1) * 32 + appearance->displacementX.value_or(0);
    result.anchorOffsetY = (static_cast<std::int32_t>(group.height) - 1) * 32 + appearance->displacementY.value_or(0);
    return result;
}

void LegacySpriteTextureCache::clear() noexcept {
    for (auto& [key, cached] : cache_) {
        (void)key;
        if (cached.texture != nullptr) SDL_DestroyTexture(cached.texture);
        cached.texture = nullptr;
    }
    cache_.clear();
}

} // namespace fantasy::studio::rendering
