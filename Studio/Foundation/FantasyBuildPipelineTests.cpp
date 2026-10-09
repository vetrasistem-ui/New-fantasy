#include "Foundation/FantasyBuildPipeline.hpp"
#include "Foundation/FantasyServerWorkspaceController.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using namespace fantasy::studio;
using namespace fantasy::studio::foundation;
using namespace fantasy::studio::runtime;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("FANTASY_BUILD_PIPELINE TEST FAIL: " + message);
}

void writeText(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    require(output.good(), "unable to write fixture: " + path.string());
    output << text;
    require(output.good(), "failed while writing fixture: " + path.string());
}

ProjectInfo createProjectFixture(const fs::path& root) {
    ProjectInfo project;
    project.root = root;
    project.manifestPath = root / "fantasy.project.json";
    project.mainMapPath = root / "Maps" / "main.fmap";
    project.protocolSpecPath = root / "Protocols" / "fantasy-protocol-v1.md";
    project.mapSchemaPath = root / "Schemas" / "fmap-v0.md";
    project.name = "Pipeline Fixture";

    writeText(project.manifestPath, "{\"name\":\"Pipeline Fixture\"}\n");
    writeText(project.mainMapPath, "FMAP-SYNTHETIC\n");
    writeText(project.protocolSpecPath, "protocol fixture\n");
    writeText(project.mapSchemaPath, "schema fixture\n");
    return project;
}

void createTfsFixture(const fs::path& directory) {
    fs::create_directories(directory);
    writeText(
        directory / "data" / "creaturescripts" / "creaturescripts.xml",
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<creaturescripts>\n"
        "\t<event type=\"login\" name=\"PlayerLogin\" script=\"login.lua\" />\n"
        "</creaturescripts>\n");
    writeText(
        directory / "data" / "creaturescripts" / "scripts" / "login.lua",
        "function onLogin(player)\n"
        "\tplayer:registerEvent(\"PlayerDeath\")\n"
        "\treturn true\n"
        "end\n");
}

void populateAuthoring(const fs::path& root) {
    FantasyAuthoringRepository repository(root);

    AssetProfile assets;
    assets.id = "assets.1098.official";
    assets.version = 1;
    assets.sources = {
        {"dat", fs::path{"Assets/1098/Tibia.dat"}, std::string(64, 'a')},
        {"spr", fs::path{"Assets/1098/Tibia.spr"}, std::string(64, 'b')},
        {"otb", fs::path{"Assets/1098/items.otb"}, std::string(64, 'c')},
    };
    repository.save(assets);

    SystemDefinition persistence;
    persistence.id = "system.persistence.player";
    persistence.version = 1;
    persistence.triggers = {{"trigger.login", TriggerKind::Login, {}}};
    persistence.actions = {{"action.persist", ActionKind::PersistValue, {{"key.id", "player.state"}}}};
    repository.save(persistence);

    SystemDefinition inventory;
    inventory.id = "system.inventory.sync";
    inventory.version = 1;
    inventory.dependencies = {persistence.id};
    inventory.requiredChannels = {"ui.inventory"};
    inventory.triggers = {{"trigger.login", TriggerKind::Login, {}}};
    inventory.actions = {{"action.sync", ActionKind::SendSystemMessage, {{"channel.id", "ui.inventory"}}}};
    repository.save(inventory);

    SystemDefinition quest;
    quest.id = "system.quest.sync";
    quest.version = 1;
    quest.dependencies = {persistence.id};
    quest.requiredChannels = {"system.quest:v1"};
    quest.triggers = {{"trigger.login", TriggerKind::Login, {}}};
    quest.actions = {{"action.sync", ActionKind::SendSystemMessage, {{"channel.id", "system.quest:v1"}}}};
    repository.save(quest);
}

void configureExtendedProfile(const fs::path& root) {
    Tfs1098TargetConfig profile;
    profile.mapName = "pipeline_test";
    profile.outputDirectory = fs::path{"build"} / "runtime" / "tfs1098";
    profile.compatibilityProfile = Tfs1098CompatibilityProfile::OtcExtended;
    Tfs1098RuntimeProfile::save(root, profile);
}

FantasyBuildPipelineRequest makeRequest(
    const ProjectInfo& project,
    const fs::path& runtimeTemplate,
    const fs::path& runtime,
    const fs::path& client,
    const fs::path& vps) {

    FantasyBuildPipelineRequest request;
    request.project = project;
    request.runtimeTemplateDirectory = runtimeTemplate;
    request.runtimeDirectory = runtime;
    request.clientPackageDirectory = client;
    request.vpsBundleDirectory = vps;
    request.projectId = "fantasy.pipeline.test";
    request.buildVersion = "v1.2.3";
    request.assetProfileId = "assets.1098.official";
    request.serverHost = "play.example.com";
    request.loginPort = 7171;
    request.gamePort = 7172;
    request.plan.validateProject = true;
    request.plan.buildRuntime = true;
    request.plan.buildClient = true;
    return request;
}

