#pragma once

#include "Foundation/FantasyAssetMigrationEngine.hpp"
#include "Foundation/FantasyBuildManifest.hpp"
#include "Foundation/FantasyFoundationV2.hpp"
#include "Foundation/FantasyProjectLayoutV2.hpp"
#include "Foundation/FantasySystemCatalog.hpp"
#include "Runtime/Tfs1098RuntimePreparation.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace fantasy::studio::foundation::tests {

namespace fs = std::filesystem;
using namespace fantasy::studio::runtime;

inline void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("FANTASY_FOUNDATION_V2 TEST FAIL: " + message);
}

inline void writeText(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("unable to write foundation test fixture: " + path.string());
    output << text;
    if (!output) throw std::runtime_error("failed while writing foundation test fixture: " + path.string());
}

inline void testZones() {
    ZoneDefinition zone;
    zone.id = "zone.town.center";
    zone.name = "Town Center";
    zone.tags = {"safe", "social.hub"};
    zone.rectangles.push_back(ZoneRect{100, 100, 7, 10, 8});
    zone.tiles.push_back(GridPosition{150, 150, 7});
    zone.metadata["music.id"] = "theme.town";
    zone.validate();

    require(zone.contains({100, 100, 7}), "zone must contain rectangle origin");
    require(zone.contains({109, 107, 7}), "zone must contain rectangle end");
    require(!zone.contains({110, 108, 7}), "zone rectangle bounds must be exclusive");
    require(zone.contains({150, 150, 7}), "zone must contain explicit tile");
    require(!zone.contains({150, 150, 8}), "zone must respect floor");

    bool rejectedEmptyGeometry = false;
    try {
        ZoneDefinition invalid;
        invalid.id = "zone.empty";
        invalid.name = "Empty";
        invalid.validate();
    } catch (const std::invalid_argument&) {
        rejectedEmptyGeometry = true;
    }
    require(rejectedEmptyGeometry, "zone without geometry must be rejected");
}

inline void testAppearanceAndEntities() {
    AppearanceDefinition appearance;
    appearance.id = "appearance.hero.knight";
    appearance.baseVisual = "visual.hero.base";
    appearance.attachments.push_back({"back", "visual.cape.blue", 10, 0, 0});
    appearance.attachments.push_back({"head", "visual.helmet.iron", 20, 0, -2});
    appearance.effects.push_back({"effect.aura.faint", {{"intensity", "0.25"}}});
    appearance.shaderRef = "shader.outfit.default";
    appearance.validate();

    EntityArchetype entity;
    entity.id = "entity.npc.merchant";
    entity.name = "Merchant";
    entity.tags = {"entity.interactable", "npc.merchant"};
    entity.appearanceRef = appearance.id;
    entity.components.push_back({"interaction.shop", {{"catalog.id", "shop.starter"}}});
    entity.components.push_back({"dialogue.basic", {{"dialogue.id", "merchant.welcome"}}});
    entity.validate();

    bool rejectedDuplicateLayer = false;
    try {
        AppearanceDefinition invalid = appearance;
        invalid.attachments.push_back({"back", "visual.wings", 10, 0, 0});
        invalid.validate();
    } catch (const std::invalid_argument&) {
        rejectedDuplicateLayer = true;
    }
    require(rejectedDuplicateLayer, "appearance must reject duplicate slot/layer");
}

