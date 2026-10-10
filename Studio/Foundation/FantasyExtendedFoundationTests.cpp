#include "Foundation/FantasyAssetMigrationReceipt.hpp"
#include "Foundation/FantasyClientUpdatePlanner.hpp"
#include "Foundation/FantasySystemLabContracts.hpp"
#include "Foundation/FantasyWindowsInstallerBundle.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using namespace fantasy::studio::foundation;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("FANTASY_EXTENDED_FOUNDATION TEST FAIL: " + message);
}

void writeText(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    require(output.good(), "unable to write fixture: " + path.string());
    output << text;
    require(output.good(), "failed while writing fixture: " + path.string());
}

SystemDefinition makeSystemGraph() {
    SystemDefinition graph;
    graph.id = "system.zone.welcome";
    graph.version = 1;
    graph.requiredChannels = {"ui.notification"};
    graph.triggers = {{"trigger.enter", TriggerKind::EnterZone, {{"zone.id", "zone.town.center"}}}};
    graph.conditions = {{"condition.player", ConditionKind::HasTag, {{"tag.id", "entity.player"}}}};
    graph.actions = {{"action.notify", ActionKind::SendSystemMessage,
        {{"channel.id", "ui.notification"}, {"message.id", "town.welcome"}}}};
    return graph;
}

void testSystemLabAdvancedContracts() {
    SystemPackageDefinition package;
    package.id = "system.zone.welcome";
    package.version = 3;
    package.authoringMode = SystemAuthoringMode::Hybrid;
    package.graph = makeSystemGraph();
    package.variables = {
        {"welcome.count", SystemValueType::Integer, "0", true},
        {"last.zone", SystemValueType::ZoneRef, "zone.none", true},
    };
    package.events = {
        {"event.welcome.sent", {{"player.id", SystemValueType::EntityRef}}},
    };
    package.timers = {
        {"timer.cooldown", 5000, false},
    };
    package.tests = {
        {"test.enter.town", "trigger.enter", {{"player.tag", "entity.player"}},
            {{"zone.id", "zone.town.center"}}, {{"action.notify", {{"channel.id", "ui.notification"}}}}},
    };
    package.codeEntry = "Scripts/system.zone.welcome.lua";
    package.validate();

    const auto results = FantasySystemLabValidator::validateScenarios(package);
    require(results.size() == 1U && results.front().passed,
        "valid System Lab scenario must pass structural validation");

    auto broken = package;
    broken.tests.front().expectedActions.front().actionId = "action.missing";
    const auto brokenResults = FantasySystemLabValidator::validateScenarios(broken);
    require(!brokenResults.front().passed && !brokenResults.front().failures.empty(),
        "System Lab scenario must detect missing expected action");

    bool rejectedCodeModeWithoutEntry = false;
    try {
        auto invalid = package;
        invalid.authoringMode = SystemAuthoringMode::Code;
        invalid.codeEntry.clear();
        invalid.validate();
    } catch (const std::invalid_argument&) {
        rejectedCodeModeWithoutEntry = true;
    }
    require(rejectedCodeModeWithoutEntry, "code authoring mode must require codeEntry");
}

FantasyClientManifest makeClientManifest(
    const std::string& version,
    std::vector<ClientArtifactDescriptor> artifacts) {

    FantasyClientManifest manifest;
    manifest.clientId = "fantasy-client";
    manifest.version = version;
    manifest.runtimeId = "tfs1098";
    manifest.compatibilityProfile = "otc_extended";
    manifest.assetProfileId = "assets.1098.official";
    manifest.serverHost = "play.example.com";
    manifest.modules = {{"fantasy_runtime_bridge", true, "v1"}};
    manifest.artifacts = std::move(artifacts);
    return manifest;
}

void testClientUpdatePlanner() {
    const auto current = makeClientManifest(
        "v1.0.0",
        {
            {fs::path{"bin/otclient.exe"}, std::string(64, 'a')},
            {fs::path{"mods/old.otmod"}, std::string(64, 'b')},
            {fs::path{"data/static.dat"}, std::string(64, 'c')},
        });
    const auto target = makeClientManifest(
        "v1.1.0",
        {
            {fs::path{"bin/otclient.exe"}, std::string(64, 'd')},
            {fs::path{"mods/fantasy_runtime_bridge.otmod"}, std::string(64, 'e')},
            {fs::path{"data/static.dat"}, std::string(64, 'c')},
        });

    const auto plan = FantasyClientUpdatePlanner::compare(current, target);
    require(plan.requiresChanges(), "changed client manifest must require update operations");
    std::size_t downloads = 0;
    std::size_t removals = 0;
    std::size_t keeps = 0;
    for (const auto& operation : plan.operations) {
        if (operation.kind == ClientUpdateOperationKind::Download) ++downloads;
        else if (operation.kind == ClientUpdateOperationKind::Remove) ++removals;
        else ++keeps;
    }
    require(downloads == 2U && removals == 1U && keeps == 1U,
        "client update plan must classify download/remove/keep operations correctly");

    auto incompatible = target;
    incompatible.assetProfileId = "assets.1524.modern";
    bool rejectedAssetSwitch = false;
    try {
        (void)FantasyClientUpdatePlanner::compare(current, incompatible);
    } catch (const std::invalid_argument&) {
        rejectedAssetSwitch = true;
    }
    require(rejectedAssetSwitch, "launcher update must not silently switch asset profiles");
}

