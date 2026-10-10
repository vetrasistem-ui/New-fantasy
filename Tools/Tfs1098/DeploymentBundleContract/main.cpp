#include "Studio/Runtime/Tfs1098DeploymentBundle.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using fantasy::studio::runtime::Tfs1098DeploymentBundle;
using fantasy::studio::runtime::Tfs1098DeploymentBundleRequest;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("TFS1098_DEPLOYMENT_CONTRACT FAIL: " + message);
}

void writeText(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    require(output.good(), "unable to write fixture: " + path.string());
    output << text;
    require(output.good(), "failed while writing fixture: " + path.string());
}

std::string readText(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    require(input.good(), "unable to read generated file: " + path.string());
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void requireFile(const fs::path& path) {
    require(fs::is_regular_file(path), "missing generated file: " + path.string());
}

void testLifecycleBundle(const fs::path& root) {
    const fs::path runtime = root / "runtime";
    const fs::path output = root / "bundle";

    writeText(runtime / "tfs", "synthetic-linux-tfs\n");
    writeText(runtime / "config.lua", "mapName = \"fantasy\"\n");
    writeText(runtime / "data" / "marker.txt", "keep-me\n");

    Tfs1098DeploymentBundleRequest request;
    request.runtimeDirectory = runtime;
    request.outputDirectory = output;
    request.serviceName = "fantasy-contract";
    request.serviceUser = "fantasycontract";
    request.installRoot = "/opt/fantasy/fantasy-contract";
    request.executableName = "tfs";

    const auto report = Tfs1098DeploymentBundle::build(request);
    require(report.success, report.errors.empty() ? "bundle build failed without error detail" : report.errors.front());
    require(report.errors.empty(), "bundle reported errors despite success");
    require(report.generatedFiles.size() >= 10U, "bundle report does not include base + lifecycle outputs");

    requireFile(output / "server" / "tfs");
    requireFile(output / "server" / "config.lua");
    requireFile(output / "server" / "data" / "marker.txt");
    requireFile(output / "deploy" / "fantasy-contract.service");
    requireFile(output / "deploy" / "install.sh");
    requireFile(output / "deploy" / "README.md");
    requireFile(output / "deploy" / "manifest.json");
    requireFile(output / "deploy" / "env.example");
    requireFile(output / "deploy" / "healthcheck.sh");
    requireFile(output / "deploy" / "update.sh");
    requireFile(output / "deploy" / "rollback.sh");
    requireFile(output / "deploy" / "release-policy.json");

    require(readText(runtime / "data" / "marker.txt") == "keep-me\n", "source runtime was modified");

    const auto service = readText(output / "deploy" / "fantasy-contract.service");
    require(service.find("User=fantasycontract") != std::string::npos, "systemd service user missing");
    require(service.find("Restart=on-failure") != std::string::npos, "systemd restart policy missing");
    require(service.find("LimitNOFILE=65535") != std::string::npos, "systemd NOFILE limit missing");
    require(service.find("/opt/fantasy/fantasy-contract/server/tfs") != std::string::npos, "systemd ExecStart mismatch");

    const auto installer = readText(output / "deploy" / "install.sh");
    require(installer.find("systemctl enable --now") != std::string::npos, "installer does not enable/start service");
    require(installer.find("chmod 0755") != std::string::npos, "installer does not restore executable permission");

    const auto envExample = readText(output / "deploy" / "env.example");
    require(envExample.find("DB_PASSWORD=CHANGE_ME") != std::string::npos, "env example must contain placeholder secret");
    require(envExample.find("password123") == std::string::npos, "env example contains an unexpected concrete secret");

    const auto health = readText(output / "deploy" / "healthcheck.sh");
    require(health.find("systemctl is-active --quiet fantasy-contract") != std::string::npos, "healthcheck does not verify service state");
    require(health.find("FANTASY_VPS_HEALTH PASS") != std::string::npos, "healthcheck receipt missing");

    const auto update = readText(output / "deploy" / "update.sh");
    require(update.find("BACKUP_ROOT='/opt/fantasy/backups/fantasy-contract'") != std::string::npos, "update backup root mismatch");
    require(update.find("Health check failed; rolling back") != std::string::npos, "update does not rollback on failed health check");
    require(update.find("healthcheck.sh") != std::string::npos, "update does not invoke healthcheck");
    require(update.find("FANTASY_VPS_UPDATE PASS") != std::string::npos, "update receipt missing");

    const auto rollback = readText(output / "deploy" / "rollback.sh");
    require(rollback.find("BACKUP_ROOT='/opt/fantasy/backups/fantasy-contract'") != std::string::npos, "rollback backup root mismatch");
    require(rollback.find("FANTASY_VPS_ROLLBACK PASS") != std::string::npos, "rollback receipt missing");

    const auto policy = readText(output / "deploy" / "release-policy.json");
    require(policy.find("\"backupBeforeUpdate\": true") != std::string::npos, "release policy backup flag missing");
    require(policy.find("\"rollbackOnFailedHealthCheck\": true") != std::string::npos, "release policy rollback flag missing");

    const auto manifest = readText(output / "deploy" / "manifest.json");
    require(manifest.find("\"runtime\": \"tfs1098\"") != std::string::npos, "runtime manifest value missing");
}

void testSafetyGuards(const fs::path& root) {
    const fs::path runtime = root / "safe-runtime";
    writeText(runtime / "tfs", "synthetic-linux-tfs\n");

    Tfs1098DeploymentBundleRequest request;
    request.runtimeDirectory = runtime;
    request.outputDirectory = runtime / "unsafe-child";
    request.installRoot = "/opt/fantasy/safe";
    auto report = Tfs1098DeploymentBundle::build(request);
    require(!report.success, "bundle output inside runtime must be rejected");
    require(!fs::exists(request.outputDirectory), "unsafe child output should not be created");

    request.outputDirectory = root;
    report = Tfs1098DeploymentBundle::build(request);
    require(!report.success, "bundle output containing runtime must be rejected");
    require(fs::is_regular_file(runtime / "tfs"), "parent-output rejection modified source runtime");

    request.outputDirectory = root / "invalid-install-root";
    request.installRoot = "../unsafe";
    report = Tfs1098DeploymentBundle::build(request);
    require(!report.success, "relative/traversal install root must be rejected");
}

} // namespace

int main() {
    const fs::path root = fs::temp_directory_path() / "fantasy-tfs1098-deployment-contract";
    std::error_code ignored;
    fs::remove_all(root, ignored);

    try {
        testLifecycleBundle(root);
        fs::remove_all(root, ignored);
        fs::create_directories(root);
        testSafetyGuards(root);
        fs::remove_all(root, ignored);
        std::cout << "TFS1098_DEPLOYMENT_CONTRACT PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        fs::remove_all(root, ignored);
        return 1;
    }
}
