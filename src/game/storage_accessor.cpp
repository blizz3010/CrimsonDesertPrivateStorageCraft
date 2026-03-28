#include "game/storage_accessor.h"
#include "core/memory.h"
#include "core/logger.h"

namespace StorageCraft {

// ============================================================================
// BlackSpace Engine offsets into BSStorageComponent
//
// Storage containers in Crimson Desert are world actors with an associated
// component that holds the item array. The structure is similar to the
// player inventory but at different offsets.
//
// TODO: These need verification via RE. Approach:
//   1. Find the storage UI open function via AOB
//   2. Trace the component pointer to find the item array offset
//   3. Find the actor position via the owner actor chain
// ============================================================================
namespace Offsets {
    constexpr ptrdiff_t Storage_ItemArray     = 0x168;
    constexpr ptrdiff_t Storage_OwnerActor    = 0x1A0;
    constexpr ptrdiff_t Storage_LockFlag      = 0x1C8; // uint8: 0=unlocked, 1=in-use

    // BlackSpace actor position chain
    constexpr ptrdiff_t Actor_Transform       = 0x198;
    constexpr ptrdiff_t Transform_Position    = 0x10;  // FVector3 within transform
}

StorageAccessor::StorageAccessor(BSStorageComponent* storage)
    : m_storage(storage) {}

bool StorageAccessor::IsValid() const {
    if (!Memory::IsValidPtr(m_storage)) return false;
    auto* items = GetItemArray();
    return items != nullptr && Memory::IsValidPtr(items->Data);
}

int32_t StorageAccessor::GetItemCount(int32_t itemId) const {
    if (!IsValid()) return 0;

    auto* items = GetItemArray();
    int64_t total = 0;
    for (int32_t i = 0; i < items->Count; i++) {
        if (items->Data[i].ItemNo == itemId) {
            total += items->Data[i].Count;
        }
    }
    return static_cast<int32_t>(std::min(total, static_cast<int64_t>(INT32_MAX)));
}

bool StorageAccessor::ConsumeItem(int32_t itemId, int32_t amount) {
    if (!IsValid() || amount <= 0) return false;
    if (!m_locked) {
        Logger::Warn("Storage: attempted consume without lock");
        return false;
    }

    auto* items = GetItemArray();

    int64_t available = 0;
    for (int32_t i = 0; i < items->Count; i++) {
        if (items->Data[i].ItemNo == itemId) available += items->Data[i].Count;
    }
    if (available < amount) return false;

    int32_t remaining = amount;
    for (int32_t i = 0; i < items->Count && remaining > 0; i++) {
        if (items->Data[i].ItemNo != itemId) continue;

        auto take = std::min(items->Data[i].Count, static_cast<int64_t>(remaining));
        items->Data[i].Count -= take;
        remaining -= static_cast<int32_t>(take);

        if (items->Data[i].Count <= 0) {
            items->Data[i] = items->Data[items->Count - 1];
            items->Count--;
            i--;
        }
    }

    Logger::Debug("Storage: consumed {}x item {}", amount, itemId);
    return true;
}

bool StorageAccessor::IsInRange(float maxDistance) const {
    if (!IsValid()) return false;

    FVector3 storagePos = GetStoragePosition();
    FVector3 playerPos = GetPlayerPosition();
    float distance = playerPos.DistanceTo(storagePos);

    return distance <= maxDistance;
}

bool StorageAccessor::TryLock() {
    if (!IsValid()) return false;

    // Check the game's in-use flag (set when another player opens the container)
    auto* lockFlag = reinterpret_cast<uint8_t*>(
        reinterpret_cast<uintptr_t>(m_storage) + Offsets::Storage_LockFlag
    );
    if (*lockFlag != 0) {
        Logger::Warn("Storage: container locked by another player");
        return false;
    }

    m_locked = true;
    return true;
}

void StorageAccessor::Unlock() {
    m_locked = false;
}

std::vector<BSItemEntry> StorageAccessor::GetItems() const {
    std::vector<BSItemEntry> result;
    if (!IsValid()) return result;

    auto* items = GetItemArray();
    result.reserve(items->Count);
    for (int32_t i = 0; i < items->Count; i++) {
        result.push_back(items->Data[i]);
    }
    return result;
}

BSArray<BSItemEntry>* StorageAccessor::GetItemArray() const {
    return reinterpret_cast<BSArray<BSItemEntry>*>(
        reinterpret_cast<uintptr_t>(m_storage) + Offsets::Storage_ItemArray
    );
}

FVector3 StorageAccessor::GetStoragePosition() const {
    auto* ownerActor = Memory::ReadOffset<void*>(m_storage, Offsets::Storage_OwnerActor);
    if (!Memory::IsValidPtr(ownerActor)) return {};

    auto* transform = Memory::ReadOffset<void*>(ownerActor, Offsets::Actor_Transform);
    if (!Memory::IsValidPtr(transform)) return {};

    return Memory::ReadOffset<FVector3>(transform, Offsets::Transform_Position);
}

FVector3 StorageAccessor::GetPlayerPosition() const {
    // Resolve player position via the captured player state.
    // The player-pointer hook populates g_playerState.ownerPtr which is
    // the owner actor. We walk: owner -> transform -> position using
    // the same Actor_Transform and Transform_Position offsets.
    auto ownerAddr = g_playerState.ownerPtr.load();
    if (ownerAddr == 0 || !Memory::IsValidPtr(ownerAddr)) return {};

    auto* owner = reinterpret_cast<void*>(ownerAddr);
    auto* transform = Memory::ReadOffset<void*>(owner, Offsets::Actor_Transform);
    if (!Memory::IsValidPtr(transform)) return {};

    return Memory::ReadOffset<FVector3>(transform, Offsets::Transform_Position);
}

} // namespace StorageCraft
