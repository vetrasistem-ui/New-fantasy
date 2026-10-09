#pragma once

#include "Foundation/FantasyPersistence.hpp"
#include "Foundation/FantasyProjectLayoutV2.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace fantasy::studio::foundation {

class FantasyAuthoringRepository {
public:
    explicit FantasyAuthoringRepository(std::filesystem::path projectRoot)
        : root_(std::move(projectRoot)) {
        if (root_.empty()) throw std::invalid_argument("Fantasy authoring repository requires a project root");
        FantasyProjectLayoutV2::ensureAuthoringDirectories(root_);
    }

    [[nodiscard]] const std::filesystem::path& root() const noexcept { return root_; }

    void save(const ZoneDefinition& value) const { FantasyPersistence::saveZone(FantasyProjectLayoutV2::zonePath(root_, value.id), value); }
    void save(const AppearanceDefinition& value) const { FantasyPersistence::saveAppearance(FantasyProjectLayoutV2::appearancePath(root_, value.id), value); }
    void save(const EntityArchetype& value) const { FantasyPersistence::saveEntity(FantasyProjectLayoutV2::entityPath(root_, value.id), value); }
    void save(const ItemDefinition& value) const { FantasyPersistence::saveItem(FantasyProjectLayoutV2::itemPath(root_, value.id), value); }
    void save(const CreatureDefinition& value) const { FantasyPersistence::saveCreature(FantasyProjectLayoutV2::creaturePath(root_, value.id), value); }
    void save(const ClassDefinition& value) const { FantasyPersistence::saveClass(FantasyProjectLayoutV2::classPath(root_, value.id), value); }
    void save(const SystemDefinition& value) const { FantasyPersistence::saveSystem(FantasyProjectLayoutV2::systemPath(root_, value.id), value); }
    void save(const AssetProfile& value) const { FantasyPersistence::saveAssetProfile(FantasyProjectLayoutV2::assetProfilePath(root_, value.id), value); }
    void saveModernAsset(const ModernAssetDefinition& value) const { FantasyPersistence::saveModernAsset(FantasyProjectLayoutV2::modernAssetPath(root_, value.id), value); }
    void saveBrush(const BrushDefinition& value) const { FantasyPersistence::saveBrush(FantasyProjectLayoutV2::brushPath(root_, value.id), value); }
    void saveMigration(const std::string& migrationId, const AssetMigrationPlan& value) const {
        FantasyPersistence::saveMigration(FantasyProjectLayoutV2::migrationPath(root_, migrationId), value);
    }

    [[nodiscard]] ZoneDefinition loadZone(const std::string& id) const { return FantasyPersistence::loadZone(FantasyProjectLayoutV2::zonePath(root_, id)); }
    [[nodiscard]] AppearanceDefinition loadAppearance(const std::string& id) const { return FantasyPersistence::loadAppearance(FantasyProjectLayoutV2::appearancePath(root_, id)); }
    [[nodiscard]] EntityArchetype loadEntity(const std::string& id) const { return FantasyPersistence::loadEntity(FantasyProjectLayoutV2::entityPath(root_, id)); }
    [[nodiscard]] ItemDefinition loadItem(const std::string& id) const { return FantasyPersistence::loadItem(FantasyProjectLayoutV2::itemPath(root_, id)); }
    [[nodiscard]] CreatureDefinition loadCreature(const std::string& id) const { return FantasyPersistence::loadCreature(FantasyProjectLayoutV2::creaturePath(root_, id)); }
    [[nodiscard]] ClassDefinition loadClass(const std::string& id) const { return FantasyPersistence::loadClass(FantasyProjectLayoutV2::classPath(root_, id)); }
    [[nodiscard]] SystemDefinition loadSystem(const std::string& id) const { return FantasyPersistence::loadSystem(FantasyProjectLayoutV2::systemPath(root_, id)); }
    [[nodiscard]] AssetProfile loadAssetProfile(const std::string& id) const { return FantasyPersistence::loadAssetProfile(FantasyProjectLayoutV2::assetProfilePath(root_, id)); }
    [[nodiscard]] ModernAssetDefinition loadModernAsset(const std::string& id) const { return FantasyPersistence::loadModernAsset(FantasyProjectLayoutV2::modernAssetPath(root_, id)); }
    [[nodiscard]] BrushDefinition loadBrush(const std::string& id) const { return FantasyPersistence::loadBrush(FantasyProjectLayoutV2::brushPath(root_, id)); }
    [[nodiscard]] AssetMigrationPlan loadMigration(const std::string& id) const { return FantasyPersistence::loadMigration(FantasyProjectLayoutV2::migrationPath(root_, id)); }

