#pragma once

#include "Foundation/FantasyFoundationV2.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fantasy::studio::foundation {

struct FantasyWindowsInstallerRequest {
    std::filesystem::path packageDirectory;
    std::filesystem::path outputDirectory;
    std::string productId = "fantasy-studio";
    std::string displayName = "Fantasy Studio";
    std::string installFolder = "FantasyStudio";
    std::string executableName = "FantasyStudio.exe";
    bool createDesktopShortcut = true;
};

struct FantasyWindowsInstallerReport {
    bool success = false;
    std::vector<std::filesystem::path> generatedFiles;
    std::vector<std::string> errors;
};

class FantasyWindowsInstallerBundle {
public:
    [[nodiscard]] static FantasyWindowsInstallerReport write(
        const FantasyWindowsInstallerRequest& request) {

        FantasyWindowsInstallerReport report;
        try {
            validate(request);
            std::filesystem::create_directories(request.outputDirectory);

            const auto install = request.outputDirectory / "install.ps1";
            const auto uninstall = request.outputDirectory / "uninstall.ps1";
            const auto manifest = request.outputDirectory / "installer-manifest.json";

            writeText(install, installScript(request));
            writeText(uninstall, uninstallScript(request));
            writeText(manifest, manifestJson(request));
            report.generatedFiles = {install, uninstall, manifest};
            report.success = true;
        } catch (const std::exception& error) {
            report.errors.emplace_back(error.what());
        }
        return report;
    }

private:
    static void validate(const FantasyWindowsInstallerRequest& request) {
        if (!std::filesystem::is_directory(request.packageDirectory)) {
            throw std::runtime_error("Windows installer packageDirectory is missing or invalid");
        }
        if (request.outputDirectory.empty()) {
            throw std::runtime_error("Windows installer outputDirectory is required");
        }
        requireIdentifier(request.productId, "Windows installer product id");
        if (request.displayName.empty()) throw std::runtime_error("Windows installer displayName is required");
        if (!safeFileToken(request.installFolder)) throw std::runtime_error("unsafe Windows installer installFolder");
        if (!safeFileToken(request.executableName)) throw std::runtime_error("unsafe Windows installer executableName");
        if (!std::filesystem::is_regular_file(request.packageDirectory / request.executableName)) {
            throw std::runtime_error("Windows installer package does not contain the requested executable");
        }
        const auto package = std::filesystem::weakly_canonical(request.packageDirectory);
        const auto output = std::filesystem::weakly_canonical(request.outputDirectory.parent_path()) /
            request.outputDirectory.filename();
        if (output == package || isChild(output, package)) {
            throw std::runtime_error("Windows installer outputDirectory cannot be the package directory or its child");
        }
    }

    [[nodiscard]] static bool safeFileToken(const std::string& value) noexcept {
        if (value.empty() || value == "." || value == "..") return false;
        return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
            return (ch >= '0' && ch <= '9') ||
                   (ch >= 'A' && ch <= 'Z') ||
                   (ch >= 'a' && ch <= 'z') ||
                   ch == '_' || ch == '-' || ch == '.';
        });
    }

    [[nodiscard]] static bool isChild(
        const std::filesystem::path& candidate,
        const std::filesystem::path& parent) noexcept {

        auto candidateIt = candidate.begin();
        auto parentIt = parent.begin();
        for (; parentIt != parent.end(); ++parentIt, ++candidateIt) {
            if (candidateIt == candidate.end() || *candidateIt != *parentIt) return false;
        }
        return candidateIt != candidate.end();
    }

    [[nodiscard]] static std::string escapePowerShellSingleQuoted(const std::string& value) {
        std::string result;
        result.reserve(value.size());
        for (const char ch : value) {
            if (ch == '\'') result += "''";
            else result += ch;
        }
        return result;
    }

    [[nodiscard]] static std::string installScript(const FantasyWindowsInstallerRequest& request) {
        const auto display = escapePowerShellSingleQuoted(request.displayName);
        const auto folder = escapePowerShellSingleQuoted(request.installFolder);
        const auto executable = escapePowerShellSingleQuoted(request.executableName);
        std::ostringstream out;
        out << "$ErrorActionPreference = 'Stop'\n"
            << "$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path\n"
            << "$PackageDir = Join-Path $ScriptDir 'package'\n"
            << "if (-not (Test-Path $PackageDir -PathType Container)) { throw 'Installer package directory is missing.' }\n"
            << "$InstallRoot = Join-Path $env:LOCALAPPDATA 'Fantasy'\n"
            << "$InstallDir = Join-Path $InstallRoot '" << folder << "'\n"
            << "New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null\n"
            << "Copy-Item -Path (Join-Path $PackageDir '*') -Destination $InstallDir -Recurse -Force\n"
            << "$Exe = Join-Path $InstallDir '" << executable << "'\n"
            << "if (-not (Test-Path $Exe -PathType Leaf)) { throw 'Installed executable is missing.' }\n";
        if (request.createDesktopShortcut) {
            out << "$Desktop = [Environment]::GetFolderPath('Desktop')\n"
                << "$ShortcutPath = Join-Path $Desktop '" << display << ".lnk'\n"
                << "$Shell = New-Object -ComObject WScript.Shell\n"
                << "$Shortcut = $Shell.CreateShortcut($ShortcutPath)\n"
                << "$Shortcut.TargetPath = $Exe\n"
                << "$Shortcut.WorkingDirectory = $InstallDir\n"
                << "$Shortcut.Save()\n";
        }
        out << "Write-Host 'FANTASY_WINDOWS_INSTALL PASS'\n";
        return out.str();
    }

    [[nodiscard]] static std::string uninstallScript(const FantasyWindowsInstallerRequest& request) {
        const auto display = escapePowerShellSingleQuoted(request.displayName);
        const auto folder = escapePowerShellSingleQuoted(request.installFolder);
        std::ostringstream out;
        out << "$ErrorActionPreference = 'Stop'\n"
            << "$InstallDir = Join-Path (Join-Path $env:LOCALAPPDATA 'Fantasy') '" << folder << "'\n"
            << "if (Test-Path $InstallDir) { Remove-Item -Recurse -Force $InstallDir }\n";
        if (request.createDesktopShortcut) {
            out << "$ShortcutPath = Join-Path ([Environment]::GetFolderPath('Desktop')) '" << display << ".lnk'\n"
                << "if (Test-Path $ShortcutPath) { Remove-Item -Force $ShortcutPath }\n";
        }
        out << "Write-Host 'FANTASY_WINDOWS_UNINSTALL PASS'\n";
        return out.str();
    }

    [[nodiscard]] static std::string manifestJson(const FantasyWindowsInstallerRequest& request) {
        std::ostringstream out;
        out << "{\n"
            << "  \"schemaVersion\": 1,\n"
            << "  \"productId\": \"" << request.productId << "\",\n"
            << "  \"displayName\": \"" << request.displayName << "\",\n"
            << "  \"installScope\": \"per-user\",\n"
            << "  \"installRoot\": \"%LOCALAPPDATA%/Fantasy/" << request.installFolder << "\",\n"
            << "  \"executable\": \"" << request.executableName << "\",\n"
            << "  \"desktopShortcut\": " << (request.createDesktopShortcut ? "true" : "false") << "\n"
            << "}\n";
        return out.str();
    }

    static void writeText(const std::filesystem::path& path, const std::string& text) {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("unable to write Windows installer file: " + path.string());
        output << text;
        if (!output) throw std::runtime_error("failed while writing Windows installer file: " + path.string());
    }
};

} // namespace fantasy::studio::foundation
