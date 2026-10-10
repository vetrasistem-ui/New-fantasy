#include "Shared/Assets/Legacy/SprReader.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>

namespace fantasy::assets::legacy {
namespace {

std::uint16_t readU16(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset + 2 > bytes.size()) {
        throw std::runtime_error("SPR truncated while reading uint16");
    }
    return static_cast<std::uint16_t>(bytes[offset]) |
           (static_cast<std::uint16_t>(bytes[offset + 1]) << 8U);
}

std::uint32_t readU32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset + 4 > bytes.size()) {
        throw std::runtime_error("SPR truncated while reading uint32");
    }
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

std::vector<std::uint8_t> readFile(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        throw std::runtime_error("Unable to open SPR file: " + path.string());
    }
    const std::streamsize size = stream.tellg();
    if (size < 0) {
        throw std::runtime_error("Unable to determine SPR file size: " + path.string());
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    stream.seekg(0, std::ios::beg);
    if (!bytes.empty() && !stream.read(reinterpret_cast<char*>(bytes.data()), size)) {
        throw std::runtime_error("Unable to read SPR file: " + path.string());
    }
    return bytes;
}

bool structurallyValidSprite(
    const std::vector<std::uint8_t>& bytes,
    std::uint32_t spriteOffset,
    std::uint8_t colorChannels) noexcept {

    if (spriteOffset == 0 || colorChannels < 3 || colorChannels > 4) return false;
    std::size_t cursor = spriteOffset;
    if (cursor + 5 > bytes.size()) return false;

    cursor += 3; // transparent-color marker
    const std::uint16_t encodedBytes =
        static_cast<std::uint16_t>(bytes[cursor]) |
        (static_cast<std::uint16_t>(bytes[cursor + 1]) << 8U);
    cursor += 2;

    const std::size_t encodedEnd = cursor + encodedBytes;
    if (encodedEnd > bytes.size()) return false;

    constexpr std::size_t pixelCount = SpriteRgba::Width * SpriteRgba::Height;
    std::size_t pixelIndex = 0;
    while (cursor < encodedEnd) {
        if (cursor + 4 > encodedEnd) return false;

        const std::uint16_t transparentPixels =
            static_cast<std::uint16_t>(bytes[cursor]) |
            (static_cast<std::uint16_t>(bytes[cursor + 1]) << 8U);
        const std::uint16_t coloredPixels =
            static_cast<std::uint16_t>(bytes[cursor + 2]) |
            (static_cast<std::uint16_t>(bytes[cursor + 3]) << 8U);
        cursor += 4;

        if (pixelIndex + transparentPixels > pixelCount) return false;
        pixelIndex += transparentPixels;

        const std::size_t colorBytes = static_cast<std::size_t>(coloredPixels) * colorChannels;
        if (cursor + colorBytes > encodedEnd) return false;
        if (pixelIndex + coloredPixels > pixelCount) return false;

        cursor += colorBytes;
        pixelIndex += coloredPixels;
    }

    return cursor == encodedEnd;
}

std::uint8_t detectColorChannels(
    const std::vector<std::uint8_t>& bytes,
    const std::vector<std::uint32_t>& offsets) noexcept {

    std::size_t rgbScore = 0;
    std::size_t rgbaScore = 0;
    std::size_t sampled = 0;
    constexpr std::size_t sampleLimit = 64;

    for (const std::uint32_t offset : offsets) {
        if (offset == 0) continue;
        if (structurallyValidSprite(bytes, offset, 3)) ++rgbScore;
        if (structurallyValidSprite(bytes, offset, 4)) ++rgbaScore;
        if (++sampled >= sampleLimit) break;
    }

    // Preserve classic RGB behavior on ties. A true RGBA SPR normally makes
    // the RGB interpretation fail quickly because one alpha byte remains per
    // colored pixel and corrupts the following RLE segment boundary.
    return rgbaScore > rgbScore ? 4U : 3U;
}

} // namespace

