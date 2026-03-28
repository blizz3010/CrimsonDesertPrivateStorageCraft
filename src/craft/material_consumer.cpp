#include "craft/material_consumer.h"
#include "core/logger.h"

namespace StorageCraft {

bool MaterialConsumer::Execute(
    const ConsumptionPlan& plan,
    InventoryAccessor& inventory,
    std::vector<StorageAccessor>& storages
) {
    // Phase 1: Validate
    if (!Validate(plan, inventory, storages)) {
        Logger::Warn("MaterialConsumer: validation failed, aborting");
        return false;
    }

    // Phase 2: Consume (inventory first, then storage)
    for (const auto& entry : plan.entries) {
        if (entry.fromInventory > 0) {
            if (!inventory.ConsumeItem(entry.itemId, entry.fromInventory)) {
                Logger::Error("MaterialConsumer: unexpected failure consuming {}x item {} from inventory",
                              entry.fromInventory, entry.itemId);
                return false;
            }
        }

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
                }
            }

            if (remaining > 0) {
                Logger::Error("MaterialConsumer: short by {} of item {} from storage",
                              remaining, entry.itemId);
                return false;
            }
        }

        Logger::Info("MaterialConsumer: consumed item {} (inv:{}, storage:{})",
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
        if (inventory.GetItemCount(entry.itemId) < entry.fromInventory) return false;

        if (entry.fromStorage > 0) {
            int32_t storageAvailable = 0;
            for (const auto& storage : storages) {
                storageAvailable += storage.GetItemCount(entry.itemId);
            }
            if (storageAvailable < entry.fromStorage) return false;
        }
    }
    return true;
}

} // namespace StorageCraft
