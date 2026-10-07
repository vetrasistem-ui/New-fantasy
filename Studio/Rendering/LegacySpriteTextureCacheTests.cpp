#include "LegacySpriteTextureCache.hpp"

#include <SDL3/SDL.h>

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;
using fantasy::studio::rendering::LegacySpriteTextureCache;

namespace {

void appendU16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
}

void appendU32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
}

void writeBinary(const fs::path& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    assert(stream.good());
    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    assert(stream.good());
}

std::vector<std::uint8_t> makeSpr() {
    std::vector<std::uint8_t> bytes;
    appendU32(bytes, 0x57BBD603U);
    appendU32(bytes, 2U);
    appendU32(bytes, 0U);
    appendU32(bytes, 16U);

    bytes.push_back(255U);
    bytes.push_back(0U);
    bytes.push_back(255U);
    appendU16(bytes, 7U);
    appendU16(bytes, 1023U);
    appendU16(bytes, 1U);
    bytes.push_back(24U);
    bytes.push_back(200U);
    bytes.push_back(255U);
    return bytes;
}

std::vector<std::uint8_t> makeDat() {
    std::vector<std::uint8_t> bytes;
    appendU32(bytes, 0x000042A3U);
    appendU16(bytes, 100U);
    appendU16(bytes, 0U);
    appendU16(bytes, 0U);
    appendU16(bytes, 0U);

    bytes.push_back(0xFFU);
    bytes.push_back(1U);
    bytes.push_back(1U);
    bytes.push_back(1U);
    bytes.push_back(1U);
    bytes.push_back(1U);
    bytes.push_back(1U);
    bytes.push_back(1U);
    appendU32(bytes, 2U);
    return bytes;
}

} // namespace

int main() {
    const fs::path root = fs::temp_directory_path() / "fantasy-legacy-texture-tests";
    fs::create_directories(root);
    const fs::path datPath = root / "synthetic.dat";
    const fs::path sprPath = root / "synthetic.spr";
    writeBinary(datPath, makeDat());
    writeBinary(sprPath, makeSpr());

    SDL_Surface* surface = SDL_CreateSurface(64, 64, SDL_PIXELFORMAT_RGBA32);
    assert(surface != nullptr);
    SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
    assert(renderer != nullptr);

    {
        LegacySpriteTextureCache cache(renderer, datPath, sprPath);
        assert(cache.datHeader().signature == 0x42A3U);
        assert(cache.sprInfo().signature == 0x57BBD603U);
        assert(cache.sprInfo().spriteCount == 2U);

        const auto first = cache.textureForClientId(100U);
        assert(first.has_value());
        assert(first->texture != nullptr);
        assert(first->widthPixels == 32);
        assert(first->heightPixels == 32);
        assert(first->anchorOffsetX == 0);
        assert(first->anchorOffsetY == 0);
        assert(cache.cachedTextureCount() == 1U);

        const auto second = cache.textureForClientId(100U);
        assert(second.has_value());
        assert(second->texture == first->texture);
        assert(cache.cachedTextureCount() == 1U);

        const auto missing = cache.textureForClientId(999U);
        assert(!missing.has_value());
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);

    std::error_code ignored;
    fs::remove_all(root, ignored);
    return 0;
}
