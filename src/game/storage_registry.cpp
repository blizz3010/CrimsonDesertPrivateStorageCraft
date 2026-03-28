#include "game/storage_registry.h"
#include "core/memory.h"
#include "core/logger.h"
#include "config/mod_config.h"

#include <algorithm>

namespace StorageCraft {

std::mutex StorageRegistry::s_mutex;
std::vector<BSStorageComponent*> StorageRegistry::s_storages;
BSStorageComponent* StorageRegistry::s_lastOpened = nullptr;

void StorageRegistry::RegisterStorage(BSStorageComponent* storage) {
    if (!Memory::IsValidPtr(storage)) return;

    std::lock_guard lock(s_mutex);

    // Check if already registered
    for (auto* existing : s_storages) {
        if (existing == storage) {
            s_lastOpened = storage;
            return;
        }
    }

    s_storages.push_back(storage);
    s_lastOpened = storage;
    Logger::Info("StorageRegistry: registered storage at {:X} (total: {})",
                 reinterpret_cast<uintptr_t>(storage), s_storages.size());
}

void StorageRegistry::UnregisterStorage(BSStorageComponent* storage) {
    std::lock_guard lock(s_mutex);
    std::erase(s_storages, storage);
    if (s_lastOpened == storage) s_lastOpened = nullptr;
}

std::vector<StorageAccessor> StorageRegistry::GetActiveStorages(float maxDistance) {
    std::lock_guard lock(s_mutex);
    PruneInvalid();

    std::vector<StorageAccessor> result;
    for (auto* ptr : s_storages) {
        StorageAccessor accessor(ptr);
        if (accessor.IsValid() && accessor.IsInRange(maxDistance)) {
            result.push_back(std::move(accessor));
        }
    }
    return result;
}

std::vector<StorageAccessor> StorageRegistry::GetAllStorages() {
    std::lock_guard lock(s_mutex);
    PruneInvalid();

    std::vector<StorageAccessor> result;
    for (auto* ptr : s_storages) {
        StorageAccessor accessor(ptr);
        if (accessor.IsValid()) {
            result.push_back(std::move(accessor));
        }
    }
    return result;
}

bool StorageRegistry::HasStorages() {
    std::lock_guard lock(s_mutex);
    return !s_storages.empty();
}

void StorageRegistry::Clear() {
    std::lock_guard lock(s_mutex);
    s_storages.clear();
    s_lastOpened = nullptr;
    Logger::Debug("StorageRegistry: cleared all storages");
}

BSStorageComponent* StorageRegistry::GetLastOpened() {
    std::lock_guard lock(s_mutex);
    return s_lastOpened;
}

void StorageRegistry::PruneInvalid() {
    auto before = s_storages.size();
    std::erase_if(s_storages, [](BSStorageComponent* ptr) {
        return !Memory::IsValidPtr(ptr);
    });
    auto removed = before - s_storages.size();
    if (removed > 0) {
        Logger::Debug("StorageRegistry: pruned {} invalid pointers", removed);
    }
}

} // namespace StorageCraft