inline void testEquipmentAndClasses() {
    EquipmentDefinition boots;
    boots.id = "equipment.boots.agility";
    boots.slot = "feet";
    boots.requirements.minimumLevel = 10;
    boots.requirements.attributes = {{"agility", 10}};
    boots.bonuses.attributes = {{"agility", 20}};
    boots.validate();

    EquipmentDefinition bow;
    bow.id = "equipment.bow.ranger";
    bow.slot = "weapon";
    bow.requirements.minimumLevel = 50;
    bow.requirements.attributes = {{"strength", 30}, {"agility", 40}};
    bow.bonuses.attributes = {{"agility", 5}};
    bow.validate();

    CharacterState character;
    character.level = 50;
    character.naturalAttributes = {{"strength", 30}, {"agility", 20}};

    const auto withoutBoots = evaluateEquipmentEligibility(character, bow);
    require(!withoutBoots.allowed, "bow must fail before existing equipment bonus is applied");
    require(withoutBoots.levelSatisfied, "level 50 must satisfy level 50 requirement");
    require(withoutBoots.missingAttributes.size() == 1U && withoutBoots.missingAttributes.front() == "agility",
        "agility must be the only missing requirement");

    character.equippedBonuses["agility"] = 20;
    const auto withBoots = evaluateEquipmentEligibility(character, bow);
    require(withBoots.allowed, "already-equipped boots must satisfy bow agility requirement");

    CharacterState lowLevel = character;
    lowLevel.level = 10;
    lowLevel.equippedBonuses["agility"] = 200;
    const auto lowLevelResult = evaluateEquipmentEligibility(lowLevel, bow);
    require(!lowLevelResult.allowed && !lowLevelResult.levelSatisfied,
        "attribute bonuses must never bypass minimum level");

    CharacterState selfBonusAttempt;
    selfBonusAttempt.level = 50;
    selfBonusAttempt.naturalAttributes = {{"strength", 30}, {"agility", 35}};
    const auto circularGuard = evaluateEquipmentEligibility(selfBonusAttempt, bow);
    require(!circularGuard.allowed,
        "an item's own bonus must not be counted to satisfy its own requirement");

    ClassDefinition ranger;
    ranger.id = "class.ranger";
    ranger.name = "Ranger";
    ranger.passiveSystems = {"system.class.ranger.passive"};
    ranger.abilitySystems = {"system.ability.aimed_shot"};
    ranger.validate();
}

inline void testItemsAndCreatures() {
    ItemDefinition item;
    item.id = "item.health_potion";
    item.name = "Health Potion";
    item.appearanceRef = "appearance.item.health_potion";
    item.tags = {"consumable", "healing"};
    item.components.push_back({"use.consume", {{"system.id", "system.item.health_potion"}}});
    item.validate();

    CreatureDefinition creature;
    creature.id = "creature.wolf.basic";
    creature.name = "Wolf";
    creature.appearanceRef = "appearance.creature.wolf";
    creature.level = 8;
    creature.movementSpeed = 120;
    creature.attributes = {{"strength", 12}, {"agility", 18}};
    creature.tags = {"hostile", "beast"};
    creature.components.push_back({"ai.melee", {{"profile.id", "ai.wolf.basic"}}});
    creature.components.push_back({"loot.table", {{"table.id", "loot.wolf.basic"}}});
    creature.validate();
}

inline SystemDefinition makeWelcomeSystem() {
    SystemDefinition system;
    system.id = "system.zone.welcome";
    system.version = 1;
    system.requiredChannels = {"ui.notification"};
    system.triggers.push_back({"trigger.enter", TriggerKind::EnterZone, {{"zone.id", "zone.town.center"}}});
    system.conditions.push_back({"condition.player", ConditionKind::HasTag, {{"tag.id", "entity.player"}}});
    system.actions.push_back({"action.notify", ActionKind::SendSystemMessage,
        {{"channel.id", "ui.notification"}, {"message.id", "town.welcome"}}});
    return system;
}

inline void testSystemLabContract() {
    const auto system = makeWelcomeSystem();
    system.validate();

    bool rejectedDuplicateNode = false;
    try {
        SystemDefinition invalid = system;
        invalid.actions.push_back({"trigger.enter", ActionKind::PersistValue, {{"key.id", "bad"}}});
        invalid.validate();
    } catch (const std::invalid_argument&) {
        rejectedDuplicateNode = true;
    }
    require(rejectedDuplicateNode, "System Lab must reject duplicate node IDs across node kinds");

    bool rejectedNoAction = false;
    try {
        SystemDefinition invalid = system;
        invalid.actions.clear();
        invalid.validate();
    } catch (const std::invalid_argument&) {
        rejectedNoAction = true;
    }
    require(rejectedNoAction, "System Lab system without actions must be rejected");
}

