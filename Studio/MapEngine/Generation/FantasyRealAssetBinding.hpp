#pragma once

#include "../../Foundation/FantasyAssetCatalog.hpp"
#include "../../../Shared/Assets/LegacyAssetRegistry.hpp"
#include "../../../Shared/Assets/Legacy/SprReader.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fantasy::studio::mapgen {

struct RealAssetBindingDefinition {
    std::string id;
    std::uint32_t serverId = 0;
    std::string familyId;
    std::string role;
    std::vector<std::string> tags;
    std::uint32_t weight = 1;

    void validate() const {
        foundation::requireIdentifier(id, "real asset binding id");
        if (serverId == 0U) throw std::invalid_argument("real asset binding serverId must be non-zero");
        if (!familyId.empty()) foundation::requireIdentifier(familyId, "real asset binding family id");
        if (!role.empty()) foundation::requireIdentifier(role, "real asset binding role");
        if (weight == 0U) throw std::invalid_argument("real asset binding weight must be greater than zero");

        std::set<std::string> seen;
        for (const auto& tag : tags) {
            foundation::requireIdentifier(tag, "real asset binding tag");
            if (!seen.insert(tag).second) {
                throw std::invalid_argument("duplicate real asset binding tag: " + tag);
            }
        }
    }
};

struct RealAssetBindingManifest {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::string profileId;
    std::vector<foundation::AssetFamilyDefinition> families;
    std::vector<RealAssetBindingDefinition> bindings;

    void validate() const {
        foundation::requireIdentifier(profileId, "real asset binding profile id");

        std::set<std::string> familyIds;
        for (const auto& family : families) {
            family.validate();
            if (!familyIds.insert(family.id).second) {
                throw std::invalid_argument("duplicate real asset binding family id: " + family.id);
            }
        }

        std::set<std::string> bindingIds;
        std::set<std::uint32_t> serverIds;
        for (const auto& binding : bindings) {
            binding.validate();
            if (!bindingIds.insert(binding.id).second) {
                throw std::invalid_argument("duplicate real asset binding id: " + binding.id);
            }
            if (!serverIds.insert(binding.serverId).second) {
                throw std::invalid_argument("duplicate real asset binding serverId: " + std::to_string(binding.serverId));
            }
            if (!binding.familyId.empty() && !familyIds.contains(binding.familyId)) {
                throw std::invalid_argument("real asset binding references missing family: " + binding.familyId);
            }
        }
    }
};

class RealAssetBindingManifestStore {
public:
    [[nodiscard]] static RealAssetBindingManifest load(const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        if (!input.good()) throw std::runtime_error("unable to read real asset binding manifest: " + path.string());

        nlohmann::json json;
        input >> json;
        if (!json.contains("schemaVersion") ||
            json.at("schemaVersion").get<std::uint32_t>() != RealAssetBindingManifest::SchemaVersion) {
            throw std::runtime_error("unsupported real asset binding manifest schema version");
        }

        RealAssetBindingManifest manifest;
        json.at("profileId").get_to(manifest.profileId);

        if (json.contains("families")) {
            for (const auto& source : json.at("families")) {
                foundation::AssetFamilyDefinition family;
                source.at("id").get_to(family.id);
                source.at("name").get_to(family.name);
                if (source.contains("tags")) source.at("tags").get_to(family.tags);
                if (source.contains("brushRef") && !source.at("brushRef").is_null()) {
                    family.brushRef = source.at("brushRef").get<std::string>();
                }
                manifest.families.push_back(std::move(family));
            }
        }

        for (const auto& source : json.at("bindings")) {
            RealAssetBindingDefinition binding;
            source.at("id").get_to(binding.id);
            source.at("serverId").get_to(binding.serverId);
            if (source.contains("familyId")) source.at("familyId").get_to(binding.familyId);
            if (source.contains("role")) source.at("role").get_to(binding.role);
            if (source.contains("tags")) source.at("tags").get_to(binding.tags);
            if (source.contains("weight")) source.at("weight").get_to(binding.weight);
            manifest.bindings.push_back(std::move(binding));
        }

        manifest.validate();
        return manifest;
    }
};

struct RealAssetBindingReport {
    bool success = false;
    std::size_t bound = 0;
    std::vector<std::string> messages;
    std::vector<std::string> errors;
};

struct RealAssetBindingResult {
    foundation::FantasyAssetCatalog catalog;
    RealAssetBindingReport report;
};

class FantasyRealAssetBinder {
public:
    [[nodiscard]] static RealAssetBindingResult bind(
        const RealAssetBindingManifest& manifest,
        const fantasy::assets::FantasyAssetRegistry& registry,
        const fantasy::assets::legacy::SprReader* sprites = nullptr) {

        RealAssetBindingResult result;
        result.catalog.profileId = manifest.profileId;
        result.catalog.families = manifest.families;

        try {
            manifest.validate();

            for (const auto& binding : manifest.bindings) {
                const auto* record = registry.findByLegacyServerId(binding.serverId);
                if (record == nullptr) {
                    result.report.errors.push_back(
                        binding.id + ": serverId " + std::to_string(binding.serverId) + " is not present in the real legacy registry");
                    continue;
                }

                if (record->clientId == 0U) {
                    result.report.errors.push_back(binding.id + ": resolved legacy record has no clientId");
                    continue;
                }
                if (record->spriteIds.empty()) {
                    result.report.errors.push_back(binding.id + ": resolved legacy record has no sprite ids");
                    continue;
                }

                if (sprites != nullptr) {
                    bool allSpritesExist = true;
                    for (const auto spriteId : record->spriteIds) {
                        if (spriteId == 0U || !sprites->hasSprite(spriteId)) {
                            result.report.errors.push_back(
                                binding.id + ": spriteId " + std::to_string(spriteId) + " is missing from the supplied SPR file");
                            allSpritesExist = false;
                            break;
                        }
                    }
                    if (!allSpritesExist) continue;
                }

                foundation::AssetCatalogEntry entry;
                entry.id = binding.id;
                entry.source = foundation::AssetCatalogSourceKind::LegacyRegistry;
                entry.assetRef = record->semanticKey;
                entry.familyId = binding.familyId;
                entry.role = binding.role;
                entry.tags = binding.tags;
                entry.weight = binding.weight;
                result.catalog.entries.push_back(std::move(entry));

                ++result.report.bound;
                result.report.messages.push_back(
                    binding.id + " -> serverId=" + std::to_string(record->serverId) +
                    " clientId=" + std::to_string(record->clientId) +
                    " sprites=" + std::to_string(record->spriteIds.size()));
            }

            if (result.report.errors.empty()) {
                result.catalog.validate();
                const auto referenceErrors = result.catalog.validateLegacyReferences(registry);
                result.report.errors.insert(
                    result.report.errors.end(), referenceErrors.begin(), referenceErrors.end());
            }
        } catch (const std::exception& error) {
            result.report.errors.push_back(error.what());
        }

        result.report.success = result.report.errors.empty() && result.report.bound == manifest.bindings.size();
        return result;
    }
};

} // namespace fantasy::studio::mapgen
