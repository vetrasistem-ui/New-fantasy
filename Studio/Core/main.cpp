#include "Project/ProjectManager.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using fantasy::studio::ProjectManager;

namespace {

void printUsage() {
    std::cout
        << "Fantasy Studio Project CLI\n"
        << "  --open-project <project-root-or-manifest>\n"
        << "  --open-main-map <project-root-or-manifest>\n"
        << "  --new-project <target-root> <name> <template-root>\n"
        << "  --remember-project <workspace-root> <project-root>\n"
        << "  --recent-projects <workspace-root>\n";
}

int run(int argc, char** argv) {
    if (argc < 2) {
        printUsage();
        return 0;
    }

    const std::string command = argv[1];

    if (command == "--open-project") {
        if (argc != 3) {
            throw std::runtime_error("--open-project expects exactly one path");
        }
        const auto project = ProjectManager::openProject(argv[2]);
        std::cout << "PROJECT_OPEN PASS\n";
        std::cout << "name=" << project.name << '\n';
        std::cout << "root=" << project.root.string() << '\n';
        std::cout << "mainMap=" << project.mainMapPath.string() << '\n';
        return 0;
    }

    if (command == "--open-main-map") {
        if (argc != 3) {
            throw std::runtime_error("--open-main-map expects exactly one project path");
        }
        const auto project = ProjectManager::openProject(argv[2]);
        const auto map = ProjectManager::openMainMap(project);
        std::cout << "OPEN_MAIN_MAP PASS\n";
        std::cout << map.string() << '\n';
        return 0;
    }

    if (command == "--new-project") {
        if (argc != 5) {
            throw std::runtime_error("--new-project expects <target-root> <name> <template-root>");
        }
        const auto project = ProjectManager::createProject(argv[2], argv[3], argv[4]);
        std::cout << "NEW_PROJECT PASS\n";
        std::cout << "name=" << project.name << '\n';
        std::cout << "root=" << project.root.string() << '\n';
        return 0;
    }

    if (command == "--remember-project") {
        if (argc != 4) {
            throw std::runtime_error("--remember-project expects <workspace-root> <project-root>");
        }
        ProjectManager::rememberProject(argv[2], argv[3]);
        std::cout << "REMEMBER_PROJECT PASS\n";
        return 0;
    }

    if (command == "--recent-projects") {
        if (argc != 3) {
            throw std::runtime_error("--recent-projects expects <workspace-root>");
        }
        const auto recent = ProjectManager::recentProjects(argv[2]);
        std::cout << "RECENT_PROJECTS count=" << recent.size() << '\n';
        for (const auto& path : recent) {
            std::cout << path.string() << '\n';
        }
        return 0;
    }

    throw std::runtime_error("Unknown command: " + command);
}

} // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Fantasy Studio error: " << error.what() << '\n';
        return 1;
    }
}
