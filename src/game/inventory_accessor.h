#pragma once

#include "game/game_types.h"
#include <vector>

namespace StorageCraft {

// Provides safe read/write access to the player's inventory.
// Wraps raw game pointers and applies known memory offsets.
class InventoryAccessor {
public:
    explicit InventoryAccessor(UPlayerInventory* inventory);

    // Check if the underlying pointer is valid.
    bool IsValid() const;

    // Get total count of a specific item across all stacks.
    int32_t GetItemCount(int32_t itemId) const;

    // Consume (remove) a specified amount of an item.
    // Returns true if the full amount was consumed.
    // Returns false and makes no changes if insufficient quantity.
    bool ConsumeItem(int32_t itemId, int32_t amount);

    // Get a snapshot of all items currently in inventory.
    std::vector<FItemStack> GetItems() const;

    UPlayerInventory* GetRaw() const { return m_inventory; }

private:
    TArray<FItemStack>* GetItemArray() const;
    UPlayerInventory* m_inventory;
};

} // namespace StorageCraft
