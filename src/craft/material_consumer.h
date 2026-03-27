#pragma once

#include "game/inventory_accessor.h"
#include "game/storage_accessor.h"
#include <vector>

namespace StorageCraft {

// A single entry in a consumption plan.
struct ConsumptionEntry {
    int32_t itemId = 0;
    int32_t fromInventory = 0;
    int32_t fromStorage = 0;
};

// A complete plan for consuming materials for a craft.
struct ConsumptionPlan {
    std::vector<ConsumptionEntry> entries;
};

// Executes material consumption with a two-phase commit:
// 1. Validate all materials are still available (guard against race conditions)
// 2. Consume from inventory first, then storage
// If validation fails, no materials are consumed (atomic rollback).
class MaterialConsumer {
public:
    // Execute the consumption plan.
    // Returns true if all materials were successfully consumed.
    // Returns false if any material was insufficient; no changes are made.
    static bool Execute(
        const ConsumptionPlan& plan,
        InventoryAccessor& inventory,
        std::vector<StorageAccessor>& storages
    );

private:
    // Phase 1: Verify all materials are available.
    static bool Validate(
        const ConsumptionPlan& plan,
        const InventoryAccessor& inventory,
        const std::vector<StorageAccessor>& storages
    );
};

} // namespace StorageCraft
