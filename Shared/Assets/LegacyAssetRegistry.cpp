#include "Shared/Assets/LegacyAssetRegistry.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace fantasy::assets {
namespace {

std::string normalizeProfileSegment(const std::string& value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (const unsigned char ch : value) {
        if (std::isalnum(ch) != 0) {
            normalized.push_back(static_cast<char>(std::tolower(ch)));
        } else if (!normalized.empty() && normalized.back() != '_') {
            normalized.push_back('_');
        }
    }
    while (!normalized.empty() && normalized.back() == '_') {
        normalized.pop_back();
    }
    return normalized;
}

void setError(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
}

} // namespace

bool LegacyAssetProfile::usesRelativePathsOnly() const {
    return datPath.is_relative() && sprPath.is_relative() && otbPath.is_relative();
}

bool FantasyAssetRegistry::registerLegacyAsset(LegacyAssetRecord record, std::string* error) {
    if (record.semanticKey.empty()) {
        setError(error, "semantic key is empty");
        return false;
    }
    if (record.serverId == 0) {
        setError(error, "legacy server id must be non-zero");
        return false;
    }
    if (bySemanticKey_.contains(record.semanticKey)) {
        setError(error, "semantic key is already registered: " + record.semanticKey);
        return false;
    }
    if (semanticKeyByServerId_.contains(record.serverId)) {
        setError(error, "legacy server id is already registered: " + std::to_string(record.serverId));
        return false;
    }

    const std::string key = record.semanticKey;
    const std::uint32_t serverId = record.serverId;
    bySemanticKey_.emplace(key, std::move(record));
    semanticKeyByServerId_.emplace(serverId, key);
    return true;
}

const LegacyAssetRecord* FantasyAssetRegistry::findBySemanticKey(const std::string& semanticKey) const {
    const auto it = bySemanticKey_.find(semanticKey);
    return it == bySemanticKey_.end() ? nullptr : &it->second;
}

const LegacyAssetRecord* FantasyAssetRegistry::findByLegacyServerId(std::uint32_t serverId) const {
    const auto keyIt = semanticKeyByServerId_.find(serverId);
    if (keyIt == semanticKeyByServerId_.end()) {
        return nullptr;
    }
    return findBySemanticKey(keyIt->second);
}

std::size_t FantasyAssetRegistry::size() const noexcept {
    return bySemanticKey_.size();
}

std::string FantasyAssetRegistry::bootstrapSemanticKey(
    const std::string& profileId,
    std::uint32_t serverId) {
    std::string segment = normalizeProfileSegment(profileId);
    if (segment.empty()) {
        segment = "legacy";
    }
    return "legacy." + segment + ".item." + std::to_string(serverId);
}

LegacyAssetProfile makePokeFans1098Profile() {
    LegacyAssetProfile profile;
    profile.id = "pokefans1098";
    profile.family = "PokeFans";
    profile.clientVersion = "10.98";
    profile.datPath = "Game/Assets/Legacy/PokeFans1098/Tibia.dat";
    profile.sprPath = "Game/Assets/Legacy/PokeFans1098/Tibia.spr";
    profile.otbPath = "Game/Assets/Legacy/PokeFans1098/items.otb";
    return profile;
}

} // namespace fantasy::assets
