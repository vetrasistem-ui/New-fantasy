#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace fantasy::studio {

struct ProjectInfo {
    std::filesystem::path root;
    std::filesystem::path manifestPath;
    std::filesystem::path mainMapPath;
    std::filesystem::path protocolSpecPath;
    std::filesystem::path mapSchemaPath;
    std::string name;
};

class ProjectManager {
public:
    static ProjectInfo openProject(const std::filesystem::path& projectOrManifest);
    static ProjectInfo createProject(
        const std::filesystem::path& projectRoot,
        const std::string& name,
        const std::filesystem::path& templateRoot);

    static std::filesystem::path openMainMap(const ProjectInfo& project);

    static void rememberProject(
        const std::filesystem::path& workspaceRoot,
        const std::filesystem::path& projectRoot);

    static std::vector<std::filesystem::path> recentProjects(
        const std::filesystem::path& workspaceRoot);
};

} // namespace fantasy::studio
