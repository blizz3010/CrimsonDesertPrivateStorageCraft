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

// BlackSpace item entry layout - confirmed by 3 independent sources:
//   1. player-status-modifier: item-gain AOB "49 01 4C 38 10" writes [r8+rdi+0x10]
//   2. Cheat Engine community: "item count is an 8 byte integer" at +0x10
//   3. CE table v2: distinguishes stackable items (Count >= 2) from equipment
//
// Item-loss confirmed: aobscanmodule(INJECT,CrimsonDesert.exe,49 29 4C 07 10)
// NOPing this instruction = items never decrease (selling, refining, crafting)
//
// Items use a dual-ID system: ItemNo (int32) + ItemKey (int32)
//   Example: Abyss Artifact = ItemNo:65, ItemKey:75002
//            Arrow = ItemNo:66, ItemKey:50001
//            Silver = ItemNo:21/22, ItemKey:1 (stack = copper cents, 100 = 1.00 silver)
//   Internal string codes: "item_currency_pywel_01" etc.
//
// Additional fields per item (from save editor ItemSaveData):
//   - Enchant level (0-10 for equipment)
//   - Endurance
//   - Sharpness
//   - ItemBuffs (binary format with flat2/flat1/rate stat types, 28 stat hashes)
//
// Inventory config stored in 0008/0.paz, InventoryInfo table:
//   _defaultSlotCount: uint16 (vanilla: 50)
//   _maxSlotCount: uint16 (vanilla: 240, hard limit: 65535)
//
// Private Storage (added Patch 1.00.03, March 22 2026):
//   - 240-slot shared/account-wide chest
//   - Located at Greymane Camp (behind Carl the Provisioner) or temporary lodgings
//   - Consumables stack up to 50 per slot; equipment uses 1 slot each
//   - Expandable to 999 slots via PAZ patching (Nexus mod #244)
struct BSItemEntry {
    int32_t ItemNo = 0;        // +0x00: Item type number (e.g., 65 = Abyss Artifact)
    int32_t ItemKey = 0;       // +0x04: Item key (e.g., 75002)
    int64_t Reserved = 0;      // +0x08: Metadata / quality / durability
    int64_t Count = 0;         // +0x10: Stack count (8-byte int, CONFIRMED by CE + ASI mod)
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
