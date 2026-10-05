#include "Project/ProjectManager.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace fantasy::studio {
namespace {

std::string readText(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Unable to open file: " + path.string());
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void writeText(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Unable to write file: " + path.string());
    }
    output << content;
}

std::string extractString(const std::string& json, const std::string& key) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        throw std::runtime_error("Missing string field in project manifest: " + key);
    }
    return match[1].str();
}

int extractInteger(const std::string& json, const std::string& key) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+)");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        throw std::runtime_error("Missing integer field in project manifest: " + key);
    }
    return std::stoi(match[1].str());
}

bool isSafeRelative(const fs::path& value) {
    if (value.empty() || value.is_absolute() || value.has_root_name() || value.has_root_directory()) {
        return false;
    }
    for (const auto& part : value) {
        if (part == "..") {
            return false;
        }
    }
    return true;
}

fs::path resolveProjectPath(const fs::path& root, const std::string& relativeText) {
    const fs::path relative = fs::path(relativeText).lexically_normal();
    if (!isSafeRelative(relative)) {
        throw std::runtime_error("Project path must be safe and relative: " + relativeText);
    }
    return fs::weakly_canonical(root / relative);
}

std::string escapeJson(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += ch; break;
        }
    }
    return result;
}

std::string slugify(const std::string& name) {
    std::string slug;
    bool previousDash = false;
    for (const unsigned char raw : name) {
        const char ch = static_cast<char>(raw);
        if (std::isalnum(raw)) {
            slug += static_cast<char>(std::tolower(raw));
            previousDash = false;
        } else if (!previousDash && !slug.empty()) {
            slug += '-';
            previousDash = true;
        }
    }
    while (!slug.empty() && slug.back() == '-') {
        slug.pop_back();
    }
    return slug.empty() ? "fantasy-project" : slug;
}

void requireExists(const fs::path& path, const std::string& label) {
    if (!fs::exists(path)) {
        throw std::runtime_error(label + " does not exist: " + path.string());
    }
}

fs::path manifestPathFromInput(const fs::path& projectOrManifest) {
    if (fs::is_directory(projectOrManifest)) {
        return projectOrManifest / "fantasy.project.json";
    }
    return projectOrManifest;
}

} // namespace

ProjectInfo ProjectManager::openProject(const fs::path& projectOrManifest) {
    const fs::path manifestPath = fs::absolute(manifestPathFromInput(projectOrManifest)).lexically_normal();
    requireExists(manifestPath, "Project manifest");

    const std::string json = readText(manifestPath);
    if (extractInteger(json, "schemaVersion") != 2) {
        throw std::runtime_error("Unsupported Fantasy Project schemaVersion; expected 2");
    }

    const fs::path root = fs::weakly_canonical(manifestPath.parent_path());
    ProjectInfo project;
    project.root = root;
    project.manifestPath = fs::weakly_canonical(manifestPath);
    project.name = extractString(json, "name");
    if (project.name.empty()) {
        throw std::runtime_error("Project name cannot be empty");
    }

    project.mainMapPath = resolveProjectPath(root, extractString(json, "mainMap"));
    project.protocolSpecPath = resolveProjectPath(root, extractString(json, "protocolSpec"));
    project.mapSchemaPath = resolveProjectPath(root, extractString(json, "mapSchema"));

    requireExists(project.mainMapPath, "Main map");
    requireExists(project.protocolSpecPath, "Protocol specification");
    requireExists(project.mapSchemaPath, "FMAP schema");

    return project;
}

