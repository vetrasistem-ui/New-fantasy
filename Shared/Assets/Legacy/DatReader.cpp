#include "Shared/Assets/Legacy/DatReader.hpp"

#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace fantasy::assets::legacy {
namespace {

constexpr std::uint8_t kDatFlagLast = 255;
constexpr std::uint8_t kDatFlagNoMoveAnimation = 253;

constexpr std::uint8_t kGround = 0;
constexpr std::uint8_t kWritable = 8;
constexpr std::uint8_t kWritableOnce = 9;
constexpr std::uint8_t kLight = 21;
constexpr std::uint8_t kDisplacement = 24;
constexpr std::uint8_t kElevation = 25;
constexpr std::uint8_t kMinimapColor = 28;
constexpr std::uint8_t kLensHelp = 29;
constexpr std::uint8_t kCloth = 32;
constexpr std::uint8_t kMarket = 33;
constexpr std::uint8_t kUsable = 34;

std::vector<std::uint8_t> readFile(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        throw std::runtime_error("Unable to open DAT file: " + path.string());
    }
    const std::streamsize size = stream.tellg();
    if (size < 0) {
        throw std::runtime_error("Unable to determine DAT file size: " + path.string());
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    stream.seekg(0, std::ios::beg);
    if (!bytes.empty() && !stream.read(reinterpret_cast<char*>(bytes.data()), size)) {
        throw std::runtime_error("Unable to read DAT file: " + path.string());
    }
    return bytes;
}

class Cursor {
public:
    explicit Cursor(const std::vector<std::uint8_t>& bytes) : bytes_(bytes) {}

    std::uint8_t u8(const char* context) {
        require(1, context);
        return bytes_[offset_++];
    }

    std::int8_t i8(const char* context) {
        return static_cast<std::int8_t>(u8(context));
    }

    std::uint16_t u16(const char* context) {
        require(2, context);
        const std::uint16_t result = static_cast<std::uint16_t>(bytes_[offset_]) |
                                     (static_cast<std::uint16_t>(bytes_[offset_ + 1]) << 8U);
        offset_ += 2;
        return result;
    }

    std::uint32_t u32(const char* context) {
        require(4, context);
        const std::uint32_t result = static_cast<std::uint32_t>(bytes_[offset_]) |
                                     (static_cast<std::uint32_t>(bytes_[offset_ + 1]) << 8U) |
                                     (static_cast<std::uint32_t>(bytes_[offset_ + 2]) << 16U) |
                                     (static_cast<std::uint32_t>(bytes_[offset_ + 3]) << 24U);
        offset_ += 4;
        return result;
    }

    std::int32_t i32(const char* context) {
        return static_cast<std::int32_t>(u32(context));
    }

    std::string string16(const char* context) {
        const std::uint16_t size = u16(context);
        require(size, context);
        const auto begin = bytes_.begin() + static_cast<std::ptrdiff_t>(offset_);
        const auto end = begin + static_cast<std::ptrdiff_t>(size);
        std::string result(begin, end);
        offset_ += size;
        return result;
    }

    void skip(std::size_t size, const char* context) {
        require(size, context);
        offset_ += size;
    }

private:
    void require(std::size_t size, const char* context) const {
        if (offset_ > bytes_.size() || size > bytes_.size() - offset_) {
            throw std::runtime_error(std::string("DAT truncated while reading ") + context);
        }
    }

