#pragma once

#include "game/game_types.h"
#include <vector>

namespace StorageCraft {

// Provides safe read/write access to the player's inventory
// within the BlackSpace Engine's BSPlayerComponent.
class InventoryAccessor {
public:
    explicit InventoryAccessor(BSPlayerComponent* component);

    bool IsValid() const;

    // Get total count of a specific item across all stacks.
    int32_t GetItemCount(int32_t itemId) const;

    // Consume (remove) a specified amount of an item.
    // Returns false and makes no changes if insufficient quantity.
    bool ConsumeItem(int32_t itemId, int32_t amount);

    // Get a snapshot of all items currently in inventory.
    std::vector<BSItemEntry> GetItems() const;

    BSPlayerComponent* GetRaw() const { return m_component; }

private:
    BSArray<BSItemEntry>* GetItemArray() const;
    BSPlayerComponent* m_component;
};

} // namespace StorageCraft
