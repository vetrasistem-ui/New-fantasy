#pragma once

#include "Foundation/FantasyFoundationV2.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fantasy::studio::foundation {

struct BuildArtifactRecord {
    std::string role;
    std::filesystem::path path;
    std::string sha256;

    void validate() const {
        requireIdentifier(role, "build artifact role");
        if (!safeRelativePath(path)) {
            throw std::invalid_argument("build artifact path must be project-relative and safe");
        }
        if (!sha256.empty() && sha256.size() != 64U) {
            throw std::invalid_argument("build artifact sha256 must contain 64 characters when supplied");
        }
    }
};

struct FantasyBuildManifest {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::string projectId;
    std::string buildVersion;
    std::string runtimeId;
    std::string compatibilityProfile;
    std::string assetProfileId;
    std::vector<BuildArtifactRecord> artifacts;

    void validate() const {
        requireIdentifier(projectId, "build project id");
        requireIdentifier(buildVersion, "build version");
        requireIdentifier(runtimeId, "build runtime id");
        requireIdentifier(compatibilityProfile, "build compatibility profile");
        requireIdentifier(assetProfileId, "build asset profile id");
        if (artifacts.empty()) throw std::invalid_argument("Fantasy build manifest requires at least one artifact");
        for (const auto& artifact : artifacts) artifact.validate();
    }
};

struct ReleasePolicy {
    std::filesystem::path installRoot;
    std::filesystem::path backupRoot;
    std::string serviceName;
    bool backupBeforeUpdate = true;
    bool restartAfterUpdate = true;
    bool rollbackOnFailedHealthCheck = true;

    void validate() const {
        if (!installRoot.is_absolute()) throw std::invalid_argument("release installRoot must be absolute");
        if (!backupRoot.is_absolute()) throw std::invalid_argument("release backupRoot must be absolute");
        requireIdentifier(serviceName, "release service name");
        if (installRoot == backupRoot) throw std::invalid_argument("release install and backup roots must differ");
    }
};

class FantasyBuildManifestWriter {
public:
    static void write(const std::filesystem::path& outputPath, const FantasyBuildManifest& manifest) {
        manifest.validate();
        if (outputPath.empty()) throw std::invalid_argument("build manifest output path is required");
        std::filesystem::create_directories(outputPath.parent_path());
        std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("unable to write Fantasy build manifest: " + outputPath.string());
        output << toJson(manifest);
        if (!output) throw std::runtime_error("failed while writing Fantasy build manifest: " + outputPath.string());
    }

    [[nodiscard]] static std::string toJson(const FantasyBuildManifest& manifest) {
        manifest.validate();
        std::ostringstream out;
        out << "{\n"
            << "  \"schemaVersion\": " << FantasyBuildManifest::SchemaVersion << ",\n"
            << "  \"projectId\": \"" << escape(manifest.projectId) << "\",\n"
            << "  \"buildVersion\": \"" << escape(manifest.buildVersion) << "\",\n"
            << "  \"runtimeId\": \"" << escape(manifest.runtimeId) << "\",\n"
            << "  \"compatibilityProfile\": \"" << escape(manifest.compatibilityProfile) << "\",\n"
            << "  \"assetProfileId\": \"" << escape(manifest.assetProfileId) << "\",\n"
            << "  \"artifacts\": [\n";
        for (std::size_t i = 0; i < manifest.artifacts.size(); ++i) {
            const auto& artifact = manifest.artifacts[i];
            out << "    {\"role\": \"" << escape(artifact.role)
                << "\", \"path\": \"" << escape(artifact.path.generic_string())
                << "\", \"sha256\": \"" << escape(artifact.sha256) << "\"}";
            if (i + 1U != manifest.artifacts.size()) out << ',';
            out << '\n';
        }
        out << "  ]\n}\n";
        return out.str();
    }

private:
    [[nodiscard]] static std::string escape(const std::string& value) {
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

} // namespace fantasy::studio::foundation
