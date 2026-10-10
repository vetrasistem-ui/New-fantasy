#pragma once

#include "Foundation/FantasyFoundationV2.hpp"
#include "Shared/Assets/LegacyAssetRegistry.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace fantasy::studio::foundation {

enum class AssetCatalogSourceKind {
    LegacyRegistry,
    ModernAsset,
};

[[nodiscard]] inline std::string assetCatalogSourceKindId(AssetCatalogSourceKind value) {
    switch (value) {
        case AssetCatalogSourceKind::LegacyRegistry: return "legacy_registry";
        case AssetCatalogSourceKind::ModernAsset: return "modern_asset";
    }
    throw std::invalid_argument("unknown Fantasy asset catalog source kind");
}

[[nodiscard]] inline AssetCatalogSourceKind parseAssetCatalogSourceKind(const std::string& value) {
    if (value == "legacy_registry") return AssetCatalogSourceKind::LegacyRegistry;
    if (value == "modern_asset") return AssetCatalogSourceKind::ModernAsset;
    throw std::invalid_argument("unknown Fantasy asset catalog source kind: " + value);
}

struct AssetCatalogEntry {
    std::string id;
    AssetCatalogSourceKind source = AssetCatalogSourceKind::LegacyRegistry;
    std::string assetRef;
    std::string familyId;
    std::string role;
    std::vector<std::string> tags;
    std::uint32_t weight = 1;

    void validate() const {
        requireIdentifier(id, "asset catalog entry id");
        requireIdentifier(assetRef, "asset catalog reference");
        if (!familyId.empty()) requireIdentifier(familyId, "asset catalog family id");
        if (!role.empty()) requireIdentifier(role, "asset catalog role");
        if (weight == 0) throw std::invalid_argument("Fantasy asset catalog entry weight must be greater than zero");

        std::set<std::string> seenTags;
        for (const auto& tag : tags) {
            requireIdentifier(tag, "asset catalog tag");
            if (!seenTags.insert(tag).second) {
                throw std::invalid_argument("duplicate Fantasy asset catalog tag: " + tag);
            }
        }
    }
};

struct AssetFamilyDefinition {
    std::string id;
    std::string name;
    std::vector<std::string> tags;
    std::optional<std::string> brushRef;

    void validate() const {
        requireIdentifier(id, "asset family id");
        if (name.empty()) throw std::invalid_argument("Fantasy asset family name is required");
        if (brushRef.has_value()) requireIdentifier(*brushRef, "asset family brush reference");

        std::set<std::string> seenTags;
        for (const auto& tag : tags) {
            requireIdentifier(tag, "asset family tag");
            if (!seenTags.insert(tag).second) {
                throw std::invalid_argument("duplicate Fantasy asset family tag: " + tag);
            }
        }
    }
};

struct FantasyAssetCatalog {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::string profileId;
    std::vector<AssetFamilyDefinition> families;
    std::vector<AssetCatalogEntry> entries;

    void validate() const {
        requireIdentifier(profileId, "asset catalog profile id");

        std::set<std::string> familyIds;
        for (const auto& family : families) {
            family.validate();
            if (!familyIds.insert(family.id).second) {
                throw std::invalid_argument("duplicate Fantasy asset family id: " + family.id);
            }
        }

        std::set<std::string> entryIds;
        for (const auto& entry : entries) {
            entry.validate();
            if (!entryIds.insert(entry.id).second) {
                throw std::invalid_argument("duplicate Fantasy asset catalog entry id: " + entry.id);
            }
            if (!entry.familyId.empty() && !familyIds.contains(entry.familyId)) {
                throw std::invalid_argument(
                    "Fantasy asset catalog entry references missing family: " + entry.familyId);
            }
        }
    }

    [[nodiscard]] const AssetCatalogEntry* findEntry(const std::string& id) const noexcept {
        const auto it = std::find_if(entries.begin(), entries.end(), [&](const auto& entry) {
            return entry.id == id;
        });
        return it == entries.end() ? nullptr : &*it;
    }

    [[nodiscard]] const AssetFamilyDefinition* findFamily(const std::string& id) const noexcept {
        const auto it = std::find_if(families.begin(), families.end(), [&](const auto& family) {
            return family.id == id;
        });
        return it == families.end() ? nullptr : &*it;
    }

    [[nodiscard]] std::vector<const AssetCatalogEntry*> findByTags(
        const std::vector<std::string>& requiredTags) const {

        for (const auto& tag : requiredTags) requireIdentifier(tag, "asset catalog query tag");
        std::vector<const AssetCatalogEntry*> result;
        for (const auto& entry : entries) {
            const bool matches = std::all_of(requiredTags.begin(), requiredTags.end(), [&](const auto& tag) {
                return std::find(entry.tags.begin(), entry.tags.end(), tag) != entry.tags.end();
            });
            if (matches) result.push_back(&entry);
        }
        return result;
    }

    [[nodiscard]] std::vector<const AssetCatalogEntry*> familyMembers(
        const std::string& familyId,
        const std::string& role = {}) const {

        requireIdentifier(familyId, "asset family query id");
        if (!role.empty()) requireIdentifier(role, "asset family query role");

        std::vector<const AssetCatalogEntry*> result;
        for (const auto& entry : entries) {
            if (entry.familyId != familyId) continue;
            if (!role.empty() && entry.role != role) continue;
            result.push_back(&entry);
        }
        return result;
    }

