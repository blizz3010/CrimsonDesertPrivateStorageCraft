#pragma once

#include "game/inventory_accessor.h"
#include "game/storage_accessor.h"
#include <vector>

namespace StorageCraft {

struct ConsumptionEntry {
    int32_t itemId = 0;
    int32_t fromInventory = 0;
    int32_t fromStorage = 0;
};

struct ConsumptionPlan {
    std::vector<ConsumptionEntry> entries;
};

// Two-phase commit consumer:
//   Phase 1: Validate all materials are still available
//   Phase 2: Consume from inventory first, then storage
// If validation fails, nothing is consumed (atomic rollback).
class MaterialConsumer {
public:
    static bool Execute(
        const ConsumptionPlan& plan,
        InventoryAccessor& inventory,
        std::vector<StorageAccessor>& storages
    );

private:
    static bool Validate(
        const ConsumptionPlan& plan,
        const InventoryAccessor& inventory,
        const std::vector<StorageAccessor>& storages
    );
};

} // namespace StorageCraft
