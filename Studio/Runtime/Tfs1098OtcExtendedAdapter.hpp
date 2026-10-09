#pragma once

#include "RuntimeBackend.hpp"
#include "Tfs1098OtcExtendedBridge.hpp"
#include "Tfs1098RuntimeProfile.hpp"
#include "Tfs1098SystemChannelRegistry.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fantasy::studio::runtime {

struct Tfs1098OtcExtendedAdapterRequest {
    std::filesystem::path projectRoot;
    std::filesystem::path runtimeDirectory;
    std::filesystem::path clientDirectory;
    std::vector<SystemChannelId> requiredChannels;
};

struct Tfs1098OtcExtendedAdapterReport {
    bool success = false;
    std::vector<RuntimeChannelBinding> bindings;
    std::vector<std::filesystem::path> generatedFiles;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

class Tfs1098OtcExtendedAdapter {
public:
    [[nodiscard]] static RuntimeCapabilities extendCapabilities(RuntimeCapabilities base) noexcept {
        base.canUseSystemChannels = true;
        return base;
    }

    [[nodiscard]] static Tfs1098OtcExtendedAdapterReport apply(
        const Tfs1098OtcExtendedAdapterRequest& request) {

        Tfs1098OtcExtendedAdapterReport report;
        try {
            validateRequest(request);

            const Tfs1098TargetConfig profile = Tfs1098RuntimeProfile::load(request.projectRoot);
            if (profile.compatibilityProfile != Tfs1098CompatibilityProfile::OtcExtended) {
                throw std::runtime_error(
                    "TFS1098 OTC extended adapter requires compatibilityProfile 'otc_extended'");
            }

            auto bindings = Tfs1098SystemChannelRegistry::load(request.projectRoot);
            for (const auto& channel : request.requiredChannels) {
                (void)Tfs1098SystemChannelRegistry::bind(bindings, channel);
            }
            Tfs1098SystemChannelRegistry::save(request.projectRoot, bindings);
            report.generatedFiles.push_back(Tfs1098SystemChannelRegistry::pathForProject(request.projectRoot));

            ensureCreatureScriptRegistration(request.runtimeDirectory, report.generatedFiles);
            ensureLoginRegistration(request.runtimeDirectory, report.generatedFiles);

            const auto bridgeReport = Tfs1098OtcExtendedBridge::write(
                request.runtimeDirectory,
                request.clientDirectory,
                bindings);
            if (!bridgeReport.success) {
                throw std::runtime_error(
                    bridgeReport.errors.empty()
                        ? "TFS1098 OTC extended bridge generation failed"
                        : bridgeReport.errors.front());
            }
            report.generatedFiles.insert(
                report.generatedFiles.end(),
                bridgeReport.generatedFiles.begin(),
                bridgeReport.generatedFiles.end());

            if (bindings.empty()) {
                report.warnings.emplace_back(
                    "OTC extended bridge generated without semantic channels; bind channels before using system messages");
            }

            report.bindings = std::move(bindings);
            report.success = true;
        } catch (const std::exception& error) {
            report.errors.emplace_back(error.what());
        }
        return report;
    }

private:
    static void validateRequest(const Tfs1098OtcExtendedAdapterRequest& request) {
        if (request.projectRoot.empty()) {
            throw std::runtime_error("projectRoot is required for TFS1098 OTC extended adapter");
        }
        if (!std::filesystem::is_directory(request.runtimeDirectory)) {
            throw std::runtime_error(
                "TFS1098 OTC extended runtime directory is missing or invalid: " +
                request.runtimeDirectory.string());
        }
        if (!std::filesystem::is_directory(request.clientDirectory)) {
            throw std::runtime_error(
                "TFS1098 OTC extended client directory is missing or invalid: " +
                request.clientDirectory.string());
        }
    }

    [[nodiscard]] static std::string readText(const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            throw std::runtime_error("Unable to read TFS1098 OTC extended template file: " + path.string());
        }
        std::ostringstream buffer;
        buffer << input.rdbuf();
        return buffer.str();
    }

    static void writeText(const std::filesystem::path& path, const std::string& text) {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error("Unable to update TFS1098 OTC extended template file: " + path.string());
        }
        output << text;
        if (!output) {
            throw std::runtime_error("Failed while updating TFS1098 OTC extended template file: " + path.string());
        }
    }

    static void ensureCreatureScriptRegistration(
        const std::filesystem::path& runtimeDirectory,
        std::vector<std::filesystem::path>& generatedFiles) {

        const auto path = runtimeDirectory / "data" / "creaturescripts" / "creaturescripts.xml";
        std::string xml = readText(path);
        if (xml.find("name=\"ExtendedOpcode\"") != std::string::npos ||
            xml.find("name='ExtendedOpcode'") != std::string::npos) {
            return;
        }

        const std::string closing = "</creaturescripts>";
        const auto insertion = xml.rfind(closing);
        if (insertion == std::string::npos) {
            throw std::runtime_error(
                "Unable to register ExtendedOpcode: creaturescripts.xml has no </creaturescripts> root");
        }

        const std::string event =
            "\t<event type=\"extendedopcode\" name=\"ExtendedOpcode\" script=\"extendedopcode.lua\" />\n";
        xml.insert(insertion, event);
        writeText(path, xml);
        generatedFiles.push_back(path);
    }

    static void ensureLoginRegistration(
        const std::filesystem::path& runtimeDirectory,
        std::vector<std::filesystem::path>& generatedFiles) {

        const auto path = runtimeDirectory / "data" / "creaturescripts" / "scripts" / "login.lua";
        std::string lua = readText(path);
        if (lua.find("registerEvent(\"ExtendedOpcode\")") != std::string::npos ||
            lua.find("registerEvent('ExtendedOpcode')") != std::string::npos) {
            return;
        }

        const std::string returnTrue = "return true";
        const auto insertion = lua.rfind(returnTrue);
        if (insertion == std::string::npos) {
            throw std::runtime_error(
                "Unable to register ExtendedOpcode: login.lua has no final 'return true'");
        }

        lua.insert(insertion, "player:registerEvent(\"ExtendedOpcode\")\n\t");
        writeText(path, lua);
        generatedFiles.push_back(path);
    }
};

} // namespace fantasy::studio::runtime