ProjectInfo ProjectManager::createProject(
    const fs::path& projectRoot,
    const std::string& name,
    const fs::path& templateRoot) {

    if (name.empty()) {
        throw std::runtime_error("Project name cannot be empty");
    }

    const fs::path root = fs::absolute(projectRoot).lexically_normal();
    if (fs::exists(root) && !fs::is_empty(root)) {
        throw std::runtime_error("New Project target must be empty: " + root.string());
    }

    const fs::path templateCanonical = fs::weakly_canonical(templateRoot);
    const fs::path sourceProtocol = templateCanonical / "Shared/Protocol/protocol-v1.yaml";
    const fs::path sourceMapSchema = templateCanonical / "Shared/Formats/FMAP/schema-v0.json";
    requireExists(sourceProtocol, "Template protocol specification");
    requireExists(sourceMapSchema, "Template FMAP schema");

    const std::vector<fs::path> directories = {
        "Studio", "Game/Maps/World", "Game/Content", "Game/Assets", "Game/Scripts", "Game/Config",
        "Server", "Client", "Shared/Protocol", "Shared/Formats/FMAP", "Database", "Tools", "Projects"
    };
    for (const auto& relative : directories) {
        fs::create_directories(root / relative);
    }

    fs::copy_file(sourceProtocol, root / "Shared/Protocol/protocol-v1.yaml", fs::copy_options::overwrite_existing);
    fs::copy_file(sourceMapSchema, root / "Shared/Formats/FMAP/schema-v0.json", fs::copy_options::overwrite_existing);

    const std::string escapedName = escapeJson(name);
    const std::string worldId = slugify(name);
    writeText(root / "Game/Maps/World/world.fmap.json",
        "{\n"
        "  \"format\": \"FMAP\",\n"
        "  \"version\": 0,\n"
        "  \"world\": {\"id\": \"" + worldId + "\", \"name\": \"" + escapedName + "\", \"tileSize\": 32},\n"
        "  \"developmentSpawn\": {\"x\": 100, \"y\": 100, \"z\": 7},\n"
        "  \"regions\": []\n"
        "}\n");

    writeText(root / "fantasy.project.json",
        "{\n"
        "  \"schemaVersion\": 2,\n"
        "  \"name\": \"" + escapedName + "\",\n"
        "  \"protocol\": \"fantasy-v1\",\n"
        "  \"paths\": {\n"
        "    \"studio\": \"Studio\",\n"
        "    \"game\": \"Game\",\n"
        "    \"mainMap\": \"Game/Maps/World/world.fmap.json\",\n"
        "    \"content\": \"Game/Content\",\n"
        "    \"assets\": \"Game/Assets\",\n"
        "    \"scripts\": \"Game/Scripts\",\n"
        "    \"config\": \"Game/Config\",\n"
        "    \"server\": \"Server\",\n"
        "    \"client\": \"Client\",\n"
        "    \"shared\": \"Shared\",\n"
        "    \"protocolSpec\": \"Shared/Protocol/protocol-v1.yaml\",\n"
        "    \"mapSchema\": \"Shared/Formats/FMAP/schema-v0.json\",\n"
        "    \"database\": \"Database\",\n"
        "    \"tools\": \"Tools\",\n"
        "    \"projects\": \"Projects\"\n"
        "  },\n"
        "  \"runtime\": {\n"
        "    \"server\": \"FantasyServer\",\n"
        "    \"mapFormat\": \"FMAP-v0\",\n"
        "    \"protocol\": \"FantasyProtocol-v1\",\n"
        "    \"authoritativeServer\": true\n"
        "  },\n"
        "  \"references\": {\n"
        "    \"tfs\": \"1.4.2 / 10.98\",\n"
        "    \"rme\": \"3.7 / OTBM-v3\",\n"
        "    \"otclient\": \"10.98-compatible\",\n"
        "    \"role\": \"reference-only\"\n"
        "  }\n"
        "}\n");

    return openProject(root);
}

fs::path ProjectManager::openMainMap(const ProjectInfo& project) {
    requireExists(project.mainMapPath, "Main map");
    return project.mainMapPath;
}

void ProjectManager::rememberProject(const fs::path& workspaceRoot, const fs::path& projectRoot) {
    const fs::path workspace = fs::weakly_canonical(workspaceRoot);
    const fs::path project = fs::weakly_canonical(projectRoot);
    const fs::path relative = fs::relative(project, workspace).lexically_normal();
    if (!isSafeRelative(relative)) {
        throw std::runtime_error("Recent Projects only stores paths relative to the workspace root");
    }

    const fs::path registry = workspace / "Projects/recent-projects.txt";
    fs::create_directories(registry.parent_path());

    std::vector<std::string> entries;
    if (fs::exists(registry)) {
        std::ifstream input(registry);
        std::string line;
        while (std::getline(input, line)) {
            if (!line.empty() && line != relative.generic_string()) {
                entries.push_back(line);
            }
        }
    }

    entries.insert(entries.begin(), relative.generic_string());
    if (entries.size() > 10) {
        entries.resize(10);
    }

    std::ofstream output(registry, std::ios::trunc);
    for (const auto& entry : entries) {
        output << entry << '\n';
    }
}

std::vector<fs::path> ProjectManager::recentProjects(const fs::path& workspaceRoot) {
    const fs::path workspace = fs::weakly_canonical(workspaceRoot);
    const fs::path registry = workspace / "Projects/recent-projects.txt";
    std::vector<fs::path> projects;
    if (!fs::exists(registry)) {
        return projects;
    }

    std::ifstream input(registry);
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const fs::path relative = fs::path(line).lexically_normal();
        if (!isSafeRelative(relative)) {
            continue;
        }
        const fs::path resolved = (workspace / relative).lexically_normal();
        if (fs::exists(resolved / "fantasy.project.json")) {
            projects.push_back(resolved);
        }
    }
    return projects;
}

} // namespace fantasy::studio
