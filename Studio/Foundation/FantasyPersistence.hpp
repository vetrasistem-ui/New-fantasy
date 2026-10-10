#pragma once

#include "Foundation/FantasyBrushContracts.hpp"
#include "Foundation/FantasyFoundationV2.hpp"
#include "Foundation/FantasyModernAssets.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace fantasy::studio::foundation {

using FoundationJson = nlohmann::json;

inline std::string migrationModeId(AssetMigrationMode value) {
    switch (value) {
        case AssetMigrationMode::AddAsNew: return "add_as_new";
        case AssetMigrationMode::ReplaceObject: return "replace_object";
        case AssetMigrationMode::ReplaceVisualOnly: return "replace_visual_only";
    }
    throw std::invalid_argument("unknown asset migration mode");
}

inline AssetMigrationMode parseMigrationMode(const std::string& value) {
    if (value == "add_as_new") return AssetMigrationMode::AddAsNew;
    if (value == "replace_object") return AssetMigrationMode::ReplaceObject;
    if (value == "replace_visual_only") return AssetMigrationMode::ReplaceVisualOnly;
    throw std::invalid_argument("unknown asset migration mode: " + value);
}

inline std::string triggerKindId(TriggerKind value) {
    switch (value) {
        case TriggerKind::Login: return "login";
        case TriggerKind::Logout: return "logout";
        case TriggerKind::ChannelReceived: return "channel_received";
        case TriggerKind::EnterZone: return "enter_zone";
        case TriggerKind::LeaveZone: return "leave_zone";
        case TriggerKind::ItemUse: return "item_use";
        case TriggerKind::EntityDefeat: return "entity_defeat";
        case TriggerKind::Timer: return "timer";
        case TriggerKind::Interaction: return "interaction";
    }
    throw std::invalid_argument("unknown trigger kind");
}

inline TriggerKind parseTriggerKind(const std::string& value) {
    if (value == "login") return TriggerKind::Login;
    if (value == "logout") return TriggerKind::Logout;
    if (value == "channel_received") return TriggerKind::ChannelReceived;
    if (value == "enter_zone") return TriggerKind::EnterZone;
    if (value == "leave_zone") return TriggerKind::LeaveZone;
    if (value == "item_use") return TriggerKind::ItemUse;
    if (value == "entity_defeat") return TriggerKind::EntityDefeat;
    if (value == "timer") return TriggerKind::Timer;
    if (value == "interaction") return TriggerKind::Interaction;
    throw std::invalid_argument("unknown trigger kind: " + value);
}

inline std::string conditionKindId(ConditionKind value) {
    switch (value) {
        case ConditionKind::HasTag: return "has_tag";
        case ConditionKind::HasComponent: return "has_component";
        case ConditionKind::InZone: return "in_zone";
        case ConditionKind::InventoryContains: return "inventory_contains";
        case ConditionKind::CurrencyAtLeast: return "currency_at_least";
        case ConditionKind::ProgressAtLeast: return "progress_at_least";
        case ConditionKind::AttributeAtLeast: return "attribute_at_least";
        case ConditionKind::Ownership: return "ownership";
    }
    throw std::invalid_argument("unknown condition kind");
}

inline ConditionKind parseConditionKind(const std::string& value) {
    if (value == "has_tag") return ConditionKind::HasTag;
    if (value == "has_component") return ConditionKind::HasComponent;
    if (value == "in_zone") return ConditionKind::InZone;
    if (value == "inventory_contains") return ConditionKind::InventoryContains;
    if (value == "currency_at_least") return ConditionKind::CurrencyAtLeast;
    if (value == "progress_at_least") return ConditionKind::ProgressAtLeast;
    if (value == "attribute_at_least") return ConditionKind::AttributeAtLeast;
    if (value == "ownership") return ConditionKind::Ownership;
    throw std::invalid_argument("unknown condition kind: " + value);
}

inline std::string actionKindId(ActionKind value) {
    switch (value) {
        case ActionKind::SendSystemMessage: return "send_system_message";
        case ActionKind::SpawnEntity: return "spawn_entity";
        case ActionKind::RemoveEntity: return "remove_entity";
        case ActionKind::ModifyComponent: return "modify_component";
        case ActionKind::GrantItem: return "grant_item";
        case ActionKind::RemoveItem: return "remove_item";
        case ActionKind::UpdateProgress: return "update_progress";
        case ActionKind::ChangeAppearance: return "change_appearance";
        case ActionKind::PlayEffect: return "play_effect";
        case ActionKind::OpenClientModule: return "open_client_module";
        case ActionKind::PersistValue: return "persist_value";
    }
    throw std::invalid_argument("unknown action kind");
}

