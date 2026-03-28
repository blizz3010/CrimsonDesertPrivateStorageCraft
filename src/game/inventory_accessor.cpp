#include "game/inventory_accessor.h"
#include "core/memory.h"
#include "core/logger.h"

namespace StorageCraft {

// ============================================================================
// BlackSpace Engine offsets - VERIFIED from player-status-modifier
//
// The player-status-modifier discovered:
//   - Component data table at component + 0x58 (from "48 03 46 58")
//   - Entries are 16-byte aligned (from "48 C1 E0 04" = shl rax, 4)
//   - Item count at entry + 0x10 (from item-gain AOB "49 01 4C 38 10")
//
// The inventory item array is accessed differently from stats:
//   - Stats: component + 0x58 -> stat table (entries with type/value/max)
//   - Items: the item-gain hook shows r8 = item table base, rdi = offset
//   - The item table base pointer may be at a different component offset
//     than the stat table (0x58). Needs verification.
//
// APPROACH: We use the player-pointer hook to capture the component,
// then walk from the component to find the item data.
// ============================================================================
namespace Offsets {
    // Verified: owner actor -> component (from player-pointer AOB hook)
    constexpr ptrdiff_t Owner_Component = 0x20;

    // Verified: component -> stat/data table base (from stats AOB: "48 03 46 58")
    constexpr ptrdiff_t Component_DataTable = 0x58;

    // Verified: item count within entry (from item-gain: [r8+rdi+0x10])
    constexpr ptrdiff_t ItemEntry_Count = 0x10;

    // Verified: entry size = 16-byte shift (shl rax, 4) but actual stride
    // may differ for items vs stats. Stats use 16-byte entries with fields
    // at +0x00 (type), +0x08 (value), +0x18 (max).
    // Items likely use a wider entry. Needs verification.
    constexpr ptrdiff_t EntryAlignment = 0x10; // 16 bytes (shift left 4)
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
        if (items->Data[i].ItemNo == itemId) {
            total += static_cast<int32_t>(items->Data[i].Count);
        }
    }
    return total;
}

bool InventoryAccessor::ConsumeItem(int32_t itemId, int32_t amount) {
    if (!IsValid() || amount <= 0) return false;

    auto* items = GetItemArray();

    // Verify sufficient quantity
    int32_t available = 0;
    for (int32_t i = 0; i < items->Count; i++) {
        if (items->Data[i].ItemNo == itemId) {
            available += static_cast<int32_t>(items->Data[i].Count);
        }
    }
    if (available < amount) return false;

    // Consume across stacks
    int32_t remaining = amount;
    for (int32_t i = 0; i < items->Count && remaining > 0; i++) {
        if (items->Data[i].ItemNo != itemId) continue;

        auto entryCount = static_cast<int32_t>(items->Data[i].Count);
        int32_t take = std::min(entryCount, remaining);
        items->Data[i].Count -= take;
        remaining -= take;

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
    // Walk: component -> data table offset
    // The exact offset for the ITEM array (vs stat array at +0x58) needs
    // verification. The item-gain hook accesses items via r8 register which
    // may be resolved from a different offset than the stat table.
    //
    // For now, we read from the same component base. The player-pointer
    // hook gives us the component; we'll use the direct r8 register value
    // captured in the craft hook for actual item table access.
    return reinterpret_cast<BSArray<BSItemEntry>*>(
        reinterpret_cast<uintptr_t>(m_component) + Offsets::Component_DataTable
    );
}

} // namespace StorageCraft
