#include "craft/material_consumer.h"
#include "core/logger.h"

namespace StorageCraft {

bool MaterialConsumer::Execute(
    const ConsumptionPlan& plan,
    InventoryAccessor& inventory,
    std::vector<StorageAccessor>& storages
) {
    // Phase 1: Validate everything before making any changes
    if (!Validate(plan, inventory, storages)) {
        Logger::Warn("MaterialConsumer: validation failed, aborting craft");
        return false;
    }

    // Phase 2: Consume from inventory first, then storage
    for (const auto& entry : plan.entries) {
        // Consume from inventory
        if (entry.fromInventory > 0) {
            if (!inventory.ConsumeItem(entry.itemId, entry.fromInventory)) {
                // This shouldn't happen after validation, but handle it
                Logger::Error("MaterialConsumer: unexpected failure consuming {}x item {} from inventory",
                              entry.fromInventory, entry.itemId);
                return false;
            }
        }

        // Consume from storage (spread across available containers)
        if (entry.fromStorage > 0) {
            int32_t remaining = entry.fromStorage;

            for (auto& storage : storages) {
                if (remaining <= 0) break;
                if (!storage.IsLocked()) continue;

                int32_t available = storage.GetItemCount(entry.itemId);
                if (available <= 0) continue;

                int32_t take = std::min(available, remaining);
                if (storage.ConsumeItem(entry.itemId, take)) {
                    remaining -= take;
                } else {
                    Logger::Error("MaterialConsumer: failed to consume {}x item {} from storage",
                                  take, entry.itemId);
                    return false;
                }
            }

            if (remaining > 0) {
                Logger::Error("MaterialConsumer: could not consume all of item {} from storage "
                              "(short by {})", entry.itemId, remaining);
                return false;
            }
        }

        Logger::Info("MaterialConsumer: consumed item {} (inv: {}, storage: {})",
                     entry.itemId, entry.fromInventory, entry.fromStorage);
    }

    return true;
}

bool MaterialConsumer::Validate(
    const ConsumptionPlan& plan,
    const InventoryAccessor& inventory,
    const std::vector<StorageAccessor>& storages
) {
    for (const auto& entry : plan.entries) {
        // Check inventory has enough
        int32_t invAvailable = inventory.GetItemCount(entry.itemId);
        if (invAvailable < entry.fromInventory) {
            Logger::Debug("MaterialConsumer: insufficient inventory for item {} "
                          "(need {}, have {})", entry.itemId, entry.fromInventory, invAvailable);
            return false;
        }

        // Check storage has enough
        if (entry.fromStorage > 0) {
            int32_t storageAvailable = 0;
            for (const auto& storage : storages) {
                storageAvailable += storage.GetItemCount(entry.itemId);
            }
            if (storageAvailable < entry.fromStorage) {
                Logger::Debug("MaterialConsumer: insufficient storage for item {} "
                              "(need {}, have {})", entry.itemId, entry.fromStorage, storageAvailable);
                return false;
            }
        }
    }
    return true;
}

} // namespace StorageCraft
