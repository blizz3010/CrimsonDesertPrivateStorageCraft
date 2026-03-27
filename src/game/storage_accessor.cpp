#include "game/storage_accessor.h"
#include "core/memory.h"
#include "core/logger.h"

namespace StorageCraft {

// ============================================================================
// Offsets into UStorageContainer - update these per game patch
// ============================================================================
namespace Offsets {
    constexpr ptrdiff_t Storage_ItemArray     = 0x168; // TArray<FItemStack>
    constexpr ptrdiff_t Storage_OwnerActor    = 0x1A0; // AActor* (the container actor)
    constexpr ptrdiff_t Actor_RootComponent   = 0x198; // USceneComponent*
    constexpr ptrdiff_t SceneComp_WorldPos    = 0x140; // FVector
    constexpr ptrdiff_t Storage_LockFlag      = 0x1C8; // uint8_t (0 = unlocked, 1 = locked)

    // Player location - accessed via the local player controller -> pawn -> position
    // These are resolved at runtime via pattern scan; see GetPlayerPosition()
}

StorageAccessor::StorageAccessor(UStorageContainer* storage)
    : m_storage(storage) {}

bool StorageAccessor::IsValid() const {
    return m_storage != nullptr && GetItemArray() != nullptr;
}

int32_t StorageAccessor::GetItemCount(int32_t itemId) const {
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

bool StorageAccessor::ConsumeItem(int32_t itemId, int32_t amount) {
    if (!IsValid() || amount <= 0) return false;
    if (!m_locked) {
        Logger::Warn("Storage: attempted to consume without lock");
        return false;
    }

    auto* items = GetItemArray();

    // Verify sufficient quantity
    int32_t available = 0;
    for (int32_t i = 0; i < items->Count; i++) {
        if (items->Data[i].ItemId == itemId) {
            available += items->Data[i].Count;
        }
    }
    if (available < amount) return false;

    // Consume
    int32_t remaining = amount;
    for (int32_t i = 0; i < items->Count && remaining > 0; i++) {
        if (items->Data[i].ItemId != itemId) continue;

        int32_t take = std::min(items->Data[i].Count, remaining);
        items->Data[i].Count -= take;
        remaining -= take;

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

    FVector storagePos = GetStoragePosition();
    FVector playerPos = GetPlayerPosition();
    float distance = playerPos.DistanceTo(storagePos);

    Logger::Debug("Storage: distance to player = {:.1f} (max: {:.1f})", distance, maxDistance);
    return distance <= maxDistance;
}

bool StorageAccessor::TryLock() {
    if (!IsValid()) return false;

    // Check the game's own lock flag to detect if another player is accessing it
    auto* lockFlag = reinterpret_cast<uint8_t*>(
        reinterpret_cast<uintptr_t>(m_storage) + Offsets::Storage_LockFlag
    );

    // If the game-side flag is already set, another player is using it
    if (*lockFlag != 0) {
        Logger::Warn("Storage: container is locked by another player");
        return false;
    }

    m_locked = true;
    Logger::Debug("Storage: lock acquired");
    return true;
}

void StorageAccessor::Unlock() {
    m_locked = false;
    Logger::Debug("Storage: lock released");
}

std::vector<FItemStack> StorageAccessor::GetItems() const {
    std::vector<FItemStack> result;
    if (!IsValid()) return result;

    auto* items = GetItemArray();
    result.reserve(items->Count);
    for (int32_t i = 0; i < items->Count; i++) {
        result.push_back(items->Data[i]);
    }
    return result;
}

TArray<FItemStack>* StorageAccessor::GetItemArray() const {
    return reinterpret_cast<TArray<FItemStack>*>(
        reinterpret_cast<uintptr_t>(m_storage) + Offsets::Storage_ItemArray
    );
}

FVector StorageAccessor::GetStoragePosition() const {
    auto* ownerActor = Memory::ReadOffset<void*>(m_storage, Offsets::Storage_OwnerActor);
    if (!ownerActor) return {};

    auto* rootComponent = Memory::ReadOffset<void*>(ownerActor, Offsets::Actor_RootComponent);
    if (!rootComponent) return {};

    return Memory::ReadOffset<FVector>(rootComponent, Offsets::SceneComp_WorldPos);
}

FVector StorageAccessor::GetPlayerPosition() const {
    // TODO: Resolve via GEngine->GameViewport->GetWorld()->GetFirstPlayerController()->GetPawn()
    // For now, this uses a pattern-scanned cached pointer to the local player pawn.
    // The actual implementation will be filled in during the RE phase.
    return {};
}

} // namespace StorageCraft
