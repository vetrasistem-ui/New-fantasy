#pragma once

#include "Foundation/FantasyClientManifest.hpp"

#include <algorithm>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace fantasy::studio::foundation {

enum class ClientUpdateOperationKind {
    Download,
    Remove,
    Keep,
};

struct ClientUpdateOperation {
    ClientUpdateOperationKind kind = ClientUpdateOperationKind::Keep;
    std::filesystem::path path;
    std::string expectedSha256;
};

struct ClientUpdatePlan {
    std::string fromVersion;
    std::string toVersion;
    std::vector<ClientUpdateOperation> operations;

    [[nodiscard]] bool requiresChanges() const noexcept {
        return std::any_of(operations.begin(), operations.end(), [](const auto& operation) {
            return operation.kind != ClientUpdateOperationKind::Keep;
        });
    }
};

class FantasyClientUpdatePlanner {
public:
    [[nodiscard]] static ClientUpdatePlan compare(
        const FantasyClientManifest& current,
        const FantasyClientManifest& target) {

        current.validate();
        target.validate();
        if (current.clientId != target.clientId) {
            throw std::invalid_argument("client update manifests have different clientId values");
        }
        if (current.runtimeId != target.runtimeId) {
            throw std::invalid_argument("client update cannot silently change runtimeId");
        }
        if (current.compatibilityProfile != target.compatibilityProfile) {
            throw std::invalid_argument("client update cannot silently change compatibilityProfile");
        }
        if (current.assetProfileId != target.assetProfileId) {
            throw std::invalid_argument("client update cannot silently change assetProfileId");
        }

        ClientUpdatePlan plan;
        plan.fromVersion = current.version;
        plan.toVersion = target.version;

        const auto currentFiles = index(current.artifacts);
        const auto targetFiles = index(target.artifacts);
        for (const auto& [path, targetArtifact] : targetFiles) {
            const auto existing = currentFiles.find(path);
            if (existing == currentFiles.end() || existing->second.sha256 != targetArtifact.sha256) {
                plan.operations.push_back({ClientUpdateOperationKind::Download, path, targetArtifact.sha256});
            } else {
                plan.operations.push_back({ClientUpdateOperationKind::Keep, path, targetArtifact.sha256});
            }
        }
        for (const auto& [path, currentArtifact] : currentFiles) {
            (void)currentArtifact;
            if (!targetFiles.contains(path)) {
                plan.operations.push_back({ClientUpdateOperationKind::Remove, path, ""});
            }
        }
        std::sort(plan.operations.begin(), plan.operations.end(), [](const auto& left, const auto& right) {
            return left.path.generic_string() < right.path.generic_string();
        });
        return plan;
    }

private:
    [[nodiscard]] static std::map<std::filesystem::path, ClientArtifactDescriptor> index(
        const std::vector<ClientArtifactDescriptor>& artifacts) {

        std::map<std::filesystem::path, ClientArtifactDescriptor> result;
        for (const auto& artifact : artifacts) {
            artifact.validate();
            if (!result.emplace(artifact.path.lexically_normal(), artifact).second) {
                throw std::invalid_argument("duplicate client artifact path: " + artifact.path.string());
            }
        }
        return result;
    }
};

} // namespace fantasy::studio::foundation
