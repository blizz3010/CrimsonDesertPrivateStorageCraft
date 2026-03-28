#include "game/material_pool.h"

namespace StorageCraft {

MaterialPool::MaterialPool(InventoryAccessor& inventory, std::vector<StorageAccessor>& storages)
    : m_inventory(inventory)
    , m_storages(storages) {}

int32_t MaterialPool::GetTotalCount(int32_t itemId) const {
    return GetInventoryCount(itemId) + GetStorageCount(itemId);
}

int32_t MaterialPool::GetInventoryCount(int32_t itemId) const {
    return m_inventory.GetItemCount(itemId);
}

int32_t MaterialPool::GetStorageCount(int32_t itemId) const {
    int32_t total = 0;
    for (auto& storage : m_storages) {
        total += storage.GetItemCount(itemId);
    }
    return total;
}

std::optional<MaterialSource> MaterialPool::PlanConsumption(int32_t itemId, int32_t needed) const {
    if (needed <= 0) return MaterialSource{0, 0};

    int32_t invCount = GetInventoryCount(itemId);
    int32_t storCount = GetStorageCount(itemId);

    if (invCount + storCount < needed) return std::nullopt;

    MaterialSource source;
    source.fromInventory = std::min(invCount, needed);
    source.fromStorage = needed - source.fromInventory;
    return source;
}

bool MaterialPool::RequiresStorage(int32_t itemId, int32_t needed) const {
    if (needed <= 0) return false;
    return GetInventoryCount(itemId) < needed;
}

} // namespace StorageCraft
