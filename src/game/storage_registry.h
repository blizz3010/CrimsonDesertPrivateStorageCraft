#pragma once

#include "game/storage_accessor.h"
#include "game/game_types.h"
#include <vector>
#include <mutex>

namespace StorageCraft {

// Manages discovered storage container pointers at runtime.
//
// Storage containers are discovered when the player opens one (via a
// mid-function hook on the storage UI open path). The registry caches
// the BSStorageComponent pointer and provides StorageAccessor instances
// for the crafting system.
//
// Discovery flow:
//   1. Player opens a storage container in-game
//   2. The StorageOpen hook fires (from storage_hook pattern)
//   3. The hook reads the storage component pointer from registers
//   4. RegisterStorage() caches the pointer
//   5. CraftHook queries GetActiveStorages() during item-loss
//
// The registry also prunes invalid/stale pointers on each access.
class StorageRegistry {
public:
    // Register a newly discovered storage component.
    // Called from the storage-open hook callback.
    static void RegisterStorage(BSStorageComponent* storage);

    // Remove a specific storage (e.g., when the player closes it).
    static void UnregisterStorage(BSStorageComponent* storage);

    // Get all currently valid and in-range storage accessors.
    // Automatically prunes invalid pointers.
    static std::vector<StorageAccessor> GetActiveStorages(float maxDistance);

    // Get all registered storages regardless of range.
    static std::vector<StorageAccessor> GetAllStorages();

    // Check if any storages are registered.
    static bool HasStorages();

    // Clear all registered storages (used on shutdown/reset).
    static void Clear();

    // The most recently opened storage (for quick single-storage access).
    static BSStorageComponent* GetLastOpened();

private:
    static std::mutex s_mutex;
    static std::vector<BSStorageComponent*> s_storages;
    static BSStorageComponent* s_lastOpened;

    // Remove entries where the pointer is no longer valid.
    static void PruneInvalid();
};

} // namespace StorageCraft
