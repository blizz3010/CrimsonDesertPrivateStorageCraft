#include "game/inventory_accessor.h"
#include "core/memory.h"
#include "core/logger.h"

namespace StorageCraft {

// ============================================================================
// BlackSpace Engine offsets into BSPlayerComponent
//
// These must be discovered via reverse engineering (debugger + AOB scanning).
// The player-status-modifier found player data by:
//   1. Scanning for the "player-pointer" AOB
//   2. Reading component at rdx+0x68, then +0x20 for sub-component
//   3. Item data lives at a known offset from the component base
//
// TODO: These offsets need to be verified against the actual game binary.
// Use x64dbg or Cheat Engine to walk from the player component pointer
// to the inventory item array.
// ============================================================================
namespace Offsets {
    // BSPlayerComponent -> inventory item array (BSArray<BSItemEntry>)
    // Discovered by tracing from the item-gain AOB: "49 01 4C 38 10"
    // The r8 register points to the item table base, rdi is the slot index.
    // The item table is at component + 0x148 (needs verification per patch).
    constexpr ptrdiff_t PlayerComp_ItemArray = 0x148;
}

InventoryAccessor::InventoryAccessor(BSPlayerComponent* component)
    : m_component(component) {}

bool InventoryAccessor::IsValid() const {
    if (!Memory::IsValidPtr(m_component)) return false;
    auto* items = GetItemArray();
    return items != nullptr && Memory::IsValidPtr(items->Data);
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

    // Verify sufficient quantity before modifying
    int32_t available = 0;
    for (int32_t i = 0; i < items->Count; i++) {
        if (items->Data[i].ItemId == itemId) {
            available += items->Data[i].Count;
        }
    }
    if (available < amount) return false;

    // Consume across stacks
    int32_t remaining = amount;
    for (int32_t i = 0; i < items->Count && remaining > 0; i++) {
        if (items->Data[i].ItemId != itemId) continue;

        int32_t take = std::min(items->Data[i].Count, remaining);
        items->Data[i].Count -= take;
        remaining -= take;

        // Remove empty stacks (swap with last element)
        if (items->Data[i].Count <= 0) {
            items->Data[i] = items->Data[items->Count - 1];
            items->Count--;
            i--;
        }
    }

    Logger::Debug("Inventory: consumed {}x item {}", amount, itemId);
    return true;
}

std::vector<BSItemEntry> InventoryAccessor::GetItems() const {
    std::vector<BSItemEntry> result;
    if (!IsValid()) return result;

    auto* items = GetItemArray();
    result.reserve(items->Count);
    for (int32_t i = 0; i < items->Count; i++) {
        result.push_back(items->Data[i]);
    }
    return result;
}

BSArray<BSItemEntry>* InventoryAccessor::GetItemArray() const {
    return reinterpret_cast<BSArray<BSItemEntry>*>(
        reinterpret_cast<uintptr_t>(m_component) + Offsets::PlayerComp_ItemArray
    );
}

} // namespace StorageCraft
