#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fantasy::assets {

enum class LegacyAssetKind {
    Unknown,
    Ground,
    Border,
    Object,
    Creature,
    Effect,
};

struct LegacyAssetProfile {
    std::string id;
    std::string family;
    std::string clientVersion;
    std::filesystem::path datPath;
    std::filesystem::path sprPath;
    std::filesystem::path otbPath;

    [[nodiscard]] bool usesRelativePathsOnly() const;
};

struct LegacyAssetRecord {
    std::string semanticKey;
    LegacyAssetKind kind = LegacyAssetKind::Unknown;
    std::uint32_t serverId = 0;
    std::uint32_t clientId = 0;
    std::vector<std::uint32_t> spriteIds;
};

class FantasyAssetRegistry {
public:
    [[nodiscard]] bool registerLegacyAsset(LegacyAssetRecord record, std::string* error = nullptr);

    [[nodiscard]] const LegacyAssetRecord* findBySemanticKey(const std::string& semanticKey) const;
    [[nodiscard]] const LegacyAssetRecord* findByLegacyServerId(std::uint32_t serverId) const;
    [[nodiscard]] std::size_t size() const noexcept;

    [[nodiscard]] static std::string bootstrapSemanticKey(
        const std::string& profileId,
        std::uint32_t serverId);

private:
    std::unordered_map<std::string, LegacyAssetRecord> bySemanticKey_;
    std::unordered_map<std::uint32_t, std::string> semanticKeyByServerId_;
};

[[nodiscard]] LegacyAssetProfile makePokeFans1098Profile();

} // namespace fantasy::assets
