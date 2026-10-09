#include "Runtime/RuntimeBackend.hpp"
#include "Runtime/Tfs1098DeploymentBundle.hpp"
#include "Runtime/Tfs1098RuntimeBackend.hpp"
#include "Runtime/Tfs1098RuntimeProfile.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

using namespace fantasy::studio::runtime;
namespace fs = std::filesystem;

namespace {

void writeText(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    assert(output.good());
    output << text;
    assert(output.good());
}

std::string readText(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    assert(input.good());
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

class FakeBackend final : public RuntimeBackend {
public:
    [[nodiscard]] RuntimeKind kind() const noexcept override { return RuntimeKind::Tfs1098; }
    [[nodiscard]] const char* id() const noexcept override { return "test-tfs1098"; }
    [[nodiscard]] const char* displayName() const noexcept override { return "Test TFS 1.4.2 / 10.98"; }
    [[nodiscard]] RuntimeCapabilities capabilities() const noexcept override {
        return RuntimeCapabilities{true, false, false, false};
    }

    [[nodiscard]] RuntimePackageReport packageProject(const RuntimePackageRequest& request) override {
        RuntimePackageReport report;
        if (request.projectRoot.empty() || request.outputDirectory.empty()) {
            report.errors.emplace_back("projectRoot and outputDirectory are required");
            return report;
        }
        report.success = true;
        report.generatedFiles.push_back(request.outputDirectory / "world.otbm");
        return report;
    }
};

void testNeutralContract() {
    FakeBackend backend;
    assert(backend.kind() == RuntimeKind::Tfs1098);
    assert(backend.capabilities().canPackageProject);
    assert(!backend.capabilities().canLaunch);

    const RuntimePackageRequest request{
        fs::path{"FantasyProject"},
        fs::path{"Build/TFS1098"},
    };
    const RuntimePackageReport report = backend.packageProject(request);
    assert(report.success);
    assert(report.generatedFiles.size() == 1);
    assert(report.generatedFiles.front().filename() == "world.otbm");

    const auto unsupportedLaunch = backend.launch(RuntimeLaunchRequest{});
    assert(!unsupportedLaunch.success);
    assert(!unsupportedLaunch.errors.empty());
    assert(backend.status().state == RuntimeState::NotPrepared);
}

void testTargetProfile() {
    const fs::path root = fs::temp_directory_path() / "fantasy-tfs1098-profile-tests";
    std::error_code ignored;
    fs::remove_all(root, ignored);

    const Tfs1098TargetConfig defaults = Tfs1098RuntimeProfile::load(root);
    assert(defaults.mapName == "fantasy");
    assert(defaults.outputDirectory == fs::path{"build"} / "runtime" / "tfs1098");

    Tfs1098TargetConfig configured;
    configured.mapName = "world_alpha";
    configured.outputDirectory = fs::path{"build"} / "targets" / "tfs1098";
    Tfs1098RuntimeProfile::save(root, configured);

    const auto profilePath = Tfs1098RuntimeProfile::pathForProject(root);
    assert(fs::exists(profilePath));
    const Tfs1098TargetConfig loaded = Tfs1098RuntimeProfile::load(root);
    assert(loaded.mapName == configured.mapName);
    assert(loaded.outputDirectory == configured.outputDirectory);
    assert(
        Tfs1098RuntimeProfile::resolveOutputDirectory(root, loaded) ==
        fs::absolute(root / configured.outputDirectory).lexically_normal());

    bool rejectedTraversal = false;
    try {
        Tfs1098TargetConfig invalid = configured;
        invalid.outputDirectory = fs::path{".."} / "outside";
        Tfs1098RuntimeProfile::validate(invalid);
    } catch (const std::runtime_error&) {
        rejectedTraversal = true;
    }
    assert(rejectedTraversal);

    bool rejectedMapName = false;
    try {
        Tfs1098TargetConfig invalid = configured;
        invalid.mapName = "../world";
        Tfs1098RuntimeProfile::validate(invalid);
    } catch (const std::runtime_error&) {
        rejectedMapName = true;
    }
    assert(rejectedMapName);

    fs::remove_all(root, ignored);
}

void testTfsBackend(const fs::path& selfExecutable) {
    const fs::path root = fs::temp_directory_path() / "fantasy-tfs1098-runtime-tests";
    std::error_code ignored;
    fs::remove_all(root, ignored);

    const fs::path runtimeTemplate = root / "template";
    const fs::path inputs = root / "inputs";
    const fs::path output = root / "runtime";

    writeText(runtimeTemplate / "config.lua.dist",
        "serverName = \"Synthetic\"\n"
        "mapName = \"forgotten\"\n"
        "loginProtocolPort = 7171\n");
    writeText(runtimeTemplate / "data" / "static.txt", "template-marker\n");
    writeText(runtimeTemplate / "tfs", "synthetic-tfs-binary\n");
    writeText(inputs / "world.otbm", "synthetic-otbm\n");
    writeText(inputs / "items.otb", "synthetic-otb\n");
    writeText(inputs / "map-house.xml", "<houses/>\n");
    writeText(inputs / "map-spawn.xml", "<spawns/>\n");

    Tfs1098RuntimeBackend backend;
    const auto capabilities = backend.capabilities();
    assert(capabilities.canPackageProject);
    assert(capabilities.canLaunch);
    assert(capabilities.canStop);
    assert(capabilities.canStreamLogs);

    RuntimePackageRequest package;
    package.projectRoot = root;
    package.outputDirectory = output;
    package.runtimeTemplateDirectory = runtimeTemplate;
    package.exportedMapPath = inputs / "world.otbm";
    package.itemsOtbPath = inputs / "items.otb";
    package.houseXmlPath = inputs / "map-house.xml";
    package.spawnXmlPath = inputs / "map-spawn.xml";
    package.mapName = "fantasy_test";

    const RuntimePackageReport packaged = backend.packageProject(package);
    assert(packaged.success);
    assert(packaged.errors.empty());
    assert(fs::exists(output / "data" / "world" / "fantasy_test.otbm"));
    assert(fs::exists(output / "data" / "world" / "map-house.xml"));
    assert(fs::exists(output / "data" / "world" / "map-spawn.xml"));
    assert(fs::exists(output / "data" / "items" / "items.otb"));
    assert(readText(output / "data" / "static.txt") == "template-marker\n");
    const std::string config = readText(output / "config.lua");
    assert(config.find("mapName = \"fantasy_test\"") != std::string::npos);
    assert(config.find("mapName = \"forgotten\"") == std::string::npos);
    assert(backend.status().state == RuntimeState::Stopped);

    Tfs1098DeploymentBundleRequest deployment;
    deployment.runtimeDirectory = output;
    deployment.outputDirectory = root / "vps-bundle";
    deployment.serviceName = "fantasy-test";
    deployment.serviceUser = "fantasytest";
    deployment.installRoot = "/opt/fantasy/fantasy-test";
    deployment.executableName = "tfs";

    const Tfs1098DeploymentBundleReport deployed = Tfs1098DeploymentBundle::build(deployment);
    assert(deployed.success);
    assert(deployed.errors.empty());
    assert(fs::exists(deployment.outputDirectory / "server" / "config.lua"));
    assert(fs::exists(deployment.outputDirectory / "server" / "data" / "world" / "fantasy_test.otbm"));
    assert(fs::exists(deployment.outputDirectory / "deploy" / "fantasy-test.service"));
    assert(fs::exists(deployment.outputDirectory / "deploy" / "install.sh"));
    assert(fs::exists(deployment.outputDirectory / "deploy" / "manifest.json"));

    const std::string service = readText(deployment.outputDirectory / "deploy" / "fantasy-test.service");
    assert(service.find("User=fantasytest") != std::string::npos);
    assert(service.find("WorkingDirectory=/opt/fantasy/fantasy-test/server") != std::string::npos);
    assert(service.find("Restart=on-failure") != std::string::npos);

    const std::string installer = readText(deployment.outputDirectory / "deploy" / "install.sh");
    assert(installer.find("systemctl enable --now") != std::string::npos);
    const std::string manifest = readText(deployment.outputDirectory / "deploy" / "manifest.json");
    assert(manifest.find("\"runtime\": \"tfs1098\"") != std::string::npos);

    Tfs1098DeploymentBundleRequest invalidDeployment = deployment;
    invalidDeployment.outputDirectory = root / "invalid-vps-bundle";
    invalidDeployment.installRoot = "../unsafe";
    const auto rejectedDeployment = Tfs1098DeploymentBundle::build(invalidDeployment);
    assert(!rejectedDeployment.success);
    assert(!rejectedDeployment.errors.empty());

    const fs::path logFile = output / "runtime-test.log";
    RuntimeLaunchRequest launch;
    launch.runtimeDirectory = output;
    launch.executable = selfExecutable;
    launch.arguments = {"--runtime-child"};
    launch.logFile = logFile;

    const RuntimeLaunchReport launched = backend.launch(launch);
    assert(launched.success);
    assert(launched.processId != 0);
    assert(backend.status().state == RuntimeState::Running);

    bool sawMarker = false;
    std::uint64_t cursor = 0;
    for (int attempt = 0; attempt < 40 && !sawMarker; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        const RuntimeLogChunk logs = backend.readLogs(cursor);
        assert(logs.errors.empty());
        cursor = logs.nextCursor;
        sawMarker = logs.text.find("FANTASY_RUNTIME_CHILD_READY") != std::string::npos;
    }
    assert(sawMarker);

    const RuntimeStopReport stopped = backend.stop();
    assert(stopped.success);
    assert(backend.status().state == RuntimeState::Stopped);

    fs::remove_all(root, ignored);
}

} // namespace

int main(int argc, char** argv) {
    if (argc >= 2 && std::string(argv[1]) == "--runtime-child") {
        std::cout << "FANTASY_RUNTIME_CHILD_READY" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return 0;
    }

    assert(argc >= 1);
    testNeutralContract();
    testTargetProfile();
    testTfsBackend(fs::absolute(fs::path(argv[0])));
    return 0;
}
