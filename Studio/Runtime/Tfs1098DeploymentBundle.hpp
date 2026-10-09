#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace fantasy::studio::runtime {

struct Tfs1098DeploymentBundleRequest {
    std::filesystem::path runtimeDirectory;
    std::filesystem::path outputDirectory;
    std::string serviceName = "fantasy-tfs1098";
    std::string serviceUser = "fantasy";
    std::string installRoot = "/opt/fantasy/fantasy-tfs1098";
    std::string executableName = "tfs";
};

struct Tfs1098DeploymentBundleReport {
    bool success = false;
    std::vector<std::filesystem::path> generatedFiles;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

class Tfs1098DeploymentBundle {
public:
    [[nodiscard]] static Tfs1098DeploymentBundleReport build(
        const Tfs1098DeploymentBundleRequest& request) {

        Tfs1098DeploymentBundleReport report;
        if (!validIdentifier(request.serviceName)) {
            report.errors.emplace_back("serviceName must contain only letters, digits, '_' or '-'");
            return report;
        }
        if (!validIdentifier(request.serviceUser)) {
            report.errors.emplace_back("serviceUser must contain only letters, digits, '_' or '-'");
            return report;
        }
        if (!validFilename(request.executableName)) {
            report.errors.emplace_back("executableName must be a single safe filename");
            return report;
        }
        if (!validInstallRoot(request.installRoot)) {
            report.errors.emplace_back("installRoot must be an absolute Unix path without '..'");
            return report;
        }
        if (request.runtimeDirectory.empty() || request.outputDirectory.empty()) {
            report.errors.emplace_back("runtimeDirectory and outputDirectory are required");
            return report;
        }

        std::error_code ec;
        const auto runtime = std::filesystem::absolute(request.runtimeDirectory, ec).lexically_normal();
        if (ec || !std::filesystem::is_directory(runtime, ec) || ec) {
            report.errors.emplace_back("runtimeDirectory is missing or invalid: " + request.runtimeDirectory.string());
            return report;
        }
        const auto output = std::filesystem::absolute(request.outputDirectory, ec).lexically_normal();
        if (ec) {
            report.errors.emplace_back("unable to resolve outputDirectory: " + ec.message());
            return report;
        }
        if (runtime == output || isWithin(output, runtime)) {
            report.errors.emplace_back("deployment output must not be the runtime directory or a child of it");
            return report;
        }

        const auto serverDirectory = output / "server";
        const auto deployDirectory = output / "deploy";
        const auto servicePath = deployDirectory / (request.serviceName + ".service");
        const auto installPath = deployDirectory / "install.sh";
        const auto readmePath = deployDirectory / "README.md";
        const auto manifestPath = deployDirectory / "manifest.json";

        std::filesystem::remove_all(output, ec);
        ec.clear();
        std::filesystem::create_directories(serverDirectory, ec);
        if (ec) {
            report.errors.emplace_back("unable to create deployment output: " + ec.message());
            return report;
        }

        std::string error;
        if (!copyTree(runtime, serverDirectory, error)) {
            report.errors.push_back(std::move(error));
            return report;
        }

        const auto executable = serverDirectory / request.executableName;
        if (!std::filesystem::is_regular_file(executable, ec) || ec) {
            report.errors.emplace_back("runtime executable is missing from deployment bundle: " + executable.string());
            return report;
        }
#ifndef _WIN32
        std::filesystem::permissions(
            executable,
            std::filesystem::perms::owner_exec |
                std::filesystem::perms::group_exec |
                std::filesystem::perms::others_exec,
            std::filesystem::perm_options::add,
            ec);
        if (ec) {
            report.warnings.emplace_back("unable to add executable permission to staged TFS binary: " + ec.message());
            ec.clear();
        }
#endif

        std::filesystem::create_directories(deployDirectory, ec);
        if (ec) {
            report.errors.emplace_back("unable to create deploy directory: " + ec.message());
            return report;
        }

        const std::string service = makeService(request);
        const std::string installer = makeInstaller(request);
        const std::string readme = makeReadme(request);
        const std::string manifest = makeManifest(request);

        if (!writeText(servicePath, service, error) ||
            !writeText(installPath, installer, error) ||
            !writeText(readmePath, readme, error) ||
            !writeText(manifestPath, manifest, error)) {
            report.errors.push_back(std::move(error));
            return report;
        }
#ifndef _WIN32
        std::filesystem::permissions(
            installPath,
            std::filesystem::perms::owner_exec |
                std::filesystem::perms::group_exec |
                std::filesystem::perms::others_exec,
            std::filesystem::perm_options::add,
            ec);
        if (ec) {
            report.warnings.emplace_back("unable to mark install.sh executable: " + ec.message());
        }
#endif

        report.generatedFiles = {serverDirectory, servicePath, installPath, readmePath, manifestPath};
        report.success = true;
        return report;
    }

private:
    static bool validIdentifier(const std::string& value) noexcept {
        if (value.empty()) return false;
        return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
            return std::isalnum(ch) != 0 || ch == '_' || ch == '-';
        });
    }

    static bool validFilename(const std::string& value) noexcept {
        if (value.empty() || value == "." || value == "..") return false;
        return value.find('/') == std::string::npos && value.find('\\') == std::string::npos;
    }

    static bool validInstallRoot(const std::string& value) noexcept {
        if (value.empty() || value.front() != '/') return false;
        const std::filesystem::path path(value);
        for (const auto& part : path) {
            if (part == "..") return false;
        }
        return true;
    }

    static bool isWithin(
        const std::filesystem::path& candidate,
        const std::filesystem::path& parent) noexcept {

        auto candidateIt = candidate.begin();
        auto parentIt = parent.begin();
        for (; parentIt != parent.end(); ++parentIt, ++candidateIt) {
            if (candidateIt == candidate.end() || *candidateIt != *parentIt) return false;
        }
        return true;
    }

    static bool copyTree(
        const std::filesystem::path& source,
        const std::filesystem::path& destination,
        std::string& error) {

        std::error_code ec;
        for (std::filesystem::recursive_directory_iterator it(source, ec), end; it != end; it.increment(ec)) {
            if (ec) {
                error = "unable to enumerate runtime directory: " + ec.message();
                return false;
            }
            const auto relative = std::filesystem::relative(it->path(), source, ec);
            if (ec) {
                error = "unable to resolve runtime entry: " + ec.message();
                return false;
            }
            const auto target = destination / relative;
            if (it->is_directory(ec)) {
                std::filesystem::create_directories(target, ec);
            } else if (it->is_regular_file(ec)) {
                std::filesystem::create_directories(target.parent_path(), ec);
                if (!ec) {
                    std::filesystem::copy_file(
                        it->path(), target,
                        std::filesystem::copy_options::overwrite_existing, ec);
                }
            }
            if (ec) {
                error = "unable to stage runtime entry '" + it->path().string() + "': " + ec.message();
                return false;
            }
        }
        return true;
    }

    static bool writeText(
        const std::filesystem::path& path,
        const std::string& text,
        std::string& error) {

        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            error = "unable to create deployment file: " + path.string();
            return false;
        }
        output << text;
        if (!output) {
            error = "failed while writing deployment file: " + path.string();
            return false;
        }
        return true;
    }

    static std::string makeService(const Tfs1098DeploymentBundleRequest& request) {
        const std::string serverRoot = request.installRoot + "/server";
        return
            "[Unit]\n"
            "Description=Fantasy TFS 1.4.2 / 10.98 runtime\n"
            "Wants=network-online.target\n"
            "After=network-online.target mariadb.service\n\n"
            "[Service]\n"
            "Type=simple\n"
            "User=" + request.serviceUser + "\n"
            "Group=" + request.serviceUser + "\n"
            "WorkingDirectory=" + serverRoot + "\n"
            "ExecStart=" + serverRoot + "/" + request.executableName + "\n"
            "Restart=on-failure\n"
            "RestartSec=5\n"
            "LimitNOFILE=65535\n"
            "StandardOutput=journal\n"
            "StandardError=journal\n\n"
            "[Install]\n"
            "WantedBy=multi-user.target\n";
    }

    static std::string makeInstaller(const Tfs1098DeploymentBundleRequest& request) {
        return
            "#!/usr/bin/env bash\n"
            "set -euo pipefail\n\n"
            "if [ \"${EUID}\" -ne 0 ]; then\n"
            "  echo \"Run this installer as root (sudo).\" >&2\n"
            "  exit 1\n"
            "fi\n\n"
            "SERVICE_NAME='" + request.serviceName + "'\n"
            "SERVICE_USER='" + request.serviceUser + "'\n"
            "INSTALL_ROOT='" + request.installRoot + "'\n"
            "SCRIPT_DIR=\"$(cd \"$(dirname \"${BASH_SOURCE[0]}\")\" && pwd)\"\n"
            "BUNDLE_ROOT=\"$(cd \"${SCRIPT_DIR}/..\" && pwd)\"\n\n"
            "if ! id \"${SERVICE_USER}\" >/dev/null 2>&1; then\n"
            "  useradd --system --home \"${INSTALL_ROOT}\" --shell /usr/sbin/nologin \"${SERVICE_USER}\"\n"
            "fi\n\n"
            "mkdir -p \"${INSTALL_ROOT}\"\n"
            "rm -rf \"${INSTALL_ROOT}/server\"\n"
            "cp -a \"${BUNDLE_ROOT}/server\" \"${INSTALL_ROOT}/server\"\n"
            "chown -R \"${SERVICE_USER}:${SERVICE_USER}\" \"${INSTALL_ROOT}\"\n"
            "install -m 0644 \"${SCRIPT_DIR}/${SERVICE_NAME}.service\" \"/etc/systemd/system/${SERVICE_NAME}.service\"\n"
            "systemctl daemon-reload\n"
            "systemctl enable --now \"${SERVICE_NAME}.service\"\n"
            "systemctl --no-pager --full status \"${SERVICE_NAME}.service\" || true\n";
    }

    static std::string makeReadme(const Tfs1098DeploymentBundleRequest& request) {
        return
            "# Fantasy TFS1098 VPS bundle\n\n"
            "This directory was generated from an already packaged Fantasy TFS1098 runtime.\n\n"
            "## Before installation\n\n"
            "1. Install/configure MariaDB and import the TFS 1.4.2 schema.\n"
            "2. Review `server/config.lua` and set the production database credentials, public IP and ports.\n"
            "3. Keep `config.lua` and database credentials out of source control.\n"
            "4. Open only the required login/game/status ports in the VPS firewall.\n\n"
            "## Install\n\n"
            "```bash\n"
            "cd deploy\n"
            "sudo ./install.sh\n"
            "```\n\n"
            "The service is installed as `" + request.serviceName + ".service` under `" + request.installRoot + "`.\n"
            "Logs are available with `journalctl -u " + request.serviceName + " -f`.\n";
    }

    static std::string makeManifest(const Tfs1098DeploymentBundleRequest& request) {
        return
            "{\n"
            "  \"schemaVersion\": 1,\n"
            "  \"runtime\": \"tfs1098\",\n"
            "  \"serviceName\": \"" + request.serviceName + "\",\n"
            "  \"serviceUser\": \"" + request.serviceUser + "\",\n"
            "  \"installRoot\": \"" + request.installRoot + "\",\n"
            "  \"executable\": \"" + request.executableName + "\"\n"
            "}\n";
    }
};

} // namespace fantasy::studio::runtime
