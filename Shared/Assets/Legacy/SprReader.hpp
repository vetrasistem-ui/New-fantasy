#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace fantasy::assets::legacy {

struct SprInfo {
    std::uint32_t signature = 0;
    std::uint32_t spriteCount = 0;
    std::uint8_t colorChannels = 3;
};

struct SpriteRgba {
    static constexpr std::uint32_t Width = 32;
    static constexpr std::uint32_t Height = 32;
    std::array<std::uint8_t, Width * Height * 4> pixels{};
};

class SprReader {
public:
    explicit SprReader(const std::filesystem::path& path);

    [[nodiscard]] const SprInfo& info() const noexcept;
    [[nodiscard]] bool hasSprite(std::uint32_t spriteId) const noexcept;
    [[nodiscard]] SpriteRgba readSprite(std::uint32_t spriteId) const;

private:
    std::vector<std::uint8_t> bytes_;
    SprInfo info_;
    std::vector<std::uint32_t> offsets_;
};

} // namespace fantasy::assets::legacy
