#pragma once

#include "Tfs1098OtcExtendedAdapter.hpp"
#include "Tfs1098RuntimeBackend.hpp"
#include "Tfs1098RuntimeProfile.hpp"
#include "Tfs1098SystemChannelRegistry.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace fantasy::studio::runtime {

struct Tfs1098RuntimePreparationRequest {
    std::filesystem::path projectRoot;
    std::filesystem::path runtimeDirectory;
    std::filesystem::path clientDirectory;
    std::vector<SystemChannelId> requiredChannels;
};

struct Tfs1098RuntimePreparationReport {
    bool success = false;
    Tfs1098CompatibilityProfile profile = Tfs1098CompatibilityProfile::Vanilla;
    RuntimeCapabilities capabilities;
    std::vector<RuntimeChannelBinding> bindings;
    std::vector<std::filesystem::path> generatedFiles;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

class Tfs1098RuntimePreparation {
public:
    static void saveCompatibilityProfile(
        const std::filesystem::path& projectRoot,
        Tfs1098CompatibilityProfile profile) {

        auto config = Tfs1098RuntimeProfile::load(projectRoot);
        config.compatibilityProfile = profile;
        Tfs1098RuntimeProfile::save(projectRoot, config);
    }

    [[nodiscard]] static Tfs1098RuntimePreparationReport prepare(
        const Tfs1098RuntimePreparationRequest& request) {

        Tfs1098RuntimePreparationReport report;
        try {
            if (request.projectRoot.empty()) {
                throw std::runtime_error("projectRoot is required for TFS1098 runtime preparation");
            }
            if (!std::filesystem::is_directory(request.runtimeDirectory)) {
                throw std::runtime_error(
                    "TFS1098 runtime directory is missing or invalid: " + request.runtimeDirectory.string());
            }

            const auto config = Tfs1098RuntimeProfile::load(request.projectRoot);
            report.profile = config.compatibilityProfile;

            Tfs1098RuntimeBackend backend;
            report.capabilities = backend.capabilities();

            if (config.compatibilityProfile == Tfs1098CompatibilityProfile::Vanilla) {
                report.bindings.clear();
                report.success = true;
                if (!request.requiredChannels.empty()) {
                    report.warnings.emplace_back(
                        "Vanilla TFS1098 profile ignores semantic runtime channels; select otc_extended to activate them");
                }
                return report;
            }

            if (!std::filesystem::is_directory(request.clientDirectory)) {
                throw std::runtime_error(
                    "OTC extended profile requires a valid client package directory: " +
                    request.clientDirectory.string());
            }

            Tfs1098OtcExtendedAdapterRequest adapterRequest;
            adapterRequest.projectRoot = request.projectRoot;
            adapterRequest.runtimeDirectory = request.runtimeDirectory;
            adapterRequest.clientDirectory = request.clientDirectory;
            adapterRequest.requiredChannels = request.requiredChannels;

            const auto adapterReport = Tfs1098OtcExtendedAdapter::apply(adapterRequest);
            if (!adapterReport.success) {
                report.errors = adapterReport.errors;
                if (report.errors.empty()) {
                    report.errors.emplace_back("TFS1098 OTC extended adapter failed without error detail");
                }
                return report;
            }

            report.capabilities = Tfs1098OtcExtendedAdapter::extendCapabilities(report.capabilities);
            report.bindings = adapterReport.bindings;
            report.generatedFiles = adapterReport.generatedFiles;
            report.warnings = adapterReport.warnings;
            report.success = true;
        } catch (const std::exception& error) {
            report.errors.emplace_back(error.what());
        }
        return report;
    }
};

} // namespace fantasy::studio::runtime
