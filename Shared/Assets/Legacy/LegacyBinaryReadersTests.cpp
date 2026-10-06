#include "Shared/Assets/Legacy/OtbReader.hpp"
#include "Shared/Assets/Legacy/SprReader.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using fantasy::assets::legacy::OtbReader;
using fantasy::assets::legacy::SprReader;

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

void appendEscaped(std::vector<std::uint8_t>& out, std::uint8_t value) {
    if (value == 0xFD || value == 0xFE || value == 0xFF) {
        out.push_back(0xFD);
    }
    out.push_back(value);
}

void appendEscapedBlock(std::vector<std::uint8_t>& out, const std::vector<std::uint8_t>& block) {
    for (const std::uint8_t value : block) {
        appendEscaped(out, value);
    }
}

void writeBinary(const fs::path& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    assert(stream.good());
    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    assert(stream.good());
}

std::vector<std::uint8_t> makeSyntheticSpr() {
    std::vector<std::uint8_t> bytes;
    appendU32(bytes, 0x57BBD603U);
    appendU32(bytes, 2U);
    appendU32(bytes, 0U); // sprite 1 intentionally empty
    appendU32(bytes, 16U); // sprite 2 starts immediately after the offset table

    bytes.push_back(255U); // transparent-color marker, not used by decoder
    bytes.push_back(0U);
    bytes.push_back(255U);
    appendU16(bytes, 7U); // one RLE segment: 1023 transparent + 1 RGB pixel
    appendU16(bytes, 1023U);
    appendU16(bytes, 1U);
    bytes.push_back(255U);
    bytes.push_back(0U);
    bytes.push_back(0U);
    return bytes;
}

std::vector<std::uint8_t> makeSyntheticOtb() {
    std::vector<std::uint8_t> bytes(4, 0U); // identifier
    bytes.push_back(0xFE);
    bytes.push_back(0x00); // root type

    std::vector<std::uint8_t> rootProperties(4, 0U); // root flags
    rootProperties.push_back(0x01); // VERSION attribute
    appendU16(rootProperties, 140U);
    appendU32(rootProperties, 3U);
    appendU32(rootProperties, 57U);
    appendU32(rootProperties, 63U);
    std::array<std::uint8_t, 128> description{};
    const std::string text = "OTB 3.57.63-10.98";
    std::copy(text.begin(), text.end(), description.begin());
    rootProperties.insert(rootProperties.end(), description.begin(), description.end());
    appendEscapedBlock(bytes, rootProperties);

    bytes.push_back(0xFE);
    bytes.push_back(0x01); // item group
    std::vector<std::uint8_t> itemProperties;
    appendU32(itemProperties, 0x02000003U);
    itemProperties.push_back(0x10); appendU16(itemProperties, 2U); appendU16(itemProperties, 100U);
    itemProperties.push_back(0x11); appendU16(itemProperties, 2U); appendU16(itemProperties, 200U);
    itemProperties.push_back(0x20); appendU16(itemProperties, 16U);
    for (std::uint8_t i = 0; i < 16; ++i) itemProperties.push_back(i);
    itemProperties.push_back(0x2A); appendU16(itemProperties, 4U); appendU32(itemProperties, 0xFEFFFD01U);
    appendEscapedBlock(bytes, itemProperties);
    bytes.push_back(0xFF); // item end
    bytes.push_back(0xFF); // root end
    return bytes;
}

} // namespace

int main() {
    const fs::path root = fs::temp_directory_path() / "fantasy-legacy-reader-tests";
    fs::create_directories(root);
    const fs::path sprPath = root / "synthetic.spr";
    const fs::path otbPath = root / "synthetic.otb";
    writeBinary(sprPath, makeSyntheticSpr());
    writeBinary(otbPath, makeSyntheticOtb());

    {
        const SprReader reader(sprPath);
        assert(reader.info().signature == 0x57BBD603U);
        assert(reader.info().spriteCount == 2U);
        assert(!reader.hasSprite(1U));
        assert(reader.hasSprite(2U));
        const auto empty = reader.readSprite(1U);
        for (const std::uint8_t value : empty.pixels) assert(value == 0U);
        const auto sprite = reader.readSprite(2U);
        const std::size_t last = (32U * 32U - 1U) * 4U;
        assert(sprite.pixels[last + 0] == 255U);
        assert(sprite.pixels[last + 1] == 0U);
        assert(sprite.pixels[last + 2] == 0U);
        assert(sprite.pixels[last + 3] == 255U);
    }

    {
        const OtbReader reader(otbPath);
        assert(reader.version().major == 3U);
        assert(reader.version().minor == 57U);
        assert(reader.version().build == 63U);
        assert(reader.version().description == "OTB 3.57.63-10.98");
        assert(reader.items().size() == 1U);
        const auto* item = reader.findByServerId(100U);
        assert(item != nullptr);
        assert(item->clientId.has_value() && *item->clientId == 200U);
        assert(item->spriteHash.has_value());
        assert((*item->spriteHash)[15] == 15U);
        assert(item->attributes.size() == 4U);
        assert(item->attributes.back().id == 0x2AU);
        assert(item->attributes.back().value.size() == 4U);
        assert(item->attributes.back().value[0] == 0x01U);
        assert(item->attributes.back().value[1] == 0xFDU);
        assert(item->attributes.back().value[2] == 0xFFU);
        assert(item->attributes.back().value[3] == 0xFEU);
    }

    std::error_code ignored;
    fs::remove_all(root, ignored);
    return 0;
}
