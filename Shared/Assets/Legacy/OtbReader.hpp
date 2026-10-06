#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fantasy::assets::legacy {

struct OtbVersionInfo {
    std::uint32_t major = 0;
    std::uint32_t minor = 0;
    std::uint32_t build = 0;
    std::string description;
};

struct OtbAttribute {
    std::uint8_t id = 0;
    std::vector<std::uint8_t> value;
};

struct OtbItemRecord {
    std::uint8_t group = 0;
    std::uint32_t flags = 0;
    std::optional<std::uint16_t> serverId;
    std::optional<std::uint16_t> clientId;
    std::optional<std::array<std::uint8_t, 16>> spriteHash;
    std::vector<OtbAttribute> attributes;
};

class OtbReader {
public:
    explicit OtbReader(const std::filesystem::path& path);

    [[nodiscard]] const OtbVersionInfo& version() const noexcept;
    [[nodiscard]] const std::vector<OtbItemRecord>& items() const noexcept;
    [[nodiscard]] const OtbItemRecord* findByServerId(std::uint16_t serverId) const noexcept;

private:
    OtbVersionInfo version_;
    std::vector<OtbItemRecord> items_;
};

} // namespace fantasy::assets::legacy