inline void testSystemCatalog() {
    FantasySystemCatalog catalog;

    SystemDefinition persistence;
    persistence.id = "system.persistence.player";
    persistence.version = 1;
    persistence.triggers.push_back({"trigger.login", TriggerKind::Login, {}});
    persistence.actions.push_back({"action.persist", ActionKind::PersistValue, {{"key.id", "player.state"}}});
    catalog.add(persistence);

    auto welcome = makeWelcomeSystem();
    welcome.dependencies = {"system.persistence.player"};
    catalog.add(welcome);

    catalog.validateDependencies();
    const auto order = catalog.executionOrder();
    require(order.size() == 2U, "system catalog must produce two systems in execution order");
    require(order.front() == "system.persistence.player" && order.back() == "system.zone.welcome",
        "system dependency must execute before dependent system");
    const auto channels = catalog.requiredChannels();
    require(channels.size() == 1U && *channels.begin() == "ui.notification",
        "system catalog must aggregate semantic channels");

    bool rejectedCycle = false;
    try {
        FantasySystemCatalog cyclic;
        SystemDefinition a;
        a.id = "system.a";
        a.dependencies = {"system.b"};
        a.triggers.push_back({"trigger.a", TriggerKind::Login, {}});
        a.actions.push_back({"action.a", ActionKind::PersistValue, {{"key.id", "a"}}});
        SystemDefinition b;
        b.id = "system.b";
        b.dependencies = {"system.a"};
        b.triggers.push_back({"trigger.b", TriggerKind::Login, {}});
        b.actions.push_back({"action.b", ActionKind::PersistValue, {{"key.id", "b"}}});
        cyclic.add(a);
        cyclic.add(b);
        cyclic.validateDependencies();
    } catch (const std::runtime_error&) {
        rejectedCycle = true;
    }
    require(rejectedCycle, "system dependency cycles must be rejected");
}

inline AssetMigrationPlan makeMigrationPlan() {
    AssetMigrationPlan migration;
    migration.sourceProfile = "assets.854.legacy";
    migration.targetProfile = "assets.1524.modern";
    migration.entries = {
        {100, 0, AssetMigrationMode::AddAsNew},
        {101, 5001, AssetMigrationMode::ReplaceObject},
        {102, 5002, AssetMigrationMode::ReplaceVisualOnly},
    };
    return migration;
}

inline void testAssetProfilesAndMigration() {
    AssetProfile profile;
    profile.id = "assets.1098.official";
    profile.version = 1;
    profile.sources = {
        {"dat", fs::path{"Assets/1098/Tibia.dat"}, std::string(64, 'a')},
        {"spr", fs::path{"Assets/1098/Tibia.spr"}, std::string(64, 'b')},
        {"otb", fs::path{"Assets/1098/items.otb"}, std::string(64, 'c')},
    };
    profile.validate();

    const auto migration = makeMigrationPlan();
    migration.validate();

    bool rejectedTraversal = false;
    try {
        AssetProfile invalid = profile;
        invalid.sources.front().path = fs::path{".."} / "outside.dat";
        invalid.validate();
    } catch (const std::invalid_argument&) {
        rejectedTraversal = true;
    }
    require(rejectedTraversal, "asset profile must reject path traversal");

    bool rejectedTargetCollision = false;
    try {
        AssetMigrationPlan invalid = migration;
        invalid.entries.push_back({103, 5002, AssetMigrationMode::ReplaceObject});
        invalid.validate();
    } catch (const std::invalid_argument&) {
        rejectedTargetCollision = true;
    }
    require(rejectedTargetCollision, "asset migration must reject explicit target collisions");
}

inline void testAssetMigrationEngine() {
    const auto migration = makeMigrationPlan();
    const std::set<std::uint32_t> occupied{5000, 5001, 5002, 5003};
    const auto preview = FantasyAssetMigrationEngine::preview(migration, occupied, 5000);
    require(preview.resolutions.size() == 3U, "migration preview must resolve every entry");
    require(preview.resolutions[0].legacyId == 100U && preview.resolutions[0].resolvedTargetId == 5004U,
        "AddAsNew migration must allocate the next free target id");
    require(preview.resolutions[1].resolvedTargetId == 5001U,
        "ReplaceObject migration must preserve explicit target id");
    require(preview.resolutions[2].resolvedTargetId == 5002U,
        "ReplaceVisualOnly migration must preserve explicit target id");

    bool rejectedMissingReplaceTarget = false;
    try {
        (void)FantasyAssetMigrationEngine::preview(migration, {5000}, 5000);
    } catch (const std::runtime_error&) {
        rejectedMissingReplaceTarget = true;
    }
    require(rejectedMissingReplaceTarget, "migration preview must reject replacement of missing target asset");
}

