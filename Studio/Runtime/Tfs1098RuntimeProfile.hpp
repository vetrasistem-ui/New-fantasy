#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

namespace fantasy::studio::runtime {

enum class Tfs1098CompatibilityProfile {
    Vanilla,
    OtcExtended,
};

[[nodiscard]] constexpr const char* tfs1098CompatibilityProfileId(
    Tfs1098CompatibilityProfile profile) noexcept {
    switch (profile) {
        case Tfs1098CompatibilityProfile::Vanilla: return "vanilla";
        case Tfs1098CompatibilityProfile::OtcExtended: return "otc_extended";
    }
    return "vanilla";
}

[[nodiscard]] inline Tfs1098CompatibilityProfile parseTfs1098CompatibilityProfile(
    const std::string& value) {
    if (value == "vanilla") return Tfs1098CompatibilityProfile::Vanilla;
    if (value == "otc_extended") return Tfs1098CompatibilityProfile::OtcExtended;
    throw std::runtime_error("Unsupported TFS1098 compatibilityProfile: " + value);
}

struct Tfs1098TargetConfig {
    static constexpr std::uint32_t SchemaVersion = 2;

    std::string mapName = "fantasy";
    std::filesystem::path outputDirectory = std::filesystem::path{"build"} / "runtime" / "tfs1098";
    Tfs1098CompatibilityProfile compatibilityProfile = Tfs1098CompatibilityProfile::Vanilla;
};

class Tfs1098RuntimeProfile {
public:
    [[nodiscard]] static std::filesystem::path pathForProject(
        const std::filesystem::path& projectRoot) {
        return projectRoot / "Game" / "Config" / "tfs1098.runtime.json";
    }

    [[nodiscard]] static Tfs1098TargetConfig load(
        const std::filesystem::path& projectRoot) {

        const auto path = pathForProject(projectRoot);
        if (!std::filesystem::exists(path)) return Tfs1098TargetConfig{};

        std::ifstream input(path, std::ios::binary);
        if (!input) throw std::runtime_error("Unable to open TFS1098 runtime profile: " + path.string());
        std::ostringstream buffer;
        buffer << input.rdbuf();
        const std::string json = buffer.str();

        const int schema = extractInteger(json, "schemaVersion");
        if (schema != 1 && schema != static_cast<int>(Tfs1098TargetConfig::SchemaVersion)) {
            throw std::runtime_error("Unsupported TFS1098 runtime profile schemaVersion");
        }
        const std::string backend = extractString(json, "backend");
        if (backend != "tfs1098") {
            throw std::runtime_error("TFS1098 runtime profile backend must be 'tfs1098'");
        }

        Tfs1098TargetConfig config;
        config.mapName = extractString(json, "mapName");
        config.outputDirectory = std::filesystem::path(extractString(json, "outputDirectory")).lexically_normal();

        // Schema v1 predates compatibility profiles. Preserve the old official
        // behavior by migrating it deterministically to the vanilla target.
        if (schema == 1) {
            config.compatibilityProfile = Tfs1098CompatibilityProfile::Vanilla;
        } else {
            config.compatibilityProfile = parseTfs1098CompatibilityProfile(
                extractString(json, "compatibilityProfile"));
        }

        validate(config);
        return config;
    }

    static void save(
        const std::filesystem::path& projectRoot,
        const Tfs1098TargetConfig& config) {

        validate(config);
        const auto path = pathForProject(projectRoot);
        std::filesystem::create_directories(path.parent_path());
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("Unable to write TFS1098 runtime profile: " + path.string());

        output
            << "{\n"
            << "  \"schemaVersion\": " << Tfs1098TargetConfig::SchemaVersion << ",\n"
            << "  \"backend\": \"tfs1098\",\n"
            << "  \"mapName\": \"" << escapeJson(config.mapName) << "\",\n"
            << "  \"outputDirectory\": \"" << escapeJson(config.outputDirectory.generic_string()) << "\",\n"
            << "  \"compatibilityProfile\": \""
            << tfs1098CompatibilityProfileId(config.compatibilityProfile)
            << "\"\n"
            << "}\n";
        if (!output) throw std::runtime_error("Failed while writing TFS1098 runtime profile: " + path.string());
    }

    [[nodiscard]] static std::filesystem::path resolveOutputDirectory(
        const std::filesystem::path& projectRoot,
        const Tfs1098TargetConfig& config) {

        validate(config);
        return std::filesystem::absolute(projectRoot / config.outputDirectory).lexically_normal();
    }

    static void validate(const Tfs1098TargetConfig& config) {
        if (!validMapName(config.mapName)) {
            throw std::runtime_error("TFS1098 mapName must contain only letters, digits, '_' or '-'");
        }
        if (!safeRelative(config.outputDirectory)) {
            throw std::runtime_error("TFS1098 outputDirectory must be a safe relative project path");
        }
        (void)tfs1098CompatibilityProfileId(config.compatibilityProfile);
    }

private:
    [[nodiscard]] static bool validMapName(const std::string& value) noexcept {
        if (value.empty()) return false;
        return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
            return std::isalnum(ch) != 0 || ch == '_' || ch == '-';
        });
    }

    [[nodiscard]] static bool safeRelative(const std::filesystem::path& value) noexcept {
        if (value.empty() || value.is_absolute() || value.has_root_name() || value.has_root_directory()) return false;
        for (const auto& part : value) {
            if (part == "..") return false;
        }
        return true;
    }

    [[nodiscard]] static std::string extractString(const std::string& json, const std::string& key) {
        const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
        std::smatch match;
        if (!std::regex_search(json, match, pattern)) {
            throw std::runtime_error("Missing string field in TFS1098 runtime profile: " + key);
        }
        return match[1].str();
    }

    [[nodiscard]] static int extractInteger(const std::string& json, const std::string& key) {
        const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*([0-9]+)");
        std::smatch match;
        if (!std::regex_search(json, match, pattern)) {
            throw std::runtime_error("Missing integer field in TFS1098 runtime profile: " + key);
        }
        return std::stoi(match[1].str());
    }

    [[nodiscard]] static std::string escapeJson(const std::string& value) {
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
};

} // namespace fantasy::studio::runtime
