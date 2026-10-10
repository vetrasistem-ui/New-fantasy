#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace fantasy::studio::foundation {

[[nodiscard]] inline bool validIdentifier(const std::string& value) noexcept {
    if (value.empty()) return false;
    bool hasAlphaNumeric = false;
    for (const unsigned char ch : value) {
        const bool alphaNumeric =
            (ch >= '0' && ch <= '9') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z');
        if (alphaNumeric) {
            hasAlphaNumeric = true;
            continue;
        }
        if (ch == '.' || ch == '_' || ch == '-' || ch == ':') continue;
        return false;
    }
    return hasAlphaNumeric;
}

inline void requireIdentifier(const std::string& value, const char* label) {
    if (!validIdentifier(value)) {
        throw std::invalid_argument(std::string("invalid Fantasy ") + label + ": " + value);
    }
}

[[nodiscard]] inline bool safeRelativePath(const std::filesystem::path& value) noexcept {
    if (value.empty() || value.is_absolute() || value.has_root_name() || value.has_root_directory()) return false;
    for (const auto& part : value) {
        if (part == "..") return false;
    }
    return true;
}

struct GridPosition {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int16_t z = 7;

    friend bool operator==(const GridPosition&, const GridPosition&) = default;
};

struct ZoneRect {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int16_t z = 7;
    std::uint32_t width = 1;
    std::uint32_t height = 1;

    [[nodiscard]] bool contains(const GridPosition& position) const noexcept {
        if (position.z != z || width == 0 || height == 0) return false;
        const auto maxX = static_cast<std::int64_t>(x) + static_cast<std::int64_t>(width);
        const auto maxY = static_cast<std::int64_t>(y) + static_cast<std::int64_t>(height);
        return position.x >= x && position.y >= y &&
               static_cast<std::int64_t>(position.x) < maxX &&
               static_cast<std::int64_t>(position.y) < maxY;
    }
};

struct ZoneDefinition {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::string id;
    std::string name;
    std::vector<std::string> tags;
    std::vector<ZoneRect> rectangles;
    std::vector<GridPosition> tiles;
    std::map<std::string, std::string> metadata;

    void validate() const {
        requireIdentifier(id, "zone id");
        if (name.empty()) throw std::invalid_argument("Fantasy zone name is required");
        if (rectangles.empty() && tiles.empty()) {
            throw std::invalid_argument("Fantasy zone requires at least one rectangle or tile");
        }
        std::set<std::string> seenTags;
        for (const auto& tag : tags) {
            requireIdentifier(tag, "zone tag");
            if (!seenTags.insert(tag).second) throw std::invalid_argument("duplicate Fantasy zone tag: " + tag);
        }
        for (const auto& rectangle : rectangles) {
            if (rectangle.width == 0 || rectangle.height == 0) {
                throw std::invalid_argument("Fantasy zone rectangle width/height must be greater than zero");
            }
        }
        for (const auto& [key, value] : metadata) {
            (void)value;
            requireIdentifier(key, "zone metadata key");
        }
    }

    [[nodiscard]] bool contains(const GridPosition& position) const noexcept {
        if (std::find(tiles.begin(), tiles.end(), position) != tiles.end()) return true;
        return std::any_of(rectangles.begin(), rectangles.end(), [&](const auto& rectangle) {
            return rectangle.contains(position);
        });
    }
};

struct AppearanceAttachment {
    std::string slot;
    std::string visualRef;
    std::int32_t layer = 0;
    std::int32_t offsetX = 0;
    std::int32_t offsetY = 0;
};

struct AppearanceEffect {
    std::string id;
    std::map<std::string, std::string> parameters;
};

struct AppearanceDefinition {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::string id;
    std::string baseVisual;
    std::vector<AppearanceAttachment> attachments;
    std::vector<AppearanceEffect> effects;
    std::optional<std::string> shaderRef;

    void validate() const {
        requireIdentifier(id, "appearance id");
        requireIdentifier(baseVisual, "appearance base visual");
        std::set<std::pair<std::string, std::int32_t>> occupiedLayers;
        for (const auto& attachment : attachments) {
            requireIdentifier(attachment.slot, "appearance attachment slot");
            requireIdentifier(attachment.visualRef, "appearance attachment visual");
            if (!occupiedLayers.emplace(attachment.slot, attachment.layer).second) {
                throw std::invalid_argument("duplicate Fantasy appearance attachment slot/layer");
            }
        }
        std::set<std::string> effectIds;
        for (const auto& effect : effects) {
            requireIdentifier(effect.id, "appearance effect id");
            if (!effectIds.insert(effect.id).second) {
                throw std::invalid_argument("duplicate Fantasy appearance effect: " + effect.id);
            }
        }
        if (shaderRef.has_value()) requireIdentifier(*shaderRef, "appearance shader reference");
    }
};