SprReader::SprReader(const std::filesystem::path& path)
    : bytes_(readFile(path)) {
    if (bytes_.size() < 8) {
        throw std::runtime_error("SPR header is truncated");
    }

    info_.signature = readU32(bytes_, 0);
    info_.spriteCount = readU32(bytes_, 4);

    const std::uint64_t tableBytes = static_cast<std::uint64_t>(info_.spriteCount) * 4ULL;
    const std::uint64_t tableEnd = 8ULL + tableBytes;
    if (tableEnd > bytes_.size()) {
        throw std::runtime_error("SPR offset table is truncated");
    }

    offsets_.reserve(info_.spriteCount);
    for (std::uint32_t index = 0; index < info_.spriteCount; ++index) {
        const std::uint32_t offset = readU32(bytes_, 8ULL + static_cast<std::uint64_t>(index) * 4ULL);
        if (offset != 0 && offset >= bytes_.size()) {
            throw std::runtime_error("SPR contains an out-of-range sprite offset");
        }
        offsets_.push_back(offset);
    }

    info_.colorChannels = detectColorChannels(bytes_, offsets_);
}

const SprInfo& SprReader::info() const noexcept {
    return info_;
}

bool SprReader::hasSprite(std::uint32_t spriteId) const noexcept {
    return spriteId >= 1 && spriteId <= info_.spriteCount && offsets_[spriteId - 1] != 0;
}

SpriteRgba SprReader::readSprite(std::uint32_t spriteId) const {
    if (spriteId < 1 || spriteId > info_.spriteCount) {
        throw std::out_of_range("SPR sprite id is outside the declared range");
    }

    SpriteRgba result;
    const std::uint32_t spriteOffset = offsets_[spriteId - 1];
    if (spriteOffset == 0) {
        return result;
    }

    std::size_t cursor = spriteOffset;
    // Legacy SPR records begin with a three-byte transparent-color marker.
    if (cursor + 5 > bytes_.size()) {
        throw std::runtime_error("SPR sprite record header is truncated");
    }
    cursor += 3;
    const std::uint16_t encodedBytes = readU16(bytes_, cursor);
    cursor += 2;

    const std::size_t encodedEnd = cursor + encodedBytes;
    if (encodedEnd > bytes_.size()) {
        throw std::runtime_error("SPR sprite RLE payload is truncated");
    }

    std::size_t pixelIndex = 0;
    constexpr std::size_t pixelCount = SpriteRgba::Width * SpriteRgba::Height;
    while (cursor < encodedEnd) {
        if (cursor + 4 > encodedEnd) {
            throw std::runtime_error("SPR RLE segment header is truncated");
        }
        const std::uint16_t transparentPixels = readU16(bytes_, cursor);
        const std::uint16_t coloredPixels = readU16(bytes_, cursor + 2);
        cursor += 4;

        if (pixelIndex + transparentPixels > pixelCount) {
            throw std::runtime_error("SPR RLE transparent run exceeds 32x32 bounds");
        }
        pixelIndex += transparentPixels;

        const std::size_t colorBytes =
            static_cast<std::size_t>(coloredPixels) * static_cast<std::size_t>(info_.colorChannels);
        if (cursor + colorBytes > encodedEnd) {
            throw std::runtime_error("SPR RLE colored run is truncated");
        }
        if (pixelIndex + coloredPixels > pixelCount) {
            throw std::runtime_error("SPR RLE colored run exceeds 32x32 bounds");
        }

        for (std::uint16_t pixel = 0; pixel < coloredPixels; ++pixel) {
            const std::size_t rgba = pixelIndex * 4U;
            result.pixels[rgba + 0] = bytes_[cursor + 0];
            result.pixels[rgba + 1] = bytes_[cursor + 1];
            result.pixels[rgba + 2] = bytes_[cursor + 2];
            result.pixels[rgba + 3] = info_.colorChannels == 4U ? bytes_[cursor + 3] : 255U;
            cursor += info_.colorChannels;
            ++pixelIndex;
        }
    }

    if (cursor != encodedEnd) {
        throw std::runtime_error("SPR RLE decoder ended at an invalid position");
    }
    return result;
}

} // namespace fantasy::assets::legacy