    [[nodiscard]] const AssetCatalogEntry* chooseFamilyMember(
        const std::string& familyId,
        const std::string& role,
        std::uint64_t seed) const {

        const auto candidates = familyMembers(familyId, role);
        if (candidates.empty()) return nullptr;

        std::uint64_t total = 0;
        for (const auto* candidate : candidates) total += candidate->weight;
        if (total == 0) return nullptr;

        // Same deterministic xorshift family used by FantasyBrushSelector so
        // procedural/AI authoring can reproduce the same selection from a seed.
        seed ^= seed >> 12;
        seed ^= seed << 25;
        seed ^= seed >> 27;
        const std::uint64_t value = (seed * 2685821657736338717ULL) % total;

        std::uint64_t cursor = 0;
        for (const auto* candidate : candidates) {
            cursor += candidate->weight;
            if (value < cursor) return candidate;
        }
        return candidates.back();
    }

    [[nodiscard]] std::vector<std::string> validateLegacyReferences(
        const fantasy::assets::FantasyAssetRegistry& registry) const {

        std::vector<std::string> errors;
        for (const auto& entry : entries) {
            if (entry.source != AssetCatalogSourceKind::LegacyRegistry) continue;
            if (registry.findBySemanticKey(entry.assetRef) == nullptr) {
                errors.push_back(entry.id + ": missing legacy asset reference " + entry.assetRef);
            }
        }
        return errors;
    }

    [[nodiscard]] std::optional<std::uint32_t> resolveLegacyServerId(
        const std::string& semanticId,
        const fantasy::assets::FantasyAssetRegistry& registry) const noexcept {

        const auto* entry = findEntry(semanticId);
        if (entry == nullptr || entry->source != AssetCatalogSourceKind::LegacyRegistry) return std::nullopt;
        const auto* legacy = registry.findBySemanticKey(entry->assetRef);
        if (legacy == nullptr) return std::nullopt;
        return legacy->serverId;
    }
};

class FantasyAssetCatalogStore {
public:
    static void save(const std::filesystem::path& path, const FantasyAssetCatalog& catalog) {
        catalog.validate();
        if (path.empty()) throw std::invalid_argument("Fantasy asset catalog path is required");

        nlohmann::json families = nlohmann::json::array();
        for (const auto& family : catalog.families) {
            families.push_back({
                {"id", family.id},
                {"name", family.name},
                {"tags", family.tags},
                {"brushRef", family.brushRef},
            });
        }

        nlohmann::json entries = nlohmann::json::array();
        for (const auto& entry : catalog.entries) {
            entries.push_back({
                {"id", entry.id},
                {"source", assetCatalogSourceKindId(entry.source)},
                {"assetRef", entry.assetRef},
                {"familyId", entry.familyId},
                {"role", entry.role},
                {"tags", entry.tags},
                {"weight", entry.weight},
            });
        }

        const auto parent = path.parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output.good()) throw std::runtime_error("unable to write Fantasy asset catalog: " + path.string());
        output << nlohmann::json{
            {"schemaVersion", FantasyAssetCatalog::SchemaVersion},
            {"profileId", catalog.profileId},
            {"families", std::move(families)},
            {"entries", std::move(entries)},
        }.dump(2) << '\n';
        if (!output.good()) throw std::runtime_error("failed while writing Fantasy asset catalog: " + path.string());
    }

    [[nodiscard]] static FantasyAssetCatalog load(const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        if (!input.good()) throw std::runtime_error("unable to read Fantasy asset catalog: " + path.string());

        nlohmann::json json;
        input >> json;
        if (!json.contains("schemaVersion") ||
            json.at("schemaVersion").get<std::uint32_t>() != FantasyAssetCatalog::SchemaVersion) {
            throw std::runtime_error("unsupported Fantasy asset catalog schema version");
        }

        FantasyAssetCatalog catalog;
        json.at("profileId").get_to(catalog.profileId);

        for (const auto& source : json.at("families")) {
            AssetFamilyDefinition family;
            source.at("id").get_to(family.id);
            source.at("name").get_to(family.name);
            source.at("tags").get_to(family.tags);
            if (source.contains("brushRef") && !source.at("brushRef").is_null()) {
                family.brushRef = source.at("brushRef").get<std::string>();
            }
            catalog.families.push_back(std::move(family));
        }

        for (const auto& source : json.at("entries")) {
            AssetCatalogEntry entry;
            source.at("id").get_to(entry.id);
            entry.source = parseAssetCatalogSourceKind(source.at("source").get<std::string>());
            source.at("assetRef").get_to(entry.assetRef);
            if (source.contains("familyId")) source.at("familyId").get_to(entry.familyId);
            if (source.contains("role")) source.at("role").get_to(entry.role);
            source.at("tags").get_to(entry.tags);
            if (source.contains("weight")) source.at("weight").get_to(entry.weight);
            catalog.entries.push_back(std::move(entry));
        }

        catalog.validate();
        return catalog;
    }
};

} // namespace fantasy::studio::foundation