inline void testProjectLayoutV2() {
    const fs::path root = fs::temp_directory_path() / "fantasy-project-layout-v2";
    std::error_code ignored;
    fs::remove_all(root, ignored);
    FantasyProjectLayoutV2::ensureAuthoringDirectories(root);

    require(fs::is_directory(FantasyProjectLayoutV2::zonesDirectory(root)), "zones directory must be created");
    require(fs::is_directory(FantasyProjectLayoutV2::systemsDirectory(root)), "systems directory must be created");
    require(fs::is_directory(FantasyProjectLayoutV2::assetProfilesDirectory(root)), "asset profile directory must be created");
    require(FantasyProjectLayoutV2::zonePath(root, "zone.town.center").filename() == "zone.town.center.zone.json",
        "zone path must be deterministic");
    require(FantasyProjectLayoutV2::systemPath(root, "system.zone.welcome").filename() == "system.zone.welcome.system.json",
        "system path must be deterministic");

    bool rejectedUnsafeId = false;
    try {
        (void)FantasyProjectLayoutV2::itemPath(root, "../outside");
    } catch (const std::invalid_argument&) {
        rejectedUnsafeId = true;
    }
    require(rejectedUnsafeId, "project layout must reject unsafe object identifiers");
    fs::remove_all(root, ignored);
}

inline void testBuildAndHealthContracts() {
    BuildPlan plan;
    plan.validateProject = true;
    plan.buildRuntime = true;
    plan.buildClient = true;
    plan.buildVps = true;
    const auto ordered = plan.orderedTargets();
    require(ordered.size() == 4U, "full build plan must expose validate/runtime/client/VPS targets");
    require(ordered.front() == BuildTarget::Validate && ordered.back() == BuildTarget::Vps,
        "build plan target order must preserve validation first and VPS last");

    bool rejectedVpsWithoutRuntime = false;
    try {
        BuildPlan invalid;
        invalid.buildRuntime = false;
        invalid.buildVps = true;
        invalid.validate();
    } catch (const std::invalid_argument&) {
        rejectedVpsWithoutRuntime = true;
    }
    require(rejectedVpsWithoutRuntime, "VPS build must require runtime build");

    ProjectHealthInput healthy;
    healthy.hasMainMap = true;
    healthy.hasAssetProfile = true;
    healthy.hasRuntimeProfile = true;
    healthy.hasRuntimeTemplate = true;
    healthy.extendedProfile = true;
    healthy.hasClientPackage = true;
    require(evaluateProjectHealth(healthy).ready(), "complete extended project health must be ready");

    healthy.hasClientPackage = false;
    const auto missingClient = evaluateProjectHealth(healthy);
    require(!missingClient.ready(), "extended project without client package must not be ready");
}

inline void testBuildManifest() {
    FantasyBuildManifest manifest;
    manifest.projectId = "fantasy.demo";
    manifest.buildVersion = "v1.0.0";
    manifest.runtimeId = "tfs1098";
    manifest.compatibilityProfile = "otc_extended";
    manifest.assetProfileId = "assets.1098.official";
    manifest.artifacts = {
        {"runtime", fs::path{"build/runtime/tfs1098"}, std::string(64, 'd')},
        {"client", fs::path{"build/client/otcv8"}, std::string(64, 'e')},
    };
    manifest.validate();
    const auto json = FantasyBuildManifestWriter::toJson(manifest);
    require(json.find("\"runtimeId\": \"tfs1098\"") != std::string::npos,
        "build manifest JSON must include runtime id");
    require(json.find("\"compatibilityProfile\": \"otc_extended\"") != std::string::npos,
        "build manifest JSON must include compatibility profile");

    ReleasePolicy policy;
    policy.installRoot = fs::path{"/opt/fantasy/demo"};
    policy.backupRoot = fs::path{"/opt/fantasy/backups/demo"};
    policy.serviceName = "fantasy-demo";
    policy.validate();
}

