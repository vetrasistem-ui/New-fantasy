#include "Project/ProjectManager.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#ifndef FANTASY_REPO_ROOT
#error FANTASY_REPO_ROOT must be defined for ProjectManagerTests
#endif

namespace fs = std::filesystem;
using fantasy::studio::ProjectManager;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

} // namespace

int main() {
    try {
        const fs::path templateRoot = fs::weakly_canonical(fs::path(FANTASY_REPO_ROOT));
        const fs::path workspace = fs::temp_directory_path() / "fantasy-project-manager-tests";
        const fs::path projectRoot = workspace / "Projects" / "Alpha";
        const fs::path movedRoot = workspace / "Projects" / "AlphaMoved";

        std::error_code ec;
        fs::remove_all(workspace, ec);
        fs::create_directories(workspace / "Projects");

        const auto created = ProjectManager::createProject(projectRoot, "Alpha World", templateRoot);
        require(created.name == "Alpha World", "created project name mismatch");
        require(fs::exists(created.mainMapPath), "created main map missing");
        require(fs::exists(created.protocolSpecPath), "created protocol spec missing");
        require(fs::exists(created.mapSchemaPath), "created map schema missing");

        const auto opened = ProjectManager::openProject(projectRoot);
        require(opened.root == fs::weakly_canonical(projectRoot), "open project root mismatch");
        require(ProjectManager::openMainMap(opened) == opened.mainMapPath, "open main map mismatch");

        ProjectManager::rememberProject(workspace, projectRoot);
        const auto recent = ProjectManager::recentProjects(workspace);
        require(recent.size() == 1, "recent projects should contain one entry");
        require(fs::weakly_canonical(recent.front()) == fs::weakly_canonical(projectRoot), "recent project path mismatch");

        fs::copy(projectRoot, movedRoot, fs::copy_options::recursive);
        fs::remove_all(projectRoot);
        const auto moved = ProjectManager::openProject(movedRoot);
        require(moved.name == "Alpha World", "moved project failed to reopen");
        require(fs::exists(ProjectManager::openMainMap(moved)), "moved project main map missing");

        fs::remove_all(workspace, ec);
        std::cout << "ProjectManagerTests PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ProjectManagerTests FAIL: " << error.what() << '\n';
        return 1;
    }
}