inline ActionKind parseActionKind(const std::string& value) {
    if (value == "send_system_message") return ActionKind::SendSystemMessage;
    if (value == "spawn_entity") return ActionKind::SpawnEntity;
    if (value == "remove_entity") return ActionKind::RemoveEntity;
    if (value == "modify_component") return ActionKind::ModifyComponent;
    if (value == "grant_item") return ActionKind::GrantItem;
    if (value == "remove_item") return ActionKind::RemoveItem;
    if (value == "update_progress") return ActionKind::UpdateProgress;
    if (value == "change_appearance") return ActionKind::ChangeAppearance;
    if (value == "play_effect") return ActionKind::PlayEffect;
    if (value == "open_client_module") return ActionKind::OpenClientModule;
    if (value == "persist_value") return ActionKind::PersistValue;
    throw std::invalid_argument("unknown action kind: " + value);
}

inline std::string modernAssetKindId(ModernAssetKind value) {
    switch (value) {
        case ModernAssetKind::Generic: return "generic";
        case ModernAssetKind::Ground: return "ground";
        case ModernAssetKind::Border: return "border";
        case ModernAssetKind::Wall: return "wall";
        case ModernAssetKind::Doodad: return "doodad";
        case ModernAssetKind::Item: return "item";
        case ModernAssetKind::Creature: return "creature";
        case ModernAssetKind::Outfit: return "outfit";
        case ModernAssetKind::Effect: return "effect";
        case ModernAssetKind::Missile: return "missile";
        case ModernAssetKind::Ui: return "ui";
    }
    throw std::invalid_argument("unknown modern asset kind");
}

inline ModernAssetKind parseModernAssetKind(const std::string& value) {
    if (value == "generic") return ModernAssetKind::Generic;
    if (value == "ground") return ModernAssetKind::Ground;
    if (value == "border") return ModernAssetKind::Border;
    if (value == "wall") return ModernAssetKind::Wall;
    if (value == "doodad") return ModernAssetKind::Doodad;
    if (value == "item") return ModernAssetKind::Item;
    if (value == "creature") return ModernAssetKind::Creature;
    if (value == "outfit") return ModernAssetKind::Outfit;
    if (value == "effect") return ModernAssetKind::Effect;
    if (value == "missile") return ModernAssetKind::Missile;
    if (value == "ui") return ModernAssetKind::Ui;
    throw std::invalid_argument("unknown modern asset kind: " + value);
}

inline std::string brushKindId(BrushKind value) {
    switch (value) {
        case BrushKind::Terrain: return "terrain";
        case BrushKind::AutoBorder: return "auto_border";
        case BrushKind::Wall: return "wall";
        case BrushKind::Doodad: return "doodad";
        case BrushKind::Carpet: return "carpet";
        case BrushKind::Table: return "table";
        case BrushKind::Erase: return "erase";
    }
    throw std::invalid_argument("unknown brush kind");
}

inline BrushKind parseBrushKind(const std::string& value) {
    if (value == "terrain") return BrushKind::Terrain;
    if (value == "auto_border") return BrushKind::AutoBorder;
    if (value == "wall") return BrushKind::Wall;
    if (value == "doodad") return BrushKind::Doodad;
    if (value == "carpet") return BrushKind::Carpet;
    if (value == "table") return BrushKind::Table;
    if (value == "erase") return BrushKind::Erase;
    throw std::invalid_argument("unknown brush kind: " + value);
}

inline void to_json(FoundationJson& j, const GridPosition& value) {
    j = {{"x", value.x}, {"y", value.y}, {"z", value.z}};
}
inline void from_json(const FoundationJson& j, GridPosition& value) {
    j.at("x").get_to(value.x); j.at("y").get_to(value.y); j.at("z").get_to(value.z);
}

inline void to_json(FoundationJson& j, const ZoneRect& value) {
    j = {{"x", value.x}, {"y", value.y}, {"z", value.z}, {"width", value.width}, {"height", value.height}};
}
inline void from_json(const FoundationJson& j, ZoneRect& value) {
    j.at("x").get_to(value.x); j.at("y").get_to(value.y); j.at("z").get_to(value.z);
    j.at("width").get_to(value.width); j.at("height").get_to(value.height);
}

