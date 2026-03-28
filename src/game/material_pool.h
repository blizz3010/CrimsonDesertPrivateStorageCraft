#pragma once

#include "game/inventory_accessor.h"
#include "game/storage_accessor.h"
#include <optional>
#include <vector>

namespace StorageCraft {

struct MaterialSource {
    int32_t fromInventory = 0;
    int32_t fromStorage = 0;
};

// Unified view of materials across inventory and storage containers.
// Implements the inventory-first consumption priority.
class MaterialPool {
public:
    MaterialPool(InventoryAccessor& inventory, std::vector<StorageAccessor>& storages);

    int32_t GetTotalCount(int32_t itemId) const;
    int32_t GetInventoryCount(int32_t itemId) const;
    int32_t GetStorageCount(int32_t itemId) const;

    // Plan consumption: inventory first, then storage.
    // Returns nullopt if total is insufficient.
    std::optional<MaterialSource> PlanConsumption(int32_t itemId, int32_t needed) const;

    // True if fulfilling this requirement would need storage materials.
    bool RequiresStorage(int32_t itemId, int32_t needed) const;

private:
    InventoryAccessor& m_inventory;
    std::vector<StorageAccessor>& m_storages;
};

} // namespace StorageCraft