    const std::vector<std::uint8_t>& bytes_;
    std::size_t offset_ = 0;
};

std::uint8_t normalizeFlag1057(std::uint8_t encoded) {
    // 10.10+ inserted No Movement Animation at encoded flag 16. All later
    // ordinary flags shifted by one byte. This is the same normalization used
    // by the 10.57-era Remere parser.
    if (encoded == 16) {
        return kDatFlagNoMoveAnimation;
    }
    if (encoded > 16 && encoded < kDatFlagLast) {
        return static_cast<std::uint8_t>(encoded - 1U);
    }
    return encoded;
}

void parseFlagPayload(Cursor& cursor, std::uint8_t flag, DatAppearance& appearance) {
    switch (flag) {
        case kGround:
            appearance.groundSpeed = cursor.u16("ground speed");
            return;
        case kWritable:
            cursor.skip(2, "writable max text length");
            return;
        case kWritableOnce:
            cursor.skip(2, "writable-once max text length");
            return;
        case kLight:
            cursor.skip(4, "light payload");
            return;
        case kDisplacement:
            appearance.displacementX = cursor.u16("displacement x");
            appearance.displacementY = cursor.u16("displacement y");
            return;
        case kElevation:
            appearance.elevation = cursor.u16("elevation");
            return;
        case kMinimapColor:
            appearance.minimapColor = cursor.u16("minimap color");
            return;
        case kLensHelp:
            cursor.skip(2, "lens-help payload");
            return;
        case kCloth:
            cursor.skip(2, "cloth payload");
            return;
        case kMarket:
            cursor.skip(6, "market category/trade payload");
            appearance.marketName = cursor.string16("market name");
            cursor.skip(4, "market restriction payload");
            return;
        case kUsable:
            cursor.skip(2, "usable payload");
            return;
        default:
            // All remaining normalized 10.57 flags are marker-only. Reject
            // values outside the known range so a new/unknown layout cannot
            // silently desynchronize the rest of the file.
            if ((flag >= 1 && flag <= 37) || flag == kDatFlagNoMoveAnimation) {
                return;
            }
            throw std::runtime_error("DAT 10.57 contains an unsupported metadata flag");
    }
}

std::size_t checkedSpriteCount(const DatFrameGroup& group) {
    std::size_t count = 1;
    const std::uint8_t dimensions[] = {
        group.width,
        group.height,
        group.layers,
        group.patternX,
        group.patternY,
        group.patternZ,
        group.frames,
    };
    for (const std::uint8_t value : dimensions) {
        if (value == 0) {
            throw std::runtime_error("DAT 10.57 frame group contains a zero dimension");
        }
        if (count > std::numeric_limits<std::size_t>::max() / value) {
            throw std::runtime_error("DAT 10.57 sprite count overflows size_t");
        }
        count *= value;
    }
    return count;
}

DatAppearance parseAppearance(Cursor& cursor, std::uint32_t id, bool creature) {
    DatAppearance appearance;
    appearance.id = id;
    appearance.creature = creature;

    bool terminated = false;
    for (std::size_t guard = 0; guard < 255; ++guard) {
        const std::uint8_t encoded = cursor.u8("metadata flag");
        if (encoded == kDatFlagLast) {
            terminated = true;
            break;
        }
        appearance.rawFlags.push_back(encoded);
        parseFlagPayload(cursor, normalizeFlag1057(encoded), appearance);
    }
    if (!terminated) {
        throw std::runtime_error("DAT 10.57 metadata flags are missing the 0xFF terminator");
    }

    const std::uint8_t groupCount = creature ? cursor.u8("creature frame-group count") : 1U;
    if (groupCount == 0) {
        throw std::runtime_error("DAT 10.57 appearance has zero frame groups");
    }
    appearance.frameGroups.reserve(groupCount);

    for (std::uint8_t groupIndex = 0; groupIndex < groupCount; ++groupIndex) {
        DatFrameGroup group;
        if (creature) {
            group.groupType = cursor.u8("frame-group type");
        }
        group.width = cursor.u8("sprite width");
        group.height = cursor.u8("sprite height");
        if (group.width > 1 || group.height > 1) {
            group.exactSize = cursor.u8("sprite exact size");
        }
        group.layers = cursor.u8("sprite layers");
        group.patternX = cursor.u8("sprite pattern x");
        group.patternY = cursor.u8("sprite pattern y");
        group.patternZ = cursor.u8("sprite pattern z");
        group.frames = cursor.u8("sprite frame count");

        if (group.frames > 1) {
            group.animationAsync = cursor.u8("animation async flag");
            group.animationLoopCount = cursor.i32("animation loop count");
            group.animationStartFrame = cursor.i8("animation start frame");
            group.frameDurations.reserve(group.frames);
            for (std::uint8_t frame = 0; frame < group.frames; ++frame) {
                DatAnimationFrameDuration duration;
                duration.minimumMs = cursor.u32("animation frame minimum duration");
                duration.maximumMs = cursor.u32("animation frame maximum duration");
                if (duration.minimumMs > duration.maximumMs) {
                    throw std::runtime_error("DAT 10.57 frame duration minimum exceeds maximum");
                }
                group.frameDurations.push_back(duration);
            }
        }

        const std::size_t spriteCount = checkedSpriteCount(group);
        group.spriteIds.reserve(spriteCount);
        for (std::size_t sprite = 0; sprite < spriteCount; ++sprite) {
            group.spriteIds.push_back(cursor.u32("sprite id"));
        }
        appearance.frameGroups.push_back(std::move(group));
    }

    return appearance;
}

} // namespace

DatReader1057::DatReader1057(const std::filesystem::path& path) {
    const std::vector<std::uint8_t> bytes = readFile(path);
    if (bytes.size() < 12) {
        throw std::runtime_error("DAT header is truncated");
    }

    Cursor cursor(bytes);
    header_.signature = cursor.u32("signature");
    header_.itemMaxId = cursor.u16("item max id");
    header_.creatureCount = cursor.u16("creature count");
    header_.effectCount = cursor.u16("effect count");
    header_.distanceCount = cursor.u16("distance count");

    if (header_.itemMaxId < 100) {
        throw std::runtime_error("DAT 10.57 item max id is below the first valid item id (100)");
    }

    const std::uint32_t firstId = 100U;
    const std::uint32_t finalId = static_cast<std::uint32_t>(header_.itemMaxId) +
                                  static_cast<std::uint32_t>(header_.creatureCount);
    appearances_.reserve(static_cast<std::size_t>(finalId - firstId + 1U));
    for (std::uint32_t id = firstId; id <= finalId; ++id) {
        appearances_.push_back(parseAppearance(cursor, id, id > header_.itemMaxId));
    }

    // Effects and distance effects are intentionally not interpreted by the
    // first F05.5 bridge. Their counts stay in DatHeader so later support can
    // be added without changing the item/creature contract.
}

const DatHeader& DatReader1057::header() const noexcept {
    return header_;
}

const std::vector<DatAppearance>& DatReader1057::appearances() const noexcept {
    return appearances_;
}

const DatAppearance* DatReader1057::findItem(std::uint32_t clientId) const noexcept {
    if (clientId < 100U || clientId > header_.itemMaxId) {
        return nullptr;
    }
    const std::size_t index = static_cast<std::size_t>(clientId - 100U);
    return index < appearances_.size() ? &appearances_[index] : nullptr;
}

} // namespace fantasy::assets::legacy