inline void to_json(FoundationJson& j, const ComponentDefinition& value) {
    j = {{"type", value.type}, {"properties", value.properties}};
}
inline void from_json(const FoundationJson& j, ComponentDefinition& value) {
    j.at("type").get_to(value.type); j.at("properties").get_to(value.properties);
}

inline void to_json(FoundationJson& j, const AppearanceAttachment& value) {
    j = {{"slot", value.slot}, {"visualRef", value.visualRef}, {"layer", value.layer},
         {"offsetX", value.offsetX}, {"offsetY", value.offsetY}};
}
inline void from_json(const FoundationJson& j, AppearanceAttachment& value) {
    j.at("slot").get_to(value.slot); j.at("visualRef").get_to(value.visualRef); j.at("layer").get_to(value.layer);
    j.at("offsetX").get_to(value.offsetX); j.at("offsetY").get_to(value.offsetY);
}

inline void to_json(FoundationJson& j, const AppearanceEffect& value) {
    j = {{"id", value.id}, {"parameters", value.parameters}};
}
inline void from_json(const FoundationJson& j, AppearanceEffect& value) {
    j.at("id").get_to(value.id); j.at("parameters").get_to(value.parameters);
}

inline void to_json(FoundationJson& j, const EquipmentRequirements& value) {
    j = {{"minimumLevel", value.minimumLevel}, {"attributes", value.attributes}};
}
inline void from_json(const FoundationJson& j, EquipmentRequirements& value) {
    j.at("minimumLevel").get_to(value.minimumLevel); j.at("attributes").get_to(value.attributes);
}

inline void to_json(FoundationJson& j, const EquipmentBonuses& value) {
    j = {{"attributes", value.attributes}};
}
inline void from_json(const FoundationJson& j, EquipmentBonuses& value) {
    j.at("attributes").get_to(value.attributes);
}

inline void to_json(FoundationJson& j, const EquipmentDefinition& value) {
    j = {{"id", value.id}, {"slot", value.slot}, {"requirements", value.requirements}, {"bonuses", value.bonuses}};
}
inline void from_json(const FoundationJson& j, EquipmentDefinition& value) {
    j.at("id").get_to(value.id); j.at("slot").get_to(value.slot);
    j.at("requirements").get_to(value.requirements); j.at("bonuses").get_to(value.bonuses);
}

inline void to_json(FoundationJson& j, const ModernAssetFrame& value) {
    j = {{"visualRef", value.visualRef}, {"durationMs", value.durationMs}};
}
inline void from_json(const FoundationJson& j, ModernAssetFrame& value) {
    j.at("visualRef").get_to(value.visualRef); j.at("durationMs").get_to(value.durationMs);
}

inline void to_json(FoundationJson& j, const ModernAssetLayer& value) {
    j = {{"id", value.id}, {"order", value.order}, {"offsetX", value.offsetX}, {"offsetY", value.offsetY}, {"frames", value.frames}};
}
inline void from_json(const FoundationJson& j, ModernAssetLayer& value) {
    j.at("id").get_to(value.id); j.at("order").get_to(value.order); j.at("offsetX").get_to(value.offsetX);
    j.at("offsetY").get_to(value.offsetY); j.at("frames").get_to(value.frames);
}

inline void to_json(FoundationJson& j, const TerrainTransitionRule& value) {
    j = {{"neighborTag", value.neighborTag}, {"borderAssetRef", value.borderAssetRef}, {"priority", value.priority}};
}
inline void from_json(const FoundationJson& j, TerrainTransitionRule& value) {
    j.at("neighborTag").get_to(value.neighborTag); j.at("borderAssetRef").get_to(value.borderAssetRef); j.at("priority").get_to(value.priority);
}

inline void to_json(FoundationJson& j, const BrushVariant& value) {
    j = {{"assetRef", value.assetRef}, {"weight", value.weight}};
}
inline void from_json(const FoundationJson& j, BrushVariant& value) {
    j.at("assetRef").get_to(value.assetRef); j.at("weight").get_to(value.weight);
}

inline void to_json(FoundationJson& j, const BrushTransition& value) {
    j = {{"neighborTag", value.neighborTag}, {"assetRef", value.assetRef}, {"priority", value.priority}};
}
inline void from_json(const FoundationJson& j, BrushTransition& value) {
    j.at("neighborTag").get_to(value.neighborTag); j.at("assetRef").get_to(value.assetRef); j.at("priority").get_to(value.priority);
}

