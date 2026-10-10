#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fantasy::assets::legacy {

struct DatHeader {
    std::uint32_t signature = 0;
    std::uint16_t itemMaxId = 0;
    std::uint16_t creatureCount = 0;
    std::uint16_t effectCount = 0;
    std::uint16_t distanceCount = 0;
};

struct DatAnimationFrameDuration {
    std::uint32_t minimumMs = 0;
    std::uint32_t maximumMs = 0;
};

struct DatFrameGroup {
    std::optional<std::uint8_t> groupType;
    std::uint8_t width = 0;
    std::uint8_t height = 0;
    std::optional<std::uint8_t> exactSize;
    std::uint8_t layers = 0;
    std::uint8_t patternX = 0;
    std::uint8_t patternY = 0;
    std::uint8_t patternZ = 0;
    std::uint8_t frames = 0;
    std::optional<std::uint8_t> animationAsync;
    std::optional<std::int32_t> animationLoopCount;
    std::optional<std::int8_t> animationStartFrame;
    std::vector<DatAnimationFrameDuration> frameDurations;
    std::vector<std::uint32_t> spriteIds;
};

struct DatAppearance {
    std::uint32_t id = 0;
    bool creature = false;
    std::vector<std::uint8_t> rawFlags;
    std::optional<std::uint16_t> groundSpeed;
    std::optional<std::uint16_t> displacementX;
    std::optional<std::uint16_t> displacementY;
    std::optional<std::uint16_t> elevation;
    std::optional<std::uint16_t> minimapColor;
    std::optional<std::string> marketName;
    std::vector<DatFrameGroup> frameGroups;
};

class DatReader1057 {
public:
    explicit DatReader1057(const std::filesystem::path& path);

    [[nodiscard]] const DatHeader& header() const noexcept;
    [[nodiscard]] const std::vector<DatAppearance>& appearances() const noexcept;
    [[nodiscard]] const DatAppearance* findItem(std::uint32_t clientId) const noexcept;

private:
    DatHeader header_;
    std::vector<DatAppearance> appearances_;
};

} // namespace fantasy::assets::legacy
