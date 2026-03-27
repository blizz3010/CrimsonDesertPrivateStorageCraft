#include "game/inventory_accessor.h"
#include "core/memory.h"
#include "core/logger.h"

namespace StorageCraft {

// ============================================================================
// Offsets into UPlayerInventory - update these per game patch
// ============================================================================
namespace Offsets {
    constexpr ptrdiff_t Inventory_ItemArray = 0x148; // TArray<FItemStack>
}

InventoryAccessor::InventoryAccessor(UPlayerInventory* inventory)
    : m_inventory(inventory) {}

bool InventoryAccessor::IsValid() const {
    return m_inventory != nullptr && GetItemArray() != nullptr;
}

int32_t InventoryAccessor::GetItemCount(int32_t itemId) const {
    if (!IsValid()) return 0;

    auto* items = GetItemArray();
    int32_t total = 0;
    for (int32_t i = 0; i < items->Count; i++) {
        if (items->Data[i].ItemId == itemId) {
            total += items->Data[i].Count;
        }
    }
    return total;
}

bool InventoryAccessor::ConsumeItem(int32_t itemId, int32_t amount) {
    if (!IsValid() || amount <= 0) return false;

    auto* items = GetItemArray();
    int32_t remaining = amount;

    // First pass: verify we have enough
    int32_t available = 0;
    for (int32_t i = 0; i < items->Count; i++) {
        if (items->Data[i].ItemId == itemId) {
            available += items->Data[i].Count;
        }
    }
    if (available < amount) return false;

    // Second pass: consume
    for (int32_t i = 0; i < items->Count && remaining > 0; i++) {
        if (items->Data[i].ItemId != itemId) continue;

        int32_t take = std::min(items->Data[i].Count, remaining);
        items->Data[i].Count -= take;
        remaining -= take;

        // Remove empty stacks by swapping with last element
        if (items->Data[i].Count <= 0) {
            items->Data[i] = items->Data[items->Count - 1];
            items->Count--;
            i--; // Re-check this index since we swapped
        }
    }

    Logger::Debug("Inventory: consumed {}x item {}", amount, itemId);
    return true;
}

std::vector<FItemStack> InventoryAccessor::GetItems() const {
    std::vector<FItemStack> result;
    if (!IsValid()) return result;

    auto* items = GetItemArray();
    result.reserve(items->Count);
    for (int32_t i = 0; i < items->Count; i++) {
        result.push_back(items->Data[i]);
    }
    return result;
}

TArray<FItemStack>* InventoryAccessor::GetItemArray() const {
    return reinterpret_cast<TArray<FItemStack>*>(
        reinterpret_cast<uintptr_t>(m_inventory) + Offsets::Inventory_ItemArray
    );
}

} // namespace StorageCraft