template <typename Kind>
inline FoundationJson systemNodeJson(const SystemNode<Kind>& value, const std::string& kind) {
    return FoundationJson{{"id", value.id}, {"kind", kind}, {"parameters", value.parameters}};
}

class FantasyPersistence {
public:
    static void saveZone(const std::filesystem::path& path, const ZoneDefinition& value) {
        value.validate();
        write(path, {{"schemaVersion", ZoneDefinition::SchemaVersion}, {"id", value.id}, {"name", value.name},
                     {"tags", value.tags}, {"rectangles", value.rectangles}, {"tiles", value.tiles}, {"metadata", value.metadata}});
    }

    [[nodiscard]] static ZoneDefinition loadZone(const std::filesystem::path& path) {
        const auto j = read(path); requireSchema(j, ZoneDefinition::SchemaVersion, "zone");
        ZoneDefinition value;
        j.at("id").get_to(value.id); j.at("name").get_to(value.name); j.at("tags").get_to(value.tags);
        j.at("rectangles").get_to(value.rectangles); j.at("tiles").get_to(value.tiles); j.at("metadata").get_to(value.metadata);
        value.validate(); return value;
    }

    static void saveAppearance(const std::filesystem::path& path, const AppearanceDefinition& value) {
        value.validate();
        write(path, {{"schemaVersion", AppearanceDefinition::SchemaVersion}, {"id", value.id}, {"baseVisual", value.baseVisual},
                     {"attachments", value.attachments}, {"effects", value.effects}, {"shaderRef", value.shaderRef}});
    }

    [[nodiscard]] static AppearanceDefinition loadAppearance(const std::filesystem::path& path) {
        const auto j = read(path); requireSchema(j, AppearanceDefinition::SchemaVersion, "appearance");
        AppearanceDefinition value;
        j.at("id").get_to(value.id); j.at("baseVisual").get_to(value.baseVisual);
        j.at("attachments").get_to(value.attachments); j.at("effects").get_to(value.effects);
        if (j.contains("shaderRef") && !j.at("shaderRef").is_null()) value.shaderRef = j.at("shaderRef").get<std::string>();
        value.validate(); return value;
    }

    static void saveEntity(const std::filesystem::path& path, const EntityArchetype& value) {
        value.validate();
        write(path, {{"schemaVersion", EntityArchetype::SchemaVersion}, {"id", value.id}, {"name", value.name},
                     {"tags", value.tags}, {"components", value.components}, {"appearanceRef", value.appearanceRef}});
    }

    [[nodiscard]] static EntityArchetype loadEntity(const std::filesystem::path& path) {
        const auto j = read(path); requireSchema(j, EntityArchetype::SchemaVersion, "entity");
        EntityArchetype value;
        j.at("id").get_to(value.id); j.at("name").get_to(value.name); j.at("tags").get_to(value.tags);
        j.at("components").get_to(value.components);
        if (j.contains("appearanceRef") && !j.at("appearanceRef").is_null()) value.appearanceRef = j.at("appearanceRef").get<std::string>();
        value.validate(); return value;
    }

    static void saveItem(const std::filesystem::path& path, const ItemDefinition& value) {
        value.validate();
        FoundationJson j = {{"schemaVersion", 1}, {"id", value.id}, {"name", value.name}, {"appearanceRef", value.appearanceRef},
                            {"tags", value.tags}, {"components", value.components}};
        if (value.equipment.has_value()) j["equipment"] = *value.equipment;
        else j["equipment"] = nullptr;
        write(path, j);
    }

    [[nodiscard]] static ItemDefinition loadItem(const std::filesystem::path& path) {
        const auto j = read(path); requireSchema(j, 1, "item");
        ItemDefinition value;
        j.at("id").get_to(value.id); j.at("name").get_to(value.name); j.at("tags").get_to(value.tags); j.at("components").get_to(value.components);
        if (j.contains("appearanceRef") && !j.at("appearanceRef").is_null()) value.appearanceRef = j.at("appearanceRef").get<std::string>();
        if (j.contains("equipment") && !j.at("equipment").is_null()) value.equipment = j.at("equipment").get<EquipmentDefinition>();
        value.validate(); return value;
    }