struct ComponentDefinition {
    std::string type;
    std::map<std::string, std::string> properties;
};

struct EntityArchetype {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::string id;
    std::string name;
    std::vector<std::string> tags;
    std::vector<ComponentDefinition> components;
    std::optional<std::string> appearanceRef;

    void validate() const {
        requireIdentifier(id, "entity id");
        if (name.empty()) throw std::invalid_argument("Fantasy entity name is required");
        std::set<std::string> tagIds;
        for (const auto& tag : tags) {
            requireIdentifier(tag, "entity tag");
            if (!tagIds.insert(tag).second) throw std::invalid_argument("duplicate Fantasy entity tag: " + tag);
        }
        std::set<std::string> componentTypes;
        for (const auto& component : components) {
            requireIdentifier(component.type, "entity component type");
            if (!componentTypes.insert(component.type).second) {
                throw std::invalid_argument("duplicate Fantasy entity component: " + component.type);
            }
            for (const auto& [key, value] : component.properties) {
                (void)value;
                requireIdentifier(key, "entity component property key");
            }
        }
        if (appearanceRef.has_value()) requireIdentifier(*appearanceRef, "entity appearance reference");
    }
};

using AttributeMap = std::map<std::string, std::int32_t>;

inline void validateAttributeMap(const AttributeMap& attributes, const char* label) {
    for (const auto& [id, value] : attributes) {
        requireIdentifier(id, label);
        if (value < 0) throw std::invalid_argument(std::string(label) + " cannot be negative: " + id);
    }
}

struct EquipmentRequirements {
    std::uint32_t minimumLevel = 1;
    AttributeMap attributes;

    void validate() const {
        if (minimumLevel == 0) throw std::invalid_argument("equipment minimum level must be at least 1");
        validateAttributeMap(attributes, "equipment requirement attribute");
    }
};

struct EquipmentBonuses {
    AttributeMap attributes;

    void validate() const {
        validateAttributeMap(attributes, "equipment bonus attribute");
    }
};

struct EquipmentDefinition {
    std::string id;
    std::string slot;
    EquipmentRequirements requirements;
    EquipmentBonuses bonuses;

    void validate() const {
        requireIdentifier(id, "equipment id");
        requireIdentifier(slot, "equipment slot");
        requirements.validate();
        bonuses.validate();
    }
};

struct CharacterState {
    std::uint32_t level = 1;
    AttributeMap naturalAttributes;
    AttributeMap distributedAttributes;
    AttributeMap equippedBonuses;

    [[nodiscard]] std::int32_t effectiveAttribute(const std::string& id) const noexcept {
        const auto read = [&](const AttributeMap& values) {
            const auto it = values.find(id);
            return it == values.end() ? 0 : it->second;
        };
        return read(naturalAttributes) + read(distributedAttributes) + read(equippedBonuses);
    }
};

struct EquipmentEligibility {
    bool allowed = false;
    bool levelSatisfied = false;
    std::vector<std::string> missingAttributes;
};

[[nodiscard]] inline EquipmentEligibility evaluateEquipmentEligibility(
    const CharacterState& character,
    const EquipmentDefinition& equipment) {

    equipment.validate();
    EquipmentEligibility result;
    result.levelSatisfied = character.level >= equipment.requirements.minimumLevel;
    for (const auto& [attribute, requiredValue] : equipment.requirements.attributes) {
        if (character.effectiveAttribute(attribute) < requiredValue) {
            result.missingAttributes.push_back(attribute);
        }
    }
    result.allowed = result.levelSatisfied && result.missingAttributes.empty();
    return result;
}

struct ClassDefinition {
    std::string id;
    std::string name;
    std::vector<std::string> passiveSystems;
    std::vector<std::string> abilitySystems;

    void validate() const {
        requireIdentifier(id, "class id");
        if (name.empty()) throw std::invalid_argument("Fantasy class name is required");
        for (const auto& system : passiveSystems) requireIdentifier(system, "class passive system");
        for (const auto& system : abilitySystems) requireIdentifier(system, "class ability system");
    }
};

struct ItemDefinition {
    std::string id;
    std::string name;
    std::optional<std::string> appearanceRef;
    std::vector<std::string> tags;
    std::vector<ComponentDefinition> components;
    std::optional<EquipmentDefinition> equipment;