    [[nodiscard]] std::vector<std::string> zones() const { return listIds(FantasyProjectLayoutV2::zonesDirectory(root_), ".zone.json"); }
    [[nodiscard]] std::vector<std::string> appearances() const { return listIds(FantasyProjectLayoutV2::appearancesDirectory(root_), ".appearance.json"); }
    [[nodiscard]] std::vector<std::string> entities() const { return listIds(FantasyProjectLayoutV2::entitiesDirectory(root_), ".entity.json"); }
    [[nodiscard]] std::vector<std::string> items() const { return listIds(FantasyProjectLayoutV2::itemsDirectory(root_), ".item.json"); }
    [[nodiscard]] std::vector<std::string> creatures() const { return listIds(FantasyProjectLayoutV2::creaturesDirectory(root_), ".creature.json"); }
    [[nodiscard]] std::vector<std::string> classes() const { return listIds(FantasyProjectLayoutV2::classesDirectory(root_), ".class.json"); }
    [[nodiscard]] std::vector<std::string> systems() const { return listIds(FantasyProjectLayoutV2::systemsDirectory(root_), ".system.json"); }
    [[nodiscard]] std::vector<std::string> brushes() const { return listIds(FantasyProjectLayoutV2::brushesDirectory(root_), ".brush.json"); }
    [[nodiscard]] std::vector<std::string> assetProfiles() const { return listIds(FantasyProjectLayoutV2::assetProfilesDirectory(root_), ".asset-profile.json"); }
    [[nodiscard]] std::vector<std::string> modernAssets() const { return listIds(FantasyProjectLayoutV2::modernAssetsDirectory(root_), ".asset.json"); }
    [[nodiscard]] std::vector<std::string> migrations() const { return listIds(FantasyProjectLayoutV2::migrationsDirectory(root_), ".asset-migration.json"); }

    bool removeZone(const std::string& id) const { return remove(FantasyProjectLayoutV2::zonePath(root_, id)); }
    bool removeAppearance(const std::string& id) const { return remove(FantasyProjectLayoutV2::appearancePath(root_, id)); }
    bool removeEntity(const std::string& id) const { return remove(FantasyProjectLayoutV2::entityPath(root_, id)); }
    bool removeItem(const std::string& id) const { return remove(FantasyProjectLayoutV2::itemPath(root_, id)); }
    bool removeCreature(const std::string& id) const { return remove(FantasyProjectLayoutV2::creaturePath(root_, id)); }
    bool removeClass(const std::string& id) const { return remove(FantasyProjectLayoutV2::classPath(root_, id)); }
    bool removeSystem(const std::string& id) const { return remove(FantasyProjectLayoutV2::systemPath(root_, id)); }
    bool removeBrush(const std::string& id) const { return remove(FantasyProjectLayoutV2::brushPath(root_, id)); }
    bool removeModernAsset(const std::string& id) const { return remove(FantasyProjectLayoutV2::modernAssetPath(root_, id)); }

private:
    [[nodiscard]] static std::vector<std::string> listIds(
        const std::filesystem::path& directory,
        const std::string& suffix) {

        std::vector<std::string> result;
        if (!std::filesystem::is_directory(directory)) return result;
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (!entry.is_regular_file()) continue;
            const auto filename = entry.path().filename().string();
            if (filename.size() <= suffix.size() || !filename.ends_with(suffix)) continue;
            const auto id = filename.substr(0, filename.size() - suffix.size());
            if (validIdentifier(id)) result.push_back(id);
        }
        std::sort(result.begin(), result.end());
        return result;
    }

    [[nodiscard]] static bool remove(const std::filesystem::path& path) {
        std::error_code error;
        const bool removed = std::filesystem::remove(path, error);
        if (error) throw std::runtime_error("unable to remove Fantasy authoring file: " + path.string() + ": " + error.message());
        return removed;
    }

    std::filesystem::path root_;
};

} // namespace fantasy::studio::foundation