    static void saveCreature(const std::filesystem::path& path, const CreatureDefinition& value) {
        value.validate();
        write(path, {{"schemaVersion", 1}, {"id", value.id}, {"name", value.name}, {"appearanceRef", value.appearanceRef},
                     {"level", value.level}, {"movementSpeed", value.movementSpeed}, {"attributes", value.attributes},
                     {"tags", value.tags}, {"components", value.components}});
    }

    [[nodiscard]] static CreatureDefinition loadCreature(const std::filesystem::path& path) {
        const auto j = read(path); requireSchema(j, 1, "creature");
        CreatureDefinition value;
        j.at("id").get_to(value.id); j.at("name").get_to(value.name); j.at("appearanceRef").get_to(value.appearanceRef);
        j.at("level").get_to(value.level); j.at("movementSpeed").get_to(value.movementSpeed); j.at("attributes").get_to(value.attributes);
        j.at("tags").get_to(value.tags); j.at("components").get_to(value.components);
        value.validate(); return value;
    }

    static void saveClass(const std::filesystem::path& path, const ClassDefinition& value) {
        value.validate();
        write(path, {{"schemaVersion", 1}, {"id", value.id}, {"name", value.name},
                     {"passiveSystems", value.passiveSystems}, {"abilitySystems", value.abilitySystems}});
    }

    [[nodiscard]] static ClassDefinition loadClass(const std::filesystem::path& path) {
        const auto j = read(path); requireSchema(j, 1, "class");
        ClassDefinition value;
        j.at("id").get_to(value.id); j.at("name").get_to(value.name);
        j.at("passiveSystems").get_to(value.passiveSystems); j.at("abilitySystems").get_to(value.abilitySystems);
        value.validate(); return value;
    }

    static void saveSystem(const std::filesystem::path& path, const SystemDefinition& value) {
        value.validate();
        FoundationJson triggers = FoundationJson::array();
        FoundationJson conditions = FoundationJson::array();
        FoundationJson actions = FoundationJson::array();
        for (const auto& node : value.triggers) triggers.push_back(systemNodeJson(node, triggerKindId(node.kind)));
        for (const auto& node : value.conditions) conditions.push_back(systemNodeJson(node, conditionKindId(node.kind)));
        for (const auto& node : value.actions) actions.push_back(systemNodeJson(node, actionKindId(node.kind)));
        write(path, {{"schemaVersion", SystemDefinition::SchemaVersion}, {"id", value.id}, {"version", value.version},
                     {"dependencies", value.dependencies}, {"requiredChannels", value.requiredChannels},
                     {"triggers", triggers}, {"conditions", conditions}, {"actions", actions}});
    }

    [[nodiscard]] static SystemDefinition loadSystem(const std::filesystem::path& path) {
        const auto j = read(path); requireSchema(j, SystemDefinition::SchemaVersion, "system");
        SystemDefinition value;
        j.at("id").get_to(value.id); j.at("version").get_to(value.version);
        j.at("dependencies").get_to(value.dependencies); j.at("requiredChannels").get_to(value.requiredChannels);
        for (const auto& node : j.at("triggers")) value.triggers.push_back({node.at("id").get<std::string>(), parseTriggerKind(node.at("kind").get<std::string>()), node.at("parameters").get<std::map<std::string, std::string>>()});
        for (const auto& node : j.at("conditions")) value.conditions.push_back({node.at("id").get<std::string>(), parseConditionKind(node.at("kind").get<std::string>()), node.at("parameters").get<std::map<std::string, std::string>>()});
        for (const auto& node : j.at("actions")) value.actions.push_back({node.at("id").get<std::string>(), parseActionKind(node.at("kind").get<std::string>()), node.at("parameters").get<std::map<std::string, std::string>>()});
        value.validate(); return value;
    }

    static void saveAssetProfile(const std::filesystem::path& path, const AssetProfile& value) {
        value.validate();
        FoundationJson sources = FoundationJson::array();
        for (const auto& source : value.sources) sources.push_back({{"role", source.role}, {"path", source.path.generic_string()}, {"sha256", source.sha256}});
        write(path, {{"schemaVersion", AssetProfile::SchemaVersion}, {"id", value.id}, {"version", value.version}, {"sources", sources}});
    }

