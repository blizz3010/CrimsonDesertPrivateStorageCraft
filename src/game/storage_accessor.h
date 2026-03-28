#pragma once

#include "game/game_types.h"
#include <vector>

namespace StorageCraft {

// Provides safe read/write access to a private storage container
// within the BlackSpace Engine's BSStorageComponent.
class StorageAccessor {
public:
    explicit StorageAccessor(BSStorageComponent* storage);

    bool IsValid() const;

    int32_t GetItemCount(int32_t itemId) const;
    bool ConsumeItem(int32_t itemId, int32_t amount);

    // Check if storage is within acceptable range of the player.
    bool IsInRange(float maxDistance) const;

    // Exclusive access to prevent concurrent modification.
    bool TryLock();
    void Unlock();
    bool IsLocked() const { return m_locked; }

    std::vector<BSItemEntry> GetItems() const;
    BSStorageComponent* GetRaw() const { return m_storage; }

private:
    BSArray<BSItemEntry>* GetItemArray() const;
    FVector3 GetStoragePosition() const;
    FVector3 GetPlayerPosition() const;

    BSStorageComponent* m_storage;
    bool m_locked = false;
};

} // namespace StorageCraft
