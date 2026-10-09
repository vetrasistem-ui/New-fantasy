#include "Foundation/FantasyPersistence.hpp"
#include "Foundation/FantasyProjectLayoutV2.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using namespace fantasy::studio::foundation;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("FANTASY_PERSISTENCE TEST FAIL: " + message);
}

void writeText(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    require(output.good(), "unable to write fixture: " + path.string());
    output << text;
    require(output.good(), "failed while writing fixture: " + path.string());
}

void testZoneRoundtrip(const fs::path& root) {
    ZoneDefinition value;
    value.id = "zone.town.center";
    value.name = "Town Center";
    value.tags = {"safe", "social.hub"};
    value.rectangles.push_back({100, 100, 7, 8, 6});
    value.tiles.push_back({150, 150, 7});
    value.metadata = {{"music.id", "theme.town"}};

    const auto path = FantasyProjectLayoutV2::zonePath(root, value.id);
    FantasyPersistence::saveZone(path, value);
    const auto loaded = FantasyPersistence::loadZone(path);
    require(loaded.id == value.id && loaded.name == value.name, "zone identity changed after roundtrip");
    require(loaded.contains({103, 103, 7}) && loaded.contains({150, 150, 7}), "zone geometry changed after roundtrip");
}

void testAppearanceEntityItemCreatureClass(const fs::path& root) {
    AppearanceDefinition appearance;
    appearance.id = "appearance.hero.ranger";
    appearance.baseVisual = "visual.hero.ranger";
    appearance.attachments = {{"back", "visual.cape.green", 10, 1, 0}};
    appearance.effects = {{"effect.aura.faint", {{"intensity", "0.2"}}}};
    appearance.shaderRef = "shader.outfit.default";
    const auto appearancePath = FantasyProjectLayoutV2::appearancePath(root, appearance.id);
    FantasyPersistence::saveAppearance(appearancePath, appearance);
    require(FantasyPersistence::loadAppearance(appearancePath).shaderRef == appearance.shaderRef,
        "appearance shader changed after roundtrip");

    EntityArchetype entity;
    entity.id = "entity.npc.guide";
    entity.name = "Guide";
    entity.tags = {"entity.interactable"};
    entity.appearanceRef = appearance.id;
    entity.components = {{"dialogue.basic", {{"dialogue.id", "guide.start"}}}};
    const auto entityPath = FantasyProjectLayoutV2::entityPath(root, entity.id);
    FantasyPersistence::saveEntity(entityPath, entity);
    require(FantasyPersistence::loadEntity(entityPath).appearanceRef == entity.appearanceRef,
        "entity appearance reference changed after roundtrip");

    ItemDefinition item;
    item.id = "item.bow.ranger";
    item.name = "Ranger Bow";
    item.appearanceRef = "appearance.item.bow.ranger";
    EquipmentDefinition equipment;
    equipment.id = "equipment.bow.ranger";
    equipment.slot = "weapon";
    equipment.requirements.minimumLevel = 50;
    equipment.requirements.attributes = {{"strength", 30}, {"agility", 40}};
    equipment.bonuses.attributes = {{"agility", 5}};
    item.equipment = equipment;
    const auto itemPath = FantasyProjectLayoutV2::itemPath(root, item.id);
    FantasyPersistence::saveItem(itemPath, item);
    const auto loadedItem = FantasyPersistence::loadItem(itemPath);
    require(loadedItem.equipment.has_value(), "item equipment was lost after roundtrip");
    require(loadedItem.equipment->requirements.minimumLevel == 50, "item level requirement changed after roundtrip");

    CreatureDefinition creature;
    creature.id = "creature.wolf.basic";
    creature.name = "Wolf";
    creature.appearanceRef = "appearance.creature.wolf";
    creature.level = 8;
    creature.movementSpeed = 120;
    creature.attributes = {{"strength", 12}, {"agility", 18}};
    creature.tags = {"hostile", "beast"};
    creature.components = {{"ai.melee", {{"profile.id", "ai.wolf.basic"}}}};
    const auto creaturePath = FantasyProjectLayoutV2::creaturePath(root, creature.id);
    FantasyPersistence::saveCreature(creaturePath, creature);
    require(FantasyPersistence::loadCreature(creaturePath).attributes.at("agility") == 18,
        "creature attributes changed after roundtrip");

    ClassDefinition classDefinition;
    classDefinition.id = "class.ranger";
    classDefinition.name = "Ranger";
    classDefinition.passiveSystems = {"system.class.ranger.passive"};
    classDefinition.abilitySystems = {"system.ability.aimed_shot"};
    const auto classPath = FantasyProjectLayoutV2::classPath(root, classDefinition.id);
    FantasyPersistence::saveClass(classPath, classDefinition);
    require(FantasyPersistence::loadClass(classPath).abilitySystems == classDefinition.abilitySystems,
        "class abilities changed after roundtrip");
}

