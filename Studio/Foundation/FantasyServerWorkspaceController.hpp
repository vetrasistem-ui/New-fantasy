#pragma once

#include "Foundation/FantasyAuthoringRepository.hpp"
#include "Foundation/FantasyProjectHealthScanner.hpp"
#include "Foundation/FantasySystemCatalog.hpp"
#include "Runtime/Tfs1098RuntimePreparation.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace fantasy::studio::foundation {

struct FantasyServerWorkspaceState {
    fantasy::studio::runtime::Tfs1098TargetConfig profile;
    std::filesystem::path runtimeTemplateDirectory;
    std::filesystem::path runtimeDirectory;
    std::filesystem::path clientPackageDirectory;
    std::vector<std::string> requiredChannels;
    fantasy::studio::runtime::RuntimeCapabilities capabilities;
    ProjectHealthReport health;
    std::string status;
};

class FantasyServerWorkspaceController {
public:
    FantasyServerWorkspaceController(
        fantasy::studio::ProjectInfo project,
        std::filesystem::path runtimeTemplateDirectory,
        std::filesystem::path clientPackageDirectory)
        : project_(std::move(project)),
          runtimeTemplateDirectory_(std::move(runtimeTemplateDirectory)),
          clientPackageDirectory_(std::move(clientPackageDirectory)) {
        refresh();
    }

    [[nodiscard]] const FantasyServerWorkspaceState& state() const noexcept {
        return state_;
    }

    void setRuntimeTemplateDirectory(std::filesystem::path value) {
        runtimeTemplateDirectory_ = std::move(value);
        refresh();
    }

    void setClientPackageDirectory(std::filesystem::path value) {
        clientPackageDirectory_ = std::move(value);
        refresh();
    }

    void setCompatibilityProfile(
        fantasy::studio::runtime::Tfs1098CompatibilityProfile profile) {

        fantasy::studio::runtime::Tfs1098RuntimePreparation::saveCompatibilityProfile(
            project_.root,
            profile);
        refresh();
    }

    [[nodiscard]] fantasy::studio::runtime::Tfs1098RuntimePreparationReport prepareRuntime() {
        refresh();
        fantasy::studio::runtime::Tfs1098RuntimePreparationRequest request;
        request.projectRoot = project_.root;
        request.runtimeDirectory = state_.runtimeDirectory;
        request.clientDirectory = clientPackageDirectory_;
        for (const auto& channel : state_.requiredChannels) {
            request.requiredChannels.push_back(
                fantasy::studio::runtime::SystemChannelId::parse(channel));
        }

        auto report = fantasy::studio::runtime::Tfs1098RuntimePreparation::prepare(request);
        if (report.success) {
            state_.capabilities = report.capabilities;
            state_.status =
                report.profile == fantasy::studio::runtime::Tfs1098CompatibilityProfile::OtcExtended
                    ? "OTC Extended runtime prepared"
                    : "Vanilla runtime ready";
        } else {
            state_.status = report.errors.empty()
                ? "Runtime preparation failed"
                : report.errors.front();
        }
        return report;
    }

    void refresh() {
        state_.profile = fantasy::studio::runtime::Tfs1098RuntimeProfile::load(project_.root);
        state_.runtimeTemplateDirectory = runtimeTemplateDirectory_;
        state_.clientPackageDirectory = clientPackageDirectory_;
        state_.runtimeDirectory =
            fantasy::studio::runtime::Tfs1098RuntimeProfile::resolveOutputDirectory(
                project_.root,
                state_.profile);
        state_.requiredChannels = loadRequiredChannels();

        fantasy::studio::runtime::Tfs1098RuntimeBackend backend;
        state_.capabilities = backend.capabilities();
        if (state_.profile.compatibilityProfile ==
            fantasy::studio::runtime::Tfs1098CompatibilityProfile::OtcExtended) {
            state_.capabilities =
                fantasy::studio::runtime::Tfs1098OtcExtendedAdapter::extendCapabilities(
                    state_.capabilities);
        }

        state_.health = FantasyProjectHealthScanner::scan({
            project_,
            runtimeTemplateDirectory_,
            clientPackageDirectory_,
        });
        state_.status = state_.health.ready()
            ? "Server workspace ready"
            : "Server workspace has blocking health issues";
    }

private:
    [[nodiscard]] std::vector<std::string> loadRequiredChannels() const {
        FantasyAuthoringRepository repository(project_.root);
        FantasySystemCatalog catalog;
        for (const auto& id : repository.systems()) {
            catalog.add(repository.loadSystem(id));
        }
        catalog.validateDependencies();
        const auto channels = catalog.requiredChannels();
        return {channels.begin(), channels.end()};
    }

    fantasy::studio::ProjectInfo project_;
    std::filesystem::path runtimeTemplateDirectory_;
    std::filesystem::path clientPackageDirectory_;
    FantasyServerWorkspaceState state_;
};

} // namespace fantasy::studio::foundation
