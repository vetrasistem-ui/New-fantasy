#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fantasy::studio::runtime {

struct Tfs1098VpsLifecycleRequest {
    std::filesystem::path bundleDirectory;
    std::string serviceName = "fantasy-tfs1098";
    std::string serviceUser = "fantasy";
    std::filesystem::path installRoot = "/opt/fantasy/server";
    std::filesystem::path backupRoot = "/opt/fantasy/backups/server";
};

struct Tfs1098VpsLifecycleReport {
    bool success = false;
    std::vector<std::filesystem::path> generatedFiles;
    std::vector<std::string> errors;
};

class Tfs1098VpsLifecycle {
public:
    [[nodiscard]] static Tfs1098VpsLifecycleReport write(const Tfs1098VpsLifecycleRequest& request) {
        Tfs1098VpsLifecycleReport report;
        try {
            validate(request);
            const auto deploy = request.bundleDirectory / "deploy";
            const auto server = request.bundleDirectory / "server";
            if (!std::filesystem::is_directory(server)) {
                throw std::runtime_error("VPS lifecycle requires an existing bundle server/ directory");
            }
            std::filesystem::create_directories(deploy);

            const auto envExample = deploy / "env.example";
            const auto health = deploy / "healthcheck.sh";
            const auto update = deploy / "update.sh";
            const auto rollback = deploy / "rollback.sh";
            const auto policy = deploy / "release-policy.json";

            writeText(envExample,
                "# Example only. Do not commit production secrets.\n"
                "DB_HOST=127.0.0.1\n"
                "DB_PORT=3306\n"
                "DB_NAME=fantasy\n"
                "DB_USER=fantasy\n"
                "DB_PASSWORD=CHANGE_ME\n");

            writeText(health, healthScript(request));
            writeText(update, updateScript(request));
            writeText(rollback, rollbackScript(request));
            writeText(policy, policyJson(request));

#ifndef _WIN32
            addExecutableBits(health);
            addExecutableBits(update);
            addExecutableBits(rollback);
#endif

            report.generatedFiles = {envExample, health, update, rollback, policy};
            report.success = true;
        } catch (const std::exception& error) {
            report.errors.emplace_back(error.what());
        }
        return report;
    }

private:
    static void validate(const Tfs1098VpsLifecycleRequest& request) {
        if (request.bundleDirectory.empty()) throw std::runtime_error("bundleDirectory is required");
        if (!safeToken(request.serviceName)) throw std::runtime_error("invalid VPS lifecycle serviceName");
        if (!safeToken(request.serviceUser)) throw std::runtime_error("invalid VPS lifecycle serviceUser");
        if (!safeLinuxAbsolutePath(request.installRoot)) throw std::runtime_error("invalid VPS lifecycle installRoot");
        if (!safeLinuxAbsolutePath(request.backupRoot)) throw std::runtime_error("invalid VPS lifecycle backupRoot");
        if (request.installRoot.generic_string() == request.backupRoot.generic_string()) {
            throw std::runtime_error("VPS lifecycle installRoot and backupRoot must differ");
        }
    }