void testSystemRoundtrip(const fs::path& root) {
    SystemDefinition system;
    system.id = "system.zone.welcome";
    system.version = 2;
    system.dependencies = {"system.persistence.player"};
    system.requiredChannels = {"ui.notification"};
    system.triggers = {{"trigger.enter", TriggerKind::EnterZone, {{"zone.id", "zone.town.center"}}}};
    system.conditions = {{"condition.player", ConditionKind::HasTag, {{"tag.id", "entity.player"}}}};
    system.actions = {{"action.notify", ActionKind::SendSystemMessage,
        {{"channel.id", "ui.notification"}, {"message.id", "town.welcome"}}}};

    const auto path = FantasyProjectLayoutV2::systemPath(root, system.id);
    FantasyPersistence::saveSystem(path, system);
    const auto loaded = FantasyPersistence::loadSystem(path);
    require(loaded.version == 2 && loaded.triggers.front().kind == TriggerKind::EnterZone,
        "system trigger/version changed after roundtrip");
    require(loaded.actions.front().kind == ActionKind::SendSystemMessage,
        "system action changed after roundtrip");
}

void testAssetPersistence(const fs::path& root) {
    AssetProfile profile;
    profile.id = "assets.1098.official";
    profile.version = 3;
    profile.sources = {
        {"dat", fs::path{"Assets/1098/Tibia.dat"}, std::string(64, 'a')},
        {"spr", fs::path{"Assets/1098/Tibia.spr"}, std::string(64, 'b')},
        {"otb", fs::path{"Assets/1098/items.otb"}, std::string(64, 'c')},
    };
    const auto profilePath = FantasyProjectLayoutV2::assetProfilePath(root, profile.id);
    FantasyPersistence::saveAssetProfile(profilePath, profile);
    require(FantasyPersistence::loadAssetProfile(profilePath).sources.size() == 3U,
        "asset profile sources changed after roundtrip");

    AssetMigrationPlan migration;
    migration.sourceProfile = "assets.854.legacy";
    migration.targetProfile = "assets.1524.modern";
    migration.entries = {
        {100, 0, AssetMigrationMode::AddAsNew},
        {101, 5001, AssetMigrationMode::ReplaceObject},
        {102, 5002, AssetMigrationMode::ReplaceVisualOnly},
    };
    const auto migrationPath = FantasyProjectLayoutV2::migrationPath(root, "migration.854.to.1524");
    FantasyPersistence::saveMigration(migrationPath, migration);
    const auto loadedMigration = FantasyPersistence::loadMigration(migrationPath);
    require(loadedMigration.entries[2].mode == AssetMigrationMode::ReplaceVisualOnly,
        "asset migration mode changed after roundtrip");
}

void testModernAssetAndBrush(const fs::path& root) {
    ModernAssetDefinition asset;
    asset.id = "asset.ground.grass";
    asset.kind = ModernAssetKind::Ground;
    asset.width = 1;
    asset.height = 1;
    asset.tags = {"terrain.grass", "walkable"};
    asset.layers = {{"base", 0, 0, 0, {{"visual.grass.01", 100}, {"visual.grass.02", 100}}}};
    asset.transitions = {{"terrain.sand", "asset.border.grass_sand", 100}};
    const fs::path assetPath = root / "Assets" / "Modern" / "asset.ground.grass.asset.json";
    FantasyPersistence::saveModernAsset(assetPath, asset);
    const auto loadedAsset = FantasyPersistence::loadModernAsset(assetPath);
    require(loadedAsset.kind == ModernAssetKind::Ground && loadedAsset.layers.front().frames.size() == 2U,
        "modern asset changed after roundtrip");

    BrushDefinition brush;
    brush.id = "brush.terrain.grass";
    brush.kind = BrushKind::Terrain;
    brush.requiredTags = {"terrain.grass"};
    brush.variants = {{"asset.ground.grass", 3}, {"asset.ground.grass.flowers", 1}};
    brush.transitions = {{"terrain.sand", "asset.border.grass_sand", 100}};
    const fs::path brushPath = root / "Game" / "Brushes" / "brush.terrain.grass.brush.json";
    FantasyPersistence::saveBrush(brushPath, brush);
    const auto loadedBrush = FantasyPersistence::loadBrush(brushPath);
    require(loadedBrush.kind == BrushKind::Terrain && loadedBrush.variants.size() == 2U,
        "brush changed after roundtrip");
}

void testSchemaGuard(const fs::path& root) {
    const fs::path invalid = root / "Game" / "Zones" / "invalid.zone.json";
    writeText(invalid, "{\"schemaVersion\":999,\"id\":\"zone.invalid\",\"name\":\"Invalid\",\"tags\":[],\"rectangles\":[],\"tiles\":[],\"metadata\":{}}\n");
    bool rejected = false;
    try {
        (void)FantasyPersistence::loadZone(invalid);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    require(rejected, "unsupported schemaVersion must be rejected");
}

} // namespace

int main() {
    const fs::path root = fs::temp_directory_path() / "fantasy-foundation-persistence-tests";
    std::error_code ignored;
    fs::remove_all(root, ignored);
    FantasyProjectLayoutV2::ensureAuthoringDirectories(root);

    try {
        testZoneRoundtrip(root);
        testAppearanceEntityItemCreatureClass(root);
        testSystemRoundtrip(root);
        testAssetPersistence(root);
        testModernAssetAndBrush(root);
        testSchemaGuard(root);
        fs::remove_all(root, ignored);
        std::cout << "FANTASY_FOUNDATION_PERSISTENCE PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        fs::remove_all(root, ignored);
        return 1;
    }
}