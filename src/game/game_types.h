#pragma once

#include <cstdint>
#include <cmath>
#include <atomic>

namespace StorageCraft {

// ============================================================================
// BlackSpace Engine structures - Crimson Desert
//
// These are based on VERIFIED patterns and offsets from:
//   Orcax-1399/CrimsonDesert-player-status-modifier (working mod)
//
// Key discoveries from that mod:
//   - Item count field is at +0x10 within each item entry
//   - Item gain instruction: add [r8+rdi+0x10], rcx  (49 01 4C 38 10)
//   - Item loss instruction: sub [r15+rax+0x10], rcx  (49 29 4C 07 10)
//   - Player component found via: owner(rax) +0x20 -> component
//   - Component vtable/marker at +0x00 identifies the player
//   - Stat/data table at component +0x58
//   - Entries are 16-byte aligned (shl rax, 4)
// ============================================================================

struct FVector3 {
    float X = 0.f;
    float Y = 0.f;
    float Z = 0.f;

    float DistanceTo(const FVector3& other) const {
        float dx = X - other.X;
        float dy = Y - other.Y;
        float dz = Z - other.Z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
};

// BlackSpace item entry - 16-byte aligned (verified by shl rax, 4 in stat access pattern).
// The item-gain AOB writes to [base + index + 0x10], confirming Count is at +0x10.
// Fields at 0x00-0x0F are the item identifier / metadata.
struct BSItemEntry {
    int32_t ItemId = 0;        // +0x00: Item type identifier
    int32_t Flags = 0;         // +0x04: Bitfield (bound, tradeable, etc.)
    int64_t Reserved = 0;      // +0x08: Padding / quality / durability
    int64_t Count = 0;         // +0x10: Stack count (verified: item-gain writes here)
    int64_t MaxCount = 0;      // +0x18: Max stack size (mirrors stat entry layout)
};
// Note: actual entry size may be 32 bytes (0x20) per entry based on the
// stat entry layout. The stat entries use: type(+0x00), value(+0x08), max(+0x18)
// with 16-byte alignment via shl rax, 4. Item entries likely follow a similar
// but potentially wider layout. Needs verification with debugger.

// BlackSpace dynamic array header (pointer + count + capacity).
template<typename T>
struct BSArray {
    T* Data = nullptr;         // +0x00
    int32_t Count = 0;         // +0x08
    int32_t Capacity = 0;      // +0x0C
};

// Opaque engine types - accessed via offset-based reads
struct BSPlayerComponent;      // Player status/inventory component
struct BSStorageComponent;     // Private storage container component
struct BSCraftingComponent;    // Crafting station component
struct BSActor;                // Base actor

// Recipe material requirement
struct BSMaterialRequirement {
    int32_t ItemId = 0;
    int32_t Amount = 0;
};

struct BSCraftingRecipe {
    int32_t RecipeId = 0;
    int32_t ResultItemId = 0;
    int32_t ResultCount = 0;
    BSArray<BSMaterialRequirement> Materials;
};

// Player marker - identifies the local player's component at runtime.
// The player-status-modifier uses *(component + 0x00) as a unique marker
// (likely the vtable pointer or type ID) and stores it for comparison in hooks.
struct PlayerState {
    std::atomic<uintptr_t> statusMarker{0};    // *(component + 0x00)
    std::atomic<uintptr_t> componentPtr{0};     // The component pointer itself
    std::atomic<uintptr_t> ownerPtr{0};         // The owner actor pointer

    void Reset() {
        statusMarker = 0;
        componentPtr = 0;
        ownerPtr = 0;
    }

    bool IsValid() const {
        return statusMarker.load() != 0 && componentPtr.load() > 0x10000000;
    }

    bool IsPlayerComponent(void* comp) const {
        if (!comp || reinterpret_cast<uintptr_t>(comp) < 0x10000000) return false;
        auto marker = *reinterpret_cast<uintptr_t*>(comp);
        return marker == statusMarker.load();
    }
};

// Global player state - populated by the player-pointer hook
inline PlayerState g_playerState;

} // namespace StorageCraft
