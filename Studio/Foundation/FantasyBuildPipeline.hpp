#pragma once

#include "Foundation/FantasyAuthoringRepository.hpp"
#include "Foundation/FantasyBuildManifest.hpp"
#include "Foundation/FantasyClientManifest.hpp"
#include "Foundation/FantasyProjectHealthScanner.hpp"
#include "Foundation/FantasySystemCatalog.hpp"
#include "Runtime/Tfs1098RuntimePreparation.hpp"
#include "Runtime/Tfs1098VpsLifecycle.hpp"

#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace fantasy::studio::foundation {

struct FantasyBuildPipelineRequest {
    fantasy::studio::ProjectInfo project;
    std::filesystem::path runtimeTemplateDirectory;
    std::filesystem::path runtimeDirectory;
    std::filesystem::path clientPackageDirectory;
    std::filesystem::path vpsBundleDirectory;
    std::string projectId;
    std::string buildVersion = "v1";
    std::string assetProfileId;
    std::string serverHost = "127.0.0.1";
    std::uint16_t loginPort = 7171;
    std::uint16_t gamePort = 7172;
    BuildPlan plan;
};

struct FantasyBuildPipelineReport {
    bool success = false;
    ProjectHealthReport health;
    std::vector<std::string> orderedSystems;
    std::vector<std::string> requiredChannels;
    fantasy::studio::runtime::Tfs1098RuntimePreparationReport runtimePreparation;
    std::vector<std::filesystem::path> generatedFiles;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

class FantasyBuildPipeline {
public:
    [[nodiscard]] static FantasyBuildPipelineReport prepare(const FantasyBuildPipelineRequest& request) {
        FantasyBuildPipelineReport report;
        try {
            request.plan.validate();
            requireIdentifier(request.projectId, "build project id");
            requireIdentifier(request.buildVersion, "build version");
            requireIdentifier(request.assetProfileId, "build asset profile id");

            report.health = FantasyProjectHealthScanner::scan({
                request.project,
                request.runtimeTemplateDirectory,
                request.clientPackageDirectory,
            });
            if (!report.health.ready()) {
                report.errors.emplace_back("Fantasy project health check failed; build preparation was not started");
                return report;
            }

            FantasyAuthoringRepository repository(request.project.root);
            FantasySystemCatalog catalog;
            for (const auto& id : repository.systems()) catalog.add(repository.loadSystem(id));
            catalog.validateDependencies();
            report.orderedSystems = catalog.executionOrder();

            const auto channelSet = catalog.requiredChannels();
            report.requiredChannels.assign(channelSet.begin(), channelSet.end());

            if (request.plan.buildRuntime) {
                fantasy::studio::runtime::Tfs1098RuntimePreparationRequest runtimeRequest;
                runtimeRequest.projectRoot = request.project.root;
                runtimeRequest.runtimeDirectory = request.runtimeDirectory;
                runtimeRequest.clientDirectory = request.clientPackageDirectory;
                for (const auto& channel : report.requiredChannels) {
                    runtimeRequest.requiredChannels.push_back(
                        fantasy::studio::runtime::SystemChannelId::parse(channel));
                }

                report.runtimePreparation =
                    fantasy::studio::runtime::Tfs1098RuntimePreparation::prepare(runtimeRequest);
                if (!report.runtimePreparation.success) {
                    report.errors = report.runtimePreparation.errors;
                    if (report.errors.empty()) report.errors.emplace_back("runtime preparation failed without error detail");
                    return report;
                }
                report.warnings.insert(
                    report.warnings.end(),
                    report.runtimePreparation.warnings.begin(),
                    report.runtimePreparation.warnings.end());
            }

            const auto runtimeProfile =
                fantasy::studio::runtime::Tfs1098RuntimeProfile::load(request.project.root);
            const std::string compatibilityProfile =
                fantasy::studio::runtime::tfs1098CompatibilityProfileId(runtimeProfile.compatibilityProfile);

            FantasyBuildManifest buildManifest;
            buildManifest.projectId = request.projectId;
            buildManifest.buildVersion = request.buildVersion;
            buildManifest.runtimeId = "tfs1098";
            buildManifest.compatibilityProfile = compatibilityProfile;
            buildManifest.assetProfileId = request.assetProfileId;
            if (request.plan.buildRuntime) {
                buildManifest.artifacts.push_back({
                    "runtime",
                    relativeToProject(request.project.root, request.runtimeDirectory),
                    "",
                });
            }
            if (request.plan.buildClient) {
                buildManifest.artifacts.push_back({
                    "client",
                    relativeToProject(request.project.root, request.clientPackageDirectory),
                    "",
                });
            }
            if (request.plan.buildVps) {
                buildManifest.artifacts.push_back({
                    "vps",
                    relativeToProject(request.project.root, request.vpsBundleDirectory),
                    "",
                });
            }
            if (buildManifest.artifacts.empty()) {
                // A validation-only plan still produces an auditable manifest.
                buildManifest.artifacts.push_back({"validation", std::filesystem::path{"build"} / "validation", ""});
            }

            const auto manifestPath = request.project.root / "build" / "fantasy-build-manifest.json";
            FantasyBuildManifestWriter::write(manifestPath, buildManifest);
            report.generatedFiles.push_back(manifestPath);

            if (request.plan.buildClient) {
                FantasyClientManifest clientManifest;
                clientManifest.clientId = "fantasy-client";
                clientManifest.version = request.buildVersion;
                clientManifest.runtimeId = "tfs1098";
                clientManifest.compatibilityProfile = compatibilityProfile;
                clientManifest.assetProfileId = request.assetProfileId;
                clientManifest.serverHost = request.serverHost;
                clientManifest.loginPort = request.loginPort;
                clientManifest.gamePort = request.gamePort;
                clientManifest.updateChannel = "stable";
                if (runtimeProfile.compatibilityProfile ==
                    fantasy::studio::runtime::Tfs1098CompatibilityProfile::OtcExtended) {
                    clientManifest.modules.push_back({"fantasy_runtime_bridge", true, "v1"});
                }
                clientManifest.artifacts.push_back({
                    relativeToProject(request.project.root, request.clientPackageDirectory),
                    "",
                });
                const auto clientManifestPath =
                    request.project.root / "build" / "fantasy-client-manifest.json";
                writeClientManifest(clientManifestPath, clientManifest);
                report.generatedFiles.push_back(clientManifestPath);
            }

            if (request.plan.buildVps) {
                fantasy::studio::runtime::Tfs1098VpsLifecycleRequest lifecycle;
                lifecycle.bundleDirectory = request.vpsBundleDirectory;
                const auto lifecycleReport = fantasy::studio::runtime::Tfs1098VpsLifecycle::write(lifecycle);
                if (!lifecycleReport.success) {
                    report.errors = lifecycleReport.errors;
                    return report;
                }
                report.generatedFiles.insert(
                    report.generatedFiles.end(),
                    lifecycleReport.generatedFiles.begin(),
                    lifecycleReport.generatedFiles.end());
            }

            report.success = true;
        } catch (const std::exception& error) {
            report.errors.emplace_back(error.what());
        }
        return report;
    }

private:
    [[nodiscard]] static std::filesystem::path relativeToProject(
        const std::filesystem::path& projectRoot,
        const std::filesystem::path& path) {

        if (path.empty()) throw std::invalid_argument("build artifact path is required");
        std::error_code error;
        auto relative = std::filesystem::relative(path, projectRoot, error);
        if (error || !safeRelativePath(relative)) {
            throw std::invalid_argument(
                "build artifact must be inside the Fantasy project root: " + path.string());
        }
        return relative.lexically_normal();
    }

    static void writeClientManifest(
        const std::filesystem::path& path,
        const FantasyClientManifest& manifest) {

        const auto parent = path.parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("unable to write Fantasy client manifest: " + path.string());
        output << manifest.toJson();
        if (!output) throw std::runtime_error("failed while writing Fantasy client manifest: " + path.string());
    }
};

} // namespace fantasy::studio::foundation
