#pragma once

#include "game/inventory_accessor.h"
#include "game/storage_accessor.h"
#include <optional>
#include <vector>

namespace StorageCraft {

// Describes how much of an item comes from each source.
struct MaterialSource {
    int32_t fromInventory = 0;
    int32_t fromStorage = 0;
};

// Unified view of materials across inventory and storage containers.
// This is the central abstraction that both the UI hook and craft hook use.
class MaterialPool {
public:
    MaterialPool(InventoryAccessor& inventory, std::vector<StorageAccessor>& storages);

    // Total available count across all sources.
    int32_t GetTotalCount(int32_t itemId) const;

    // Plan how to fulfill a material requirement.
    // Inventory is consumed first, then storage.
    // Returns nullopt if total available is insufficient.
    std::optional<MaterialSource> PlanConsumption(int32_t itemId, int32_t needed) const;

    // Returns true if fulfilling this requirement would pull from storage.
    bool RequiresStorage(int32_t itemId, int32_t needed) const;

    // Get inventory-only count (for comparison / UI display).
    int32_t GetInventoryCount(int32_t itemId) const;

    // Get storage-only count.
    int32_t GetStorageCount(int32_t itemId) const;

private:
    InventoryAccessor& m_inventory;
    std::vector<StorageAccessor>& m_storages;
};

} // namespace StorageCraft