inline void createExtendedRuntimeFixture(const fs::path& runtimeDirectory) {
    writeText(
        runtimeDirectory / "data" / "creaturescripts" / "creaturescripts.xml",
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<creaturescripts>\n"
        "\t<event type=\"login\" name=\"PlayerLogin\" script=\"login.lua\" />\n"
        "</creaturescripts>\n");
    writeText(
        runtimeDirectory / "data" / "creaturescripts" / "scripts" / "login.lua",
        "function onLogin(player)\n"
        "\tplayer:registerEvent(\"PlayerDeath\")\n"
        "\treturn true\n"
        "end\n");
}

inline void testRuntimePreparationV2() {
    const fs::path root = fs::temp_directory_path() / "fantasy-foundation-v2-runtime-preparation";
    const fs::path projectRoot = root / "project";
    const fs::path runtimeDirectory = root / "runtime";
    const fs::path clientDirectory = root / "client";
    std::error_code ignored;
    fs::remove_all(root, ignored);
    createExtendedRuntimeFixture(runtimeDirectory);
    fs::create_directories(clientDirectory);

    Tfs1098TargetConfig profile;
    profile.mapName = "foundation_v2";
    profile.outputDirectory = fs::path{"build"} / "runtime" / "tfs1098";
    profile.compatibilityProfile = Tfs1098CompatibilityProfile::Vanilla;
    Tfs1098RuntimeProfile::save(projectRoot, profile);

    Tfs1098RuntimePreparationRequest request;
    request.projectRoot = projectRoot;
    request.runtimeDirectory = runtimeDirectory;
    request.clientDirectory = clientDirectory;
    request.requiredChannels = {
        SystemChannelId::parse("ui.inventory"),
        SystemChannelId::parse("system.quest:v1"),
    };

    const auto vanilla = Tfs1098RuntimePreparation::prepare(request);
    require(vanilla.success, "vanilla runtime preparation must succeed");
    require(vanilla.profile == Tfs1098CompatibilityProfile::Vanilla, "vanilla runtime profile must remain vanilla");
    require(!vanilla.capabilities.canUseSystemChannels, "vanilla runtime must not advertise system channels");
    require(!vanilla.warnings.empty(), "vanilla runtime should warn when semantic channels are requested");
    require(!fs::exists(runtimeDirectory / "data" / "creaturescripts" / "scripts" / "extendedopcode.lua"),
        "vanilla preparation must not generate extended bridge");

    Tfs1098RuntimePreparation::saveCompatibilityProfile(projectRoot, Tfs1098CompatibilityProfile::OtcExtended);
    const auto extended = Tfs1098RuntimePreparation::prepare(request);
    require(extended.success, "extended runtime preparation must succeed");
    require(extended.profile == Tfs1098CompatibilityProfile::OtcExtended, "extended runtime profile must persist");
    require(extended.capabilities.canUseSystemChannels, "extended runtime must advertise semantic channels");
    require(extended.bindings.size() == 2U, "extended runtime must bind requested semantic channels");
    require(extended.bindings[0].wireCode == 200U && extended.bindings[1].wireCode == 201U,
        "extended runtime must allocate Fantasy-owned opcodes deterministically");
    require(fs::exists(runtimeDirectory / "data" / "creaturescripts" / "scripts" / "extendedopcode.lua"),
        "extended runtime must generate TFS bridge");
    require(fs::exists(clientDirectory / "mods" / "fantasy_runtime_bridge" / "fantasy_runtime_bridge.lua"),
        "extended runtime must generate OTCv8 bridge");

    const auto second = Tfs1098RuntimePreparation::prepare(request);
    require(second.success && second.bindings == extended.bindings,
        "second runtime preparation must preserve stable semantic channel bindings");

    fs::remove_all(root, ignored);
}

inline void runFoundationV2SelfTests() {
    testZones();
    testAppearanceAndEntities();
    testEquipmentAndClasses();
    testItemsAndCreatures();
    testSystemLabContract();
    testSystemCatalog();
    testAssetProfilesAndMigration();
    testAssetMigrationEngine();
    testProjectLayoutV2();
    testBuildAndHealthContracts();
    testBuildManifest();
    testRuntimePreparationV2();
}

} // namespace fantasy::studio::foundation::tests