    void validate() const {
        requireIdentifier(id, "item id");
        if (name.empty()) throw std::invalid_argument("Fantasy item name is required");
        if (appearanceRef.has_value()) requireIdentifier(*appearanceRef, "item appearance reference");
        for (const auto& tag : tags) requireIdentifier(tag, "item tag");
        std::set<std::string> componentTypes;
        for (const auto& component : components) {
            requireIdentifier(component.type, "item component type");
            if (!componentTypes.insert(component.type).second) {
                throw std::invalid_argument("duplicate Fantasy item component: " + component.type);
            }
        }
        if (equipment.has_value()) equipment->validate();
    }
};

struct CreatureDefinition {
    std::string id;
    std::string name;
    std::string appearanceRef;
    std::uint32_t level = 1;
    std::uint32_t movementSpeed = 100;
    AttributeMap attributes;
    std::vector<std::string> tags;
    std::vector<ComponentDefinition> components;

    void validate() const {
        requireIdentifier(id, "creature id");
        if (name.empty()) throw std::invalid_argument("Fantasy creature name is required");
        requireIdentifier(appearanceRef, "creature appearance reference");
        if (level == 0) throw std::invalid_argument("Fantasy creature level must be at least 1");
        if (movementSpeed == 0) throw std::invalid_argument("Fantasy creature movement speed must be greater than zero");
        validateAttributeMap(attributes, "creature attribute");
        for (const auto& tag : tags) requireIdentifier(tag, "creature tag");
        std::set<std::string> componentTypes;
        for (const auto& component : components) {
            requireIdentifier(component.type, "creature component type");
            if (!componentTypes.insert(component.type).second) {
                throw std::invalid_argument("duplicate Fantasy creature component: " + component.type);
            }
        }
    }
};

enum class TriggerKind {
    Login,
    Logout,
    ChannelReceived,
    EnterZone,
    LeaveZone,
    ItemUse,
    EntityDefeat,
    Timer,
    Interaction,
};

enum class ConditionKind {
    HasTag,
    HasComponent,
    InZone,
    InventoryContains,
    CurrencyAtLeast,
    ProgressAtLeast,
    AttributeAtLeast,
    Ownership,
};

enum class ActionKind {
    SendSystemMessage,
    SpawnEntity,
    RemoveEntity,
    ModifyComponent,
    GrantItem,
    RemoveItem,
    UpdateProgress,
    ChangeAppearance,
    PlayEffect,
    OpenClientModule,
    PersistValue,
};

template <typename Kind>
struct SystemNode {
    std::string id;
    Kind kind{};
    std::map<std::string, std::string> parameters;
};

using TriggerNode = SystemNode<TriggerKind>;
using ConditionNode = SystemNode<ConditionKind>;
using ActionNode = SystemNode<ActionKind>;

struct SystemDefinition {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::string id;
    std::uint32_t version = 1;
    std::vector<std::string> dependencies;
    std::vector<std::string> requiredChannels;
    std::vector<TriggerNode> triggers;
    std::vector<ConditionNode> conditions;
    std::vector<ActionNode> actions;

    void validate() const {
        requireIdentifier(id, "system id");
        if (version == 0) throw std::invalid_argument("Fantasy system version must be at least 1");
        if (triggers.empty()) throw std::invalid_argument("Fantasy system requires at least one trigger");
        if (actions.empty()) throw std::invalid_argument("Fantasy system requires at least one action");

        std::set<std::string> dependencyIds;
        for (const auto& dependency : dependencies) {
            requireIdentifier(dependency, "system dependency");
            if (!dependencyIds.insert(dependency).second) {
                throw std::invalid_argument("duplicate Fantasy system dependency: " + dependency);
            }
        }
        std::set<std::string> channelIds;
        for (const auto& channel : requiredChannels) {
            requireIdentifier(channel, "system channel");
            if (!channelIds.insert(channel).second) {
                throw std::invalid_argument("duplicate Fantasy system channel: " + channel);
            }
        }

        std::set<std::string> nodeIds;
        const auto validateNode = [&](const auto& node) {
            requireIdentifier(node.id, "system node id");
            if (!nodeIds.insert(node.id).second) {
                throw std::invalid_argument("duplicate Fantasy system node id: " + node.id);
            }
            for (const auto& [key, value] : node.parameters) {
                (void)value;
                requireIdentifier(key, "system node parameter key");
            }
        };
        for (const auto& node : triggers) validateNode(node);
        for (const auto& node : conditions) validateNode(node);
        for (const auto& node : actions) validateNode(node);
    }
};

struct AssetSourceDescriptor {
    std::string role;
    std::filesystem::path path;
    std::string sha256;

    void validate() const {
        requireIdentifier(role, "asset source role");
        if (!safeRelativePath(path)) throw std::invalid_argument("asset source path must be project-relative and safe");
        if (!sha256.empty() && sha256.size() != 64U) {
            throw std::invalid_argument("asset source sha256 must contain 64 hexadecimal characters when supplied");
        }
    }
};