    [[nodiscard]] static AssetProfile loadAssetProfile(const std::filesystem::path& path) {
        const auto j = read(path); requireSchema(j, AssetProfile::SchemaVersion, "asset profile");
        AssetProfile value;
        j.at("id").get_to(value.id); j.at("version").get_to(value.version);
        for (const auto& source : j.at("sources")) value.sources.push_back({source.at("role").get<std::string>(), source.at("path").get<std::string>(), source.at("sha256").get<std::string>()});
        value.validate(); return value;
    }

    static void saveMigration(const std::filesystem::path& path, const AssetMigrationPlan& value) {
        value.validate();
        FoundationJson entries = FoundationJson::array();
        for (const auto& entry : value.entries) entries.push_back({{"legacyId", entry.legacyId}, {"targetId", entry.targetId}, {"mode", migrationModeId(entry.mode)}});
        write(path, {{"schemaVersion", AssetMigrationPlan::SchemaVersion}, {"sourceProfile", value.sourceProfile}, {"targetProfile", value.targetProfile}, {"entries", entries}});
    }

    [[nodiscard]] static AssetMigrationPlan loadMigration(const std::filesystem::path& path) {
        const auto j = read(path); requireSchema(j, AssetMigrationPlan::SchemaVersion, "asset migration");
        AssetMigrationPlan value;
        j.at("sourceProfile").get_to(value.sourceProfile); j.at("targetProfile").get_to(value.targetProfile);
        for (const auto& entry : j.at("entries")) value.entries.push_back({entry.at("legacyId").get<std::uint32_t>(), entry.at("targetId").get<std::uint32_t>(), parseMigrationMode(entry.at("mode").get<std::string>())});
        value.validate(); return value;
    }

    static void saveModernAsset(const std::filesystem::path& path, const ModernAssetDefinition& value) {
        value.validate();
        write(path, {{"schemaVersion", ModernAssetDefinition::SchemaVersion}, {"id", value.id}, {"kind", modernAssetKindId(value.kind)},
                     {"width", value.width}, {"height", value.height}, {"tags", value.tags}, {"layers", value.layers}, {"transitions", value.transitions}});
    }

    [[nodiscard]] static ModernAssetDefinition loadModernAsset(const std::filesystem::path& path) {
        const auto j = read(path); requireSchema(j, ModernAssetDefinition::SchemaVersion, "modern asset");
        ModernAssetDefinition value;
        j.at("id").get_to(value.id); value.kind = parseModernAssetKind(j.at("kind").get<std::string>());
        j.at("width").get_to(value.width); j.at("height").get_to(value.height); j.at("tags").get_to(value.tags);
        j.at("layers").get_to(value.layers); j.at("transitions").get_to(value.transitions);
        value.validate(); return value;
    }

    static void saveBrush(const std::filesystem::path& path, const BrushDefinition& value) {
        value.validate();
        write(path, {{"schemaVersion", BrushDefinition::SchemaVersion}, {"id", value.id}, {"kind", brushKindId(value.kind)},
                     {"requiredTags", value.requiredTags}, {"variants", value.variants}, {"transitions", value.transitions}});
    }

    [[nodiscard]] static BrushDefinition loadBrush(const std::filesystem::path& path) {
        const auto j = read(path); requireSchema(j, BrushDefinition::SchemaVersion, "brush");
        BrushDefinition value;
        j.at("id").get_to(value.id); value.kind = parseBrushKind(j.at("kind").get<std::string>());
        j.at("requiredTags").get_to(value.requiredTags); j.at("variants").get_to(value.variants); j.at("transitions").get_to(value.transitions);
        value.validate(); return value;
    }

private:
    static void write(const std::filesystem::path& path, const FoundationJson& j) {
        if (path.empty()) throw std::invalid_argument("Fantasy persistence path is required");
        const auto parent = path.parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("unable to write Fantasy JSON: " + path.string());
        output << j.dump(2) << '\n';
        if (!output) throw std::runtime_error("failed while writing Fantasy JSON: " + path.string());
    }

    [[nodiscard]] static FoundationJson read(const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        if (!input) throw std::runtime_error("unable to read Fantasy JSON: " + path.string());
        FoundationJson j;
        input >> j;
        return j;
    }

    static void requireSchema(const FoundationJson& j, std::uint32_t expected, const char* label) {
        if (!j.contains("schemaVersion") || j.at("schemaVersion").get<std::uint32_t>() != expected) {
            throw std::runtime_error(std::string("unsupported Fantasy ") + label + " schemaVersion");
        }
    }
};

} // namespace fantasy::studio::foundation
