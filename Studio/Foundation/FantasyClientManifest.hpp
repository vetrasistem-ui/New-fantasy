#pragma once

#include "Foundation/FantasyFoundationV2.hpp"

#include <cstdint>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fantasy::studio::foundation {

struct ClientModuleDescriptor {
    std::string id;
    bool required = true;
    std::string version;

    void validate() const {
        requireIdentifier(id, "client module id");
        requireIdentifier(version, "client module version");
    }
};

struct ClientArtifactDescriptor {
    std::filesystem::path path;
    std::string sha256;

    void validate() const {
        if (!safeRelativePath(path)) throw std::invalid_argument("client artifact path must be relative and safe");
        if (!sha256.empty() && sha256.size() != 64U) {
            throw std::invalid_argument("client artifact sha256 must contain 64 characters when supplied");
        }
    }
};

struct FantasyClientManifest {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::string clientId = "fantasy-client";
    std::string version = "v1";
    std::string runtimeId = "tfs1098";
    std::string compatibilityProfile = "vanilla";
    std::string assetProfileId;
    std::string serverHost = "127.0.0.1";
    std::uint16_t loginPort = 7171;
    std::uint16_t gamePort = 7172;
    std::string updateChannel = "stable";
    std::vector<ClientModuleDescriptor> modules;
    std::vector<ClientArtifactDescriptor> artifacts;

    void validate() const {
        requireIdentifier(clientId, "client id");
        requireIdentifier(version, "client version");
        requireIdentifier(runtimeId, "client runtime id");
        requireIdentifier(compatibilityProfile, "client compatibility profile");
        requireIdentifier(assetProfileId, "client asset profile id");
        requireIdentifier(updateChannel, "client update channel");
        if (serverHost.empty()) throw std::invalid_argument("client server host is required");
        if (loginPort == 0 || gamePort == 0) throw std::invalid_argument("client login/game ports must be non-zero");
        for (const auto& module : modules) module.validate();
        for (const auto& artifact : artifacts) artifact.validate();
    }

    [[nodiscard]] std::string toJson() const {
        validate();
        std::ostringstream out;
        out << "{\n"
            << "  \"schemaVersion\": " << SchemaVersion << ",\n"
            << "  \"clientId\": \"" << escape(clientId) << "\",\n"
            << "  \"version\": \"" << escape(version) << "\",\n"
            << "  \"runtimeId\": \"" << escape(runtimeId) << "\",\n"
            << "  \"compatibilityProfile\": \"" << escape(compatibilityProfile) << "\",\n"
            << "  \"assetProfileId\": \"" << escape(assetProfileId) << "\",\n"
            << "  \"serverHost\": \"" << escape(serverHost) << "\",\n"
            << "  \"loginPort\": " << loginPort << ",\n"
            << "  \"gamePort\": " << gamePort << ",\n"
            << "  \"updateChannel\": \"" << escape(updateChannel) << "\",\n"
            << "  \"modules\": [\n";
        for (std::size_t i = 0; i < modules.size(); ++i) {
            const auto& module = modules[i];
            out << "    {\"id\": \"" << escape(module.id)
                << "\", \"required\": " << (module.required ? "true" : "false")
                << ", \"version\": \"" << escape(module.version) << "\"}";
            if (i + 1U != modules.size()) out << ',';
            out << '\n';
        }
        out << "  ],\n  \"artifacts\": [\n";
        for (std::size_t i = 0; i < artifacts.size(); ++i) {
            const auto& artifact = artifacts[i];
            out << "    {\"path\": \"" << escape(artifact.path.generic_string())
                << "\", \"sha256\": \"" << escape(artifact.sha256) << "\"}";
            if (i + 1U != artifacts.size()) out << ',';
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