void testExtendedPipeline(const fs::path& root) {
    const auto project = createProjectFixture(root / "project");
    populateAuthoring(project.root);
    configureExtendedProfile(project.root);

    const fs::path runtimeTemplate = root / "runtime-template";
    const fs::path runtime = project.root / "build" / "runtime" / "tfs1098";
    const fs::path client = project.root / "build" / "client" / "otcv8";
    const fs::path vps = project.root / "build" / "deploy" / "tfs1098-vps";
    fs::create_directories(runtimeTemplate);
    createTfsFixture(runtime);
    fs::create_directories(client);

    auto request = makeRequest(project, runtimeTemplate, runtime, client, vps);
    const auto report = FantasyBuildPipeline::prepare(request);
    require(report.success, report.errors.empty() ? "extended pipeline failed" : report.errors.front());
    require(report.health.ready(), "extended pipeline health must be ready");
    require(report.orderedSystems.size() == 3U, "pipeline must resolve three systems");
    require(report.orderedSystems.front() == "system.persistence.player",
        "dependency system must appear before its dependents");
    require(report.requiredChannels == std::vector<std::string>({"system.quest:v1", "ui.inventory"}),
        "pipeline must aggregate and sort semantic channels");
    require(report.runtimePreparation.bindings.size() == 2U,
        "pipeline must bind both required semantic channels");
    require(report.runtimePreparation.bindings[0].wireCode == 200U &&
            report.runtimePreparation.bindings[1].wireCode == 201U,
        "pipeline must allocate Fantasy-owned extended opcodes deterministically");
    require(fs::is_regular_file(project.root / "build" / "fantasy-build-manifest.json"),
        "pipeline build manifest missing");
    require(fs::is_regular_file(project.root / "build" / "fantasy-client-manifest.json"),
        "pipeline client manifest missing");
    require(fs::is_regular_file(client / "mods" / "fantasy_runtime_bridge" / "fantasy_runtime_bridge.lua"),
        "pipeline did not generate OTCv8 bridge");
}

void testServerWorkspaceController(const fs::path& root) {
    const auto project = createProjectFixture(root / "project-controller");
    populateAuthoring(project.root);
    configureExtendedProfile(project.root);

    const fs::path runtimeTemplate = root / "runtime-template-controller";
    const fs::path runtime = project.root / "build" / "runtime" / "tfs1098";
    const fs::path client = project.root / "build" / "client" / "otcv8";
    fs::create_directories(runtimeTemplate);
    createTfsFixture(runtime);
    fs::create_directories(client);

    FantasyServerWorkspaceController controller(project, runtimeTemplate, client);
    require(controller.state().health.ready(), "Server workspace controller health must be ready");
    require(controller.state().requiredChannels == std::vector<std::string>({"system.quest:v1", "ui.inventory"}),
        "Server workspace must derive semantic channels from authored systems");
    require(controller.state().profile.compatibilityProfile == Tfs1098CompatibilityProfile::OtcExtended,
        "Server workspace must load persisted OTC Extended profile");
    require(controller.state().capabilities.canUseSystemChannels,
        "OTC Extended Server workspace must advertise semantic channels");

    const auto prepared = controller.prepareRuntime();
    require(prepared.success && prepared.bindings.size() == 2U,
        "Server workspace Prepare Runtime must generate extended channel bindings");

    controller.setCompatibilityProfile(Tfs1098CompatibilityProfile::Vanilla);
    require(controller.state().profile.compatibilityProfile == Tfs1098CompatibilityProfile::Vanilla,
        "Server workspace must persist profile switch to vanilla");
    require(!controller.state().capabilities.canUseSystemChannels,
        "Vanilla Server workspace must not advertise semantic channels");
}

void testVpsLifecyclePipeline(const fs::path& root) {
    const auto project = createProjectFixture(root / "project-vps");
    populateAuthoring(project.root);
    configureExtendedProfile(project.root);

    const fs::path runtimeTemplate = root / "runtime-template-vps";
    const fs::path runtime = project.root / "build" / "runtime" / "tfs1098";
    const fs::path client = project.root / "build" / "client" / "otcv8";
    const fs::path vps = project.root / "build" / "deploy" / "tfs1098-vps";
    fs::create_directories(runtimeTemplate);
    createTfsFixture(runtime);
    fs::create_directories(client);
    writeText(vps / "server" / "config.lua", "serverName = \"Pipeline VPS\"\n");

    auto request = makeRequest(project, runtimeTemplate, runtime, client, vps);
    request.plan.buildVps = true;
    const auto report = FantasyBuildPipeline::prepare(request);
    require(report.success, report.errors.empty() ? "VPS pipeline failed" : report.errors.front());
    require(fs::is_regular_file(vps / "deploy" / "healthcheck.sh"),
        "VPS pipeline healthcheck missing");
    require(fs::is_regular_file(vps / "deploy" / "update.sh"),
        "VPS pipeline update script missing");
    require(fs::is_regular_file(vps / "deploy" / "rollback.sh"),
        "VPS pipeline rollback script missing");
    require(fs::is_regular_file(vps / "deploy" / "env.example"),
        "VPS pipeline env example missing");
}

void testHealthBlocksInvalidBuild(const fs::path& root) {
    const auto project = createProjectFixture(root / "project-invalid");
    configureExtendedProfile(project.root);
    const fs::path runtimeTemplate = root / "runtime-template-invalid";
    const fs::path runtime = project.root / "build" / "runtime" / "tfs1098";
    const fs::path client = project.root / "build" / "client" / "otcv8";
    fs::create_directories(runtimeTemplate);
    createTfsFixture(runtime);
    fs::create_directories(client);

    auto request = makeRequest(project, runtimeTemplate, runtime, client, {});
    const auto report = FantasyBuildPipeline::prepare(request);
    require(!report.success, "pipeline must fail when asset profile is missing");
    require(!report.health.ready(), "invalid project health must not be ready");
}

} // namespace

int main() {
    const fs::path root = fs::temp_directory_path() / "fantasy-build-pipeline-tests";
    std::error_code ignored;
    fs::remove_all(root, ignored);

    try {
        testExtendedPipeline(root);
        testServerWorkspaceController(root);
        testVpsLifecyclePipeline(root);
        testHealthBlocksInvalidBuild(root);
        fs::remove_all(root, ignored);
        std::cout << "FANTASY_BUILD_PIPELINE PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        fs::remove_all(root, ignored);
        return 1;
    }
}
