#pragma once

#include "Foundation/FantasyAuthoringRepository.hpp"
#include "Foundation/FantasyFoundationV2.hpp"
#include "Foundation/FantasySystemCatalog.hpp"
#include "Project/ProjectManager.hpp"
#include "Runtime/Tfs1098RuntimeProfile.hpp"

#include <algorithm>
#include <filesystem>
#include <string>

namespace fantasy::studio::foundation {

struct FantasyProjectHealthScanRequest {
    fantasy::studio::ProjectInfo project;
    std::filesystem::path runtimeTemplateDirectory;
    std::filesystem::path clientPackageDirectory;
};

class FantasyProjectHealthScanner {
public:
    [[nodiscard]] static ProjectHealthReport scan(const FantasyProjectHealthScanRequest& request) {
        ProjectHealthReport report;
        const auto add = [&](HealthSeverity severity, std::string code, std::string message) {
            report.issues.push_back({severity, std::move(code), std::move(message)});
        };

        if (request.project.root.empty() || !std::filesystem::is_directory(request.project.root)) {
            add(HealthSeverity::Error, "project.root.missing", "Fantasy project root is missing");
            return report;
        }

        if (!std::filesystem::is_regular_file(request.project.manifestPath)) {
            add(HealthSeverity::Error, "project.manifest.missing", "Fantasy project manifest is missing");
        }
        if (!std::filesystem::is_regular_file(request.project.mainMapPath)) {
            add(HealthSeverity::Error, "map.missing", "Main Fantasy map is missing");
        }

        try {
            FantasyAuthoringRepository repository(request.project.root);
            validateRepository(repository, add);
        } catch (const std::exception& error) {
            add(HealthSeverity::Error, "authoring.invalid", error.what());
        }

        const auto runtimeProfilePath =
            fantasy::studio::runtime::Tfs1098RuntimeProfile::pathForProject(request.project.root);
        if (!std::filesystem::is_regular_file(runtimeProfilePath)) {
            add(HealthSeverity::Error, "runtime.profile.missing", "TFS1098 runtime profile is missing");
            return report;
        }

        fantasy::studio::runtime::Tfs1098TargetConfig runtimeProfile;
        try {
            runtimeProfile = fantasy::studio::runtime::Tfs1098RuntimeProfile::load(request.project.root);
        } catch (const std::exception& error) {
            add(HealthSeverity::Error, "runtime.profile.invalid", error.what());
            return report;
        }

        if (!std::filesystem::is_directory(request.runtimeTemplateDirectory)) {
            add(HealthSeverity::Error, "runtime.template.missing", "External TFS runtime template directory is not configured or missing");
        }

        if (runtimeProfile.compatibilityProfile ==
                fantasy::studio::runtime::Tfs1098CompatibilityProfile::OtcExtended &&
            !std::filesystem::is_directory(request.clientPackageDirectory)) {
            add(HealthSeverity::Error, "client.package.missing", "OTC extended profile requires a valid client package directory");
        }

        if (runtimeProfile.compatibilityProfile ==
                fantasy::studio::runtime::Tfs1098CompatibilityProfile::Vanilla &&
            std::filesystem::is_directory(request.clientPackageDirectory)) {
            add(HealthSeverity::Info, "client.package.unused", "Client package is configured but vanilla runtime does not require the Fantasy Extended bridge");
        }

        if (report.ready()) {
            add(HealthSeverity::Info, "project.ready", "Fantasy project passed headless health validation");
        }
        return report;
    }

private:
    template <typename AddIssue>
    static void validateRepository(const FantasyAuthoringRepository& repository, AddIssue&& add) {
        const auto assetProfileIds = repository.assetProfiles();
        if (assetProfileIds.empty()) {
            add(HealthSeverity::Error, "assets.profile.missing", "At least one Fantasy asset profile is required");
        }

        validateCollection("zone", repository.zones(), [&](const std::string& id) { (void)repository.loadZone(id); }, add);
        validateCollection("appearance", repository.appearances(), [&](const std::string& id) { (void)repository.loadAppearance(id); }, add);
        validateCollection("entity", repository.entities(), [&](const std::string& id) { (void)repository.loadEntity(id); }, add);
        validateCollection("item", repository.items(), [&](const std::string& id) { (void)repository.loadItem(id); }, add);
        validateCollection("creature", repository.creatures(), [&](const std::string& id) { (void)repository.loadCreature(id); }, add);
        validateCollection("class", repository.classes(), [&](const std::string& id) { (void)repository.loadClass(id); }, add);
        validateCollection("brush", repository.brushes(), [&](const std::string& id) { (void)repository.loadBrush(id); }, add);
        validateCollection("modern_asset", repository.modernAssets(), [&](const std::string& id) { (void)repository.loadModernAsset(id); }, add);
        validateCollection("asset_profile", assetProfileIds, [&](const std::string& id) { (void)repository.loadAssetProfile(id); }, add);
        validateCollection("migration", repository.migrations(), [&](const std::string& id) { (void)repository.loadMigration(id); }, add);

        if (repository.hasAssetCatalog()) {
            try {
                const auto catalog = repository.loadAssetCatalog();
                if (std::find(assetProfileIds.begin(), assetProfileIds.end(), catalog.profileId) == assetProfileIds.end()) {
                    add(
                        HealthSeverity::Error,
                        "asset_catalog.profile.missing",
                        "Semantic asset catalog references missing asset profile: " + catalog.profileId);
                }

                const auto brushIds = repository.brushes();
                for (const auto& family : catalog.families) {
                    if (!family.brushRef.has_value()) continue;
                    if (std::find(brushIds.begin(), brushIds.end(), *family.brushRef) == brushIds.end()) {
                        add(
                            HealthSeverity::Error,
                            "asset_catalog.brush.missing",
                            family.id + ": missing brush " + *family.brushRef);
                    }
                }
            } catch (const std::exception& error) {
                add(HealthSeverity::Error, "asset_catalog.invalid", error.what());
            }
        }

        FantasySystemCatalog systems;
        for (const auto& id : repository.systems()) {
            try {
                systems.add(repository.loadSystem(id));
            } catch (const std::exception& error) {
                add(HealthSeverity::Error, "system.invalid", id + ": " + error.what());
            }
        }
        try {
            systems.validateDependencies();
        } catch (const std::exception& error) {
            add(HealthSeverity::Error, "system.dependencies.invalid", error.what());
        }
    }

    template <typename Loader, typename AddIssue>
    static void validateCollection(
        const char* kind,
        const std::vector<std::string>& ids,
        Loader&& loader,
        AddIssue&& add) {

        for (const auto& id : ids) {
            try {
                loader(id);
            } catch (const std::exception& error) {
                add(
                    HealthSeverity::Error,
                    std::string(kind) + ".invalid",
                    id + ": " + error.what());
            }
        }
    }
};

} // namespace fantasy::studio::foundation