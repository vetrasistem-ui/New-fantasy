#pragma once

#include "Foundation/FantasyFoundationV2.hpp"

#include <filesystem>
#include <string>

namespace fantasy::studio::foundation {

class FantasyProjectLayoutV2 {
public:
    [[nodiscard]] static std::filesystem::path zonesDirectory(const std::filesystem::path& root) {
        return root / "Game" / "Zones";
    }

    [[nodiscard]] static std::filesystem::path appearancesDirectory(const std::filesystem::path& root) {
        return root / "Game" / "Appearances";
    }

    [[nodiscard]] static std::filesystem::path entitiesDirectory(const std::filesystem::path& root) {
        return root / "Game" / "Entities";
    }

    [[nodiscard]] static std::filesystem::path itemsDirectory(const std::filesystem::path& root) {
        return root / "Game" / "Items";
    }

    [[nodiscard]] static std::filesystem::path creaturesDirectory(const std::filesystem::path& root) {
        return root / "Game" / "Creatures";
    }

    [[nodiscard]] static std::filesystem::path classesDirectory(const std::filesystem::path& root) {
        return root / "Game" / "Classes";
    }

    [[nodiscard]] static std::filesystem::path systemsDirectory(const std::filesystem::path& root) {
        return root / "Game" / "Systems";
    }

    [[nodiscard]] static std::filesystem::path brushesDirectory(const std::filesystem::path& root) {
        return root / "Game" / "Brushes";
    }

    [[nodiscard]] static std::filesystem::path assetProfilesDirectory(const std::filesystem::path& root) {
        return root / "Assets" / "Profiles";
    }

    [[nodiscard]] static std::filesystem::path modernAssetsDirectory(const std::filesystem::path& root) {
        return root / "Assets" / "Modern";
    }

    [[nodiscard]] static std::filesystem::path migrationsDirectory(const std::filesystem::path& root) {
        return root / "Assets" / "Migrations";
    }

    [[nodiscard]] static std::filesystem::path assetCatalogDirectory(const std::filesystem::path& root) {
        return root / "Assets" / "Catalog";
    }

    [[nodiscard]] static std::filesystem::path assetCatalogPath(const std::filesystem::path& root) {
        return assetCatalogDirectory(root) / "asset-catalog.json";
    }

    [[nodiscard]] static std::filesystem::path buildDirectory(const std::filesystem::path& root) {
        return root / "build";
    }

    [[nodiscard]] static std::filesystem::path zonePath(const std::filesystem::path& root, const std::string& id) {
        return typedPath(zonesDirectory(root), id, ".zone.json", "zone");
    }

    [[nodiscard]] static std::filesystem::path appearancePath(const std::filesystem::path& root, const std::string& id) {
        return typedPath(appearancesDirectory(root), id, ".appearance.json", "appearance");
    }

    [[nodiscard]] static std::filesystem::path entityPath(const std::filesystem::path& root, const std::string& id) {
        return typedPath(entitiesDirectory(root), id, ".entity.json", "entity");
    }

    [[nodiscard]] static std::filesystem::path itemPath(const std::filesystem::path& root, const std::string& id) {
        return typedPath(itemsDirectory(root), id, ".item.json", "item");
    }

    [[nodiscard]] static std::filesystem::path creaturePath(const std::filesystem::path& root, const std::string& id) {
        return typedPath(creaturesDirectory(root), id, ".creature.json", "creature");
    }

    [[nodiscard]] static std::filesystem::path classPath(const std::filesystem::path& root, const std::string& id) {
        return typedPath(classesDirectory(root), id, ".class.json", "class");
    }

    [[nodiscard]] static std::filesystem::path systemPath(const std::filesystem::path& root, const std::string& id) {
        return typedPath(systemsDirectory(root), id, ".system.json", "system");
    }

    [[nodiscard]] static std::filesystem::path brushPath(const std::filesystem::path& root, const std::string& id) {
        return typedPath(brushesDirectory(root), id, ".brush.json", "brush");
    }

    [[nodiscard]] static std::filesystem::path assetProfilePath(const std::filesystem::path& root, const std::string& id) {
        return typedPath(assetProfilesDirectory(root), id, ".asset-profile.json", "asset profile");
    }

    [[nodiscard]] static std::filesystem::path modernAssetPath(const std::filesystem::path& root, const std::string& id) {
        return typedPath(modernAssetsDirectory(root), id, ".asset.json", "modern asset");
    }

    [[nodiscard]] static std::filesystem::path migrationPath(const std::filesystem::path& root, const std::string& id) {
        return typedPath(migrationsDirectory(root), id, ".asset-migration.json", "asset migration");
    }

    static void ensureAuthoringDirectories(const std::filesystem::path& root) {
        if (root.empty()) throw std::invalid_argument("Fantasy project root is required");
        std::filesystem::create_directories(zonesDirectory(root));
        std::filesystem::create_directories(appearancesDirectory(root));
        std::filesystem::create_directories(entitiesDirectory(root));
        std::filesystem::create_directories(itemsDirectory(root));
        std::filesystem::create_directories(creaturesDirectory(root));
        std::filesystem::create_directories(classesDirectory(root));
        std::filesystem::create_directories(systemsDirectory(root));
        std::filesystem::create_directories(brushesDirectory(root));
        std::filesystem::create_directories(assetProfilesDirectory(root));
        std::filesystem::create_directories(modernAssetsDirectory(root));
        std::filesystem::create_directories(migrationsDirectory(root));
        std::filesystem::create_directories(assetCatalogDirectory(root));
        std::filesystem::create_directories(buildDirectory(root));
    }

private:
    [[nodiscard]] static std::filesystem::path typedPath(
        const std::filesystem::path& directory,
        const std::string& id,
        const char* suffix,
        const char* label) {

        requireIdentifier(id, label);
        return directory / (id + suffix);
    }
};

} // namespace fantasy::studio::foundation