struct AssetProfile {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::string id;
    std::uint32_t version = 1;
    std::vector<AssetSourceDescriptor> sources;

    void validate() const {
        requireIdentifier(id, "asset profile id");
        if (version == 0) throw std::invalid_argument("asset profile version must be at least 1");
        if (sources.empty()) throw std::invalid_argument("asset profile requires at least one source");
        std::set<std::string> roles;
        for (const auto& source : sources) {
            source.validate();
            if (!roles.insert(source.role).second) {
                throw std::invalid_argument("duplicate asset source role: " + source.role);
            }
        }
    }
};

enum class AssetMigrationMode {
    AddAsNew,
    ReplaceObject,
    ReplaceVisualOnly,
};

struct AssetMigrationEntry {
    std::uint32_t legacyId = 0;
    std::uint32_t targetId = 0;
    AssetMigrationMode mode = AssetMigrationMode::AddAsNew;
};

struct AssetMigrationPlan {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::string sourceProfile;
    std::string targetProfile;
    std::vector<AssetMigrationEntry> entries;

    void validate() const {
        requireIdentifier(sourceProfile, "asset migration source profile");
        requireIdentifier(targetProfile, "asset migration target profile");
        if (sourceProfile == targetProfile) {
            throw std::invalid_argument("asset migration source and target profiles must differ");
        }
        std::set<std::uint32_t> legacyIds;
        std::set<std::uint32_t> explicitTargets;
        for (const auto& entry : entries) {
            if (entry.legacyId == 0) throw std::invalid_argument("asset migration legacy id must be non-zero");
            if (!legacyIds.insert(entry.legacyId).second) {
                throw std::invalid_argument("duplicate asset migration legacy id");
            }
            if (entry.mode != AssetMigrationMode::AddAsNew && entry.targetId == 0) {
                throw std::invalid_argument("replace asset migration requires a target id");
            }
            if (entry.targetId != 0 && !explicitTargets.insert(entry.targetId).second) {
                throw std::invalid_argument("multiple asset migrations cannot target the same explicit id");
            }
        }
    }
};

enum class BuildTarget {
    Validate,
    Runtime,
    Client,
    Vps,
};

struct BuildPlan {
    bool validateProject = true;
    bool buildRuntime = true;
    bool buildClient = false;
    bool buildVps = false;

    void validate() const {
        if (!validateProject) throw std::invalid_argument("Fantasy build plan must validate the project first");
        if (buildVps && !buildRuntime) {
            throw std::invalid_argument("Fantasy VPS build requires runtime build");
        }
    }

    [[nodiscard]] std::vector<BuildTarget> orderedTargets() const {
        validate();
        std::vector<BuildTarget> result{BuildTarget::Validate};
        if (buildRuntime) result.push_back(BuildTarget::Runtime);
        if (buildClient) result.push_back(BuildTarget::Client);
        if (buildVps) result.push_back(BuildTarget::Vps);
        return result;
    }
};

enum class HealthSeverity {
    Info,
    Warning,
    Error,
};

struct ProjectHealthIssue {
    HealthSeverity severity = HealthSeverity::Info;
    std::string code;
    std::string message;
};

struct ProjectHealthInput {
    bool hasMainMap = false;
    bool hasAssetProfile = false;
    bool hasRuntimeProfile = false;
    bool hasRuntimeTemplate = false;
    bool extendedProfile = false;
    bool hasClientPackage = false;
};

struct ProjectHealthReport {
    std::vector<ProjectHealthIssue> issues;

    [[nodiscard]] bool ready() const noexcept {
        return std::none_of(issues.begin(), issues.end(), [](const auto& issue) {
            return issue.severity == HealthSeverity::Error;
        });
    }
};

[[nodiscard]] inline ProjectHealthReport evaluateProjectHealth(const ProjectHealthInput& input) {
    ProjectHealthReport report;
    const auto addError = [&](std::string code, std::string message) {
        report.issues.push_back({HealthSeverity::Error, std::move(code), std::move(message)});
    };
    if (!input.hasMainMap) addError("map.missing", "Main Fantasy map is missing");
    if (!input.hasAssetProfile) addError("assets.profile.missing", "Asset profile is missing");
    if (!input.hasRuntimeProfile) addError("runtime.profile.missing", "Runtime profile is missing");
    if (!input.hasRuntimeTemplate) addError("runtime.template.missing", "Runtime template is not configured");
    if (input.extendedProfile && !input.hasClientPackage) {
        addError("client.package.missing", "OTC extended profile requires a selected client package");
    }
    return report;
}

} // namespace fantasy::studio::foundation