    [[nodiscard]] static bool safeToken(const std::string& value) noexcept {
        if (value.empty()) return false;
        return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
            return (ch >= '0' && ch <= '9') ||
                   (ch >= 'A' && ch <= 'Z') ||
                   (ch >= 'a' && ch <= 'z') || ch == '_' || ch == '-' || ch == '.';
        });
    }

    [[nodiscard]] static bool safeLinuxAbsolutePath(const std::filesystem::path& path) noexcept {
        const auto text = path.generic_string();
        if (text.size() <= 1U || text.front() != '/' || text.find("..") != std::string::npos) return false;
        return std::all_of(text.begin(), text.end(), [](unsigned char ch) {
            return (ch >= '0' && ch <= '9') ||
                   (ch >= 'A' && ch <= 'Z') ||
                   (ch >= 'a' && ch <= 'z') ||
                   ch == '/' || ch == '_' || ch == '-' || ch == '.';
        });
    }

    [[nodiscard]] static std::string healthScript(const Tfs1098VpsLifecycleRequest& request) {
        std::ostringstream out;
        out << "#!/usr/bin/env bash\n"
            << "set -euo pipefail\n"
            << "systemctl is-active --quiet " << request.serviceName << "\n"
            << "test -d '" << request.installRoot.generic_string() << "/server'\n"
            << "echo 'FANTASY_VPS_HEALTH PASS'\n";
        return out.str();
    }

    [[nodiscard]] static std::string updateScript(const Tfs1098VpsLifecycleRequest& request) {
        std::ostringstream out;
        out << "#!/usr/bin/env bash\n"
            << "set -euo pipefail\n"
            << "if [[ ${EUID} -ne 0 ]]; then echo 'Run as root'; exit 1; fi\n"
            << "SCRIPT_DIR=\"$(cd \"$(dirname \"${BASH_SOURCE[0]}\")\" && pwd)\"\n"
            << "BUNDLE_ROOT=\"$(cd \"${SCRIPT_DIR}/..\" && pwd)\"\n"
            << "INSTALL_ROOT='" << request.installRoot.generic_string() << "'\n"
            << "BACKUP_ROOT='" << request.backupRoot.generic_string() << "'\n"
            << "STAMP=\"$(date -u +%Y%m%dT%H%M%SZ)\"\n"
            << "BACKUP=\"${BACKUP_ROOT}/${STAMP}\"\n"
            << "mkdir -p \"${BACKUP}\"\n"
            << "if [[ -d \"${INSTALL_ROOT}/server\" ]]; then cp -a \"${INSTALL_ROOT}/server/.\" \"${BACKUP}/\" || true; fi\n"
            << "systemctl stop " << request.serviceName << " || true\n"
            << "rm -rf \"${INSTALL_ROOT}/server\"\n"
            << "mkdir -p \"${INSTALL_ROOT}/server\"\n"
            << "cp -a \"${BUNDLE_ROOT}/server/.\" \"${INSTALL_ROOT}/server/\"\n"
            << "chown -R " << request.serviceUser << ':' << request.serviceUser << " \"${INSTALL_ROOT}/server\"\n"
            << "systemctl start " << request.serviceName << "\n"
            << "if ! \"${SCRIPT_DIR}/healthcheck.sh\"; then\n"
            << "  echo 'Health check failed; rolling back'\n"
            << "  systemctl stop " << request.serviceName << " || true\n"
            << "  rm -rf \"${INSTALL_ROOT}/server\"\n"
            << "  mkdir -p \"${INSTALL_ROOT}/server\"\n"
            << "  cp -a \"${BACKUP}/.\" \"${INSTALL_ROOT}/server/\"\n"
            << "  chown -R " << request.serviceUser << ':' << request.serviceUser << " \"${INSTALL_ROOT}/server\"\n"
            << "  systemctl start " << request.serviceName << "\n"
            << "  exit 2\n"
            << "fi\n"
            << "echo \"FANTASY_VPS_UPDATE PASS backup=${BACKUP}\"\n";
        return out.str();
    }

    [[nodiscard]] static std::string rollbackScript(const Tfs1098VpsLifecycleRequest& request) {
        std::ostringstream out;
        out << "#!/usr/bin/env bash\n"
            << "set -euo pipefail\n"
            << "if [[ ${EUID} -ne 0 ]]; then echo 'Run as root'; exit 1; fi\n"
            << "INSTALL_ROOT='" << request.installRoot.generic_string() << "'\n"
            << "BACKUP_ROOT='" << request.backupRoot.generic_string() << "'\n"
            << "BACKUP=\"${1:-$(find \"${BACKUP_ROOT}\" -mindepth 1 -maxdepth 1 -type d | sort | tail -n 1)}\"\n"
            << "if [[ -z \"${BACKUP}\" || ! -d \"${BACKUP}\" ]]; then echo 'No backup found'; exit 1; fi\n"
            << "systemctl stop " << request.serviceName << " || true\n"
            << "rm -rf \"${INSTALL_ROOT}/server\"\n"
            << "mkdir -p \"${INSTALL_ROOT}/server\"\n"
            << "cp -a \"${BACKUP}/.\" \"${INSTALL_ROOT}/server/\"\n"
            << "chown -R " << request.serviceUser << ':' << request.serviceUser << " \"${INSTALL_ROOT}/server\"\n"
            << "systemctl start " << request.serviceName << "\n"
            << "echo \"FANTASY_VPS_ROLLBACK PASS backup=${BACKUP}\"\n";
        return out.str();
    }

    [[nodiscard]] static std::string policyJson(const Tfs1098VpsLifecycleRequest& request) {
        std::ostringstream out;
        out << "{\n"
            << "  \"schemaVersion\": 1,\n"
            << "  \"serviceName\": \"" << request.serviceName << "\",\n"
            << "  \"serviceUser\": \"" << request.serviceUser << "\",\n"
            << "  \"installRoot\": \"" << request.installRoot.generic_string() << "\",\n"
            << "  \"backupRoot\": \"" << request.backupRoot.generic_string() << "\",\n"
            << "  \"backupBeforeUpdate\": true,\n"
            << "  \"rollbackOnFailedHealthCheck\": true\n"
            << "}\n";
        return out.str();
    }

    static void writeText(const std::filesystem::path& path, const std::string& text) {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("unable to write VPS lifecycle file: " + path.string());
        output << text;
        if (!output) throw std::runtime_error("failed while writing VPS lifecycle file: " + path.string());
    }

#ifndef _WIN32
    static void addExecutableBits(const std::filesystem::path& path) {
        namespace fs = std::filesystem;
        fs::permissions(
            path,
            fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
            fs::perm_options::add);
    }
#endif
};

} // namespace fantasy::studio::runtime
