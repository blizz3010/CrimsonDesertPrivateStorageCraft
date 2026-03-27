#pragma once

#include "game/game_types.h"
#include <vector>
#include <atomic>

namespace StorageCraft {

// Provides safe read/write access to a private storage container.
// Includes range checking and locking for concurrent access safety.
class StorageAccessor {
public:
    explicit StorageAccessor(UStorageContainer* storage);

    bool IsValid() const;

    // Get total count of a specific item across all stacks.
    int32_t GetItemCount(int32_t itemId) const;

    // Consume a specified amount of an item from storage.
    // Returns false and makes no changes if insufficient.
    bool ConsumeItem(int32_t itemId, int32_t amount);

    // Check if the storage container is within acceptable range of the player.
    bool IsInRange(float maxDistance) const;

    // Attempt to acquire exclusive access to this container.
    // Returns false if another system is currently modifying it.
    bool TryLock();
    void Unlock();
    bool IsLocked() const { return m_locked; }

    // Get a snapshot of all items in storage.
    std::vector<FItemStack> GetItems() const;

    UStorageContainer* GetRaw() const { return m_storage; }

private:
    TArray<FItemStack>* GetItemArray() const;
    FVector GetStoragePosition() const;
    FVector GetPlayerPosition() const;

    UStorageContainer* m_storage;
    bool m_locked = false;
};

} // namespace StorageCraft
