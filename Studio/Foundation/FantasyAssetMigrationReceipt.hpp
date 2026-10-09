#pragma once

#include "Foundation/FantasyAssetMigrationEngine.hpp"

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace fantasy::studio::foundation {

struct AssetMigrationBackup {
    std::uint32_t targetId = 0;
    std::string objectBackupRef;
    std::string visualBackupRef;
};

struct AssetMigrationReceiptEntry {
    std::uint32_t legacyId = 0;
    std::uint32_t targetId = 0;
    AssetMigrationMode mode = AssetMigrationMode::AddAsNew;
    bool createdTarget = false;
    std::string objectBackupRef;
    std::string visualBackupRef;
};

struct AssetMigrationReceipt {
    static constexpr std::uint32_t SchemaVersion = 1;

    std::string sourceProfile;
    std::string targetProfile;
    std::string migrationId;
    std::vector<AssetMigrationReceiptEntry> entries;

    void validate() const {
        requireIdentifier(sourceProfile, "asset migration receipt source profile");
        requireIdentifier(targetProfile, "asset migration receipt target profile");
        requireIdentifier(migrationId, "asset migration receipt id");
        for (const auto& entry : entries) {
            if (entry.legacyId == 0 || entry.targetId == 0) {
                throw std::invalid_argument("asset migration receipt ids must be non-zero");
            }
            if (entry.mode == AssetMigrationMode::AddAsNew && !entry.createdTarget) {
                throw std::invalid_argument("AddAsNew receipt must mark createdTarget");
            }
            if (entry.mode == AssetMigrationMode::ReplaceObject && entry.objectBackupRef.empty()) {
                throw std::invalid_argument("ReplaceObject receipt requires object backup reference");
            }
            if (entry.mode == AssetMigrationMode::ReplaceVisualOnly && entry.visualBackupRef.empty()) {
                throw std::invalid_argument("ReplaceVisualOnly receipt requires visual backup reference");
            }
        }
    }
};

enum class AssetRollbackOperationKind {
    RemoveCreatedTarget,
    RestoreObject,
    RestoreVisual,
};

struct AssetRollbackOperation {
    AssetRollbackOperationKind kind = AssetRollbackOperationKind::RemoveCreatedTarget;
    std::uint32_t targetId = 0;
    std::string backupRef;
};

class FantasyAssetMigrationReceiptBuilder {
public:
    [[nodiscard]] static AssetMigrationReceipt create(
        const std::string& migrationId,
        const AssetMigrationPlan& plan,
        const AssetMigrationPreview& preview,
        const std::map<std::uint32_t, AssetMigrationBackup>& backups) {

        plan.validate();
        requireIdentifier(migrationId, "asset migration id");
        if (plan.entries.size() != preview.resolutions.size()) {
            throw std::invalid_argument("migration plan and preview sizes differ");
        }

        AssetMigrationReceipt receipt;
        receipt.sourceProfile = plan.sourceProfile;
        receipt.targetProfile = plan.targetProfile;
        receipt.migrationId = migrationId;

        for (std::size_t i = 0; i < plan.entries.size(); ++i) {
            const auto& entry = plan.entries[i];
            const auto& resolution = preview.resolutions[i];
            if (entry.legacyId != resolution.legacyId || entry.mode != resolution.mode) {
                throw std::invalid_argument("migration preview does not correspond to migration plan");
            }

            AssetMigrationReceiptEntry receiptEntry;
            receiptEntry.legacyId = entry.legacyId;
            receiptEntry.targetId = resolution.resolvedTargetId;
            receiptEntry.mode = entry.mode;
            receiptEntry.createdTarget = entry.mode == AssetMigrationMode::AddAsNew;

            if (entry.mode != AssetMigrationMode::AddAsNew) {
                const auto backup = backups.find(resolution.resolvedTargetId);
                if (backup == backups.end()) {
                    throw std::invalid_argument("replacement migration requires a captured target backup");
                }
                receiptEntry.objectBackupRef = backup->second.objectBackupRef;
                receiptEntry.visualBackupRef = backup->second.visualBackupRef;
            }
            receipt.entries.push_back(std::move(receiptEntry));
        }
        receipt.validate();
        return receipt;
    }

    [[nodiscard]] static std::vector<AssetRollbackOperation> rollbackPlan(
        const AssetMigrationReceipt& receipt) {

        receipt.validate();
        std::vector<AssetRollbackOperation> operations;
        operations.reserve(receipt.entries.size());
        // Reverse application order to make rollback safe for dependent batches.
        for (auto it = receipt.entries.rbegin(); it != receipt.entries.rend(); ++it) {
            if (it->mode == AssetMigrationMode::AddAsNew) {
                operations.push_back({AssetRollbackOperationKind::RemoveCreatedTarget, it->targetId, ""});
            } else if (it->mode == AssetMigrationMode::ReplaceObject) {
                operations.push_back({AssetRollbackOperationKind::RestoreObject, it->targetId, it->objectBackupRef});
            } else {
                operations.push_back({AssetRollbackOperationKind::RestoreVisual, it->targetId, it->visualBackupRef});
            }
        }
        return operations;
    }
};

} // namespace fantasy::studio::foundation