void testAssetMigrationReceiptAndRollback() {
    AssetMigrationPlan plan;
    plan.sourceProfile = "assets.854.legacy";
    plan.targetProfile = "assets.1524.modern";
    plan.entries = {
        {100, 0, AssetMigrationMode::AddAsNew},
        {101, 5001, AssetMigrationMode::ReplaceObject},
        {102, 5002, AssetMigrationMode::ReplaceVisualOnly},
    };

    const std::set<std::uint32_t> occupied{5000, 5001, 5002};
    const auto preview = FantasyAssetMigrationEngine::preview(plan, occupied, 5000);
    const std::map<std::uint32_t, AssetMigrationBackup> backups = {
        {5001, {5001, "backup/object/5001", "backup/visual/5001"}},
        {5002, {5002, "backup/object/5002", "backup/visual/5002"}},
    };

    const auto receipt = FantasyAssetMigrationReceiptBuilder::create(
        "migration.854.to.1524.001", plan, preview, backups);
    require(receipt.entries.size() == 3U, "migration receipt must contain every applied entry");
    require(receipt.entries.front().createdTarget, "AddAsNew receipt entry must mark created target");

    const auto rollback = FantasyAssetMigrationReceiptBuilder::rollbackPlan(receipt);
    require(rollback.size() == 3U, "rollback plan must contain every receipt operation");
    require(rollback[0].kind == AssetRollbackOperationKind::RestoreVisual && rollback[0].targetId == 5002U,
        "rollback must reverse ReplaceVisualOnly first");
    require(rollback[1].kind == AssetRollbackOperationKind::RestoreObject && rollback[1].targetId == 5001U,
        "rollback must restore replaced object");
    require(rollback[2].kind == AssetRollbackOperationKind::RemoveCreatedTarget,
        "rollback must remove newly created asset last");
}

void testWindowsInstallerBundle(const fs::path& root) {
    const fs::path package = root / "package-source";
    const fs::path output = root / "installer";
    writeText(package / "FantasyStudio.exe", "synthetic-executable\n");
    writeText(package / "StudioVisual" / "logo.png", "synthetic-visual\n");

    FantasyWindowsInstallerRequest request;
    request.packageDirectory = package;
    request.outputDirectory = output;
    request.productId = "fantasy-studio";
    request.displayName = "Fantasy Studio";
    request.installFolder = "FantasyStudio";
    request.executableName = "FantasyStudio.exe";
    request.createDesktopShortcut = true;

    const auto report = FantasyWindowsInstallerBundle::write(request);
    require(report.success, report.errors.empty() ? "Windows installer generation failed" : report.errors.front());
    require(fs::is_regular_file(output / "install.ps1"), "Windows install script missing");
    require(fs::is_regular_file(output / "uninstall.ps1"), "Windows uninstall script missing");
    require(fs::is_regular_file(output / "installer-manifest.json"), "Windows installer manifest missing");

    std::ifstream install(output / "install.ps1", std::ios::binary);
    const std::string installText{
        std::istreambuf_iterator<char>{install},
        std::istreambuf_iterator<char>{}};
    require(installText.find("$env:LOCALAPPDATA") != std::string::npos,
        "installer must use per-user LOCALAPPDATA location");
    require(installText.find("WScript.Shell") != std::string::npos,
        "installer must generate desktop shortcut when requested");

    FantasyWindowsInstallerRequest unsafe = request;
    unsafe.installFolder = "../outside";
    const auto rejected = FantasyWindowsInstallerBundle::write(unsafe);
    require(!rejected.success, "installer generator must reject unsafe install folders");
}

} // namespace

int main() {
    const fs::path root = fs::temp_directory_path() / "fantasy-extended-foundation-tests";
    std::error_code ignored;
    fs::remove_all(root, ignored);

    try {
        testSystemLabAdvancedContracts();
        testClientUpdatePlanner();
        testAssetMigrationReceiptAndRollback();
        testWindowsInstallerBundle(root);
        fs::remove_all(root, ignored);
        std::cout << "FANTASY_EXTENDED_FOUNDATION PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        fs::remove_all(root, ignored);
        return 1;
    }
}
