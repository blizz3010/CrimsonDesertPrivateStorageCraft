#pragma once

#include <cstdint>
#include <cmath>

namespace StorageCraft {

// ============================================================================
// Mirrored BlackSpace Engine structures
//
// Crimson Desert uses Pearl Abyss's proprietary BlackSpace Engine (NOT UE5).
// These structures are determined through reverse engineering and may need
// updating with each game patch. Offsets are defined as constexpr values
// in each accessor's .cpp file.
//
// Reference: Orcax-1399/CrimsonDesert-player-status-modifier for known
// patterns and structure discovery methodology.
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

// BlackSpace Engine item representation (discovered via RE)
// The engine uses a flat item table with ID + count pairs.
struct BSItemEntry {
    int32_t ItemId = 0;
    int32_t Count = 0;
    int32_t Quality = 0;
    int32_t Flags = 0;       // Bitfield: bound, tradeable, etc.
};

// BlackSpace dynamic array - similar concept to TArray but different layout.
// The engine stores: pointer to data, element count, allocated capacity.
// Memory layout discovered via pattern scanning the item-gain AOB.
template<typename T>
struct BSArray {
    T* Data = nullptr;
    int32_t Count = 0;
    int32_t Capacity = 0;
};

// Opaque engine object types - accessed via offset-based reads.
// We don't reconstruct full class layouts; we just read fields at
// known offsets discovered through AOB scanning + stepping in debugger.
struct BSPlayerComponent;    // Player status/inventory component
struct BSStorageComponent;   // Private storage container component
struct BSCraftingComponent;  // Crafting station interaction component
struct BSActor;              // Base actor in the world

// Recipe material requirement
struct BSMaterialRequirement {
    int32_t ItemId = 0;
    int32_t Amount = 0;
};

// Crafting recipe - layout from the crafting component's recipe table
struct BSCraftingRecipe {
    int32_t RecipeId = 0;
    int32_t ResultItemId = 0;
    int32_t ResultCount = 0;
    BSArray<BSMaterialRequirement> Materials;
};

// Player status marker - used to identify the local player's component.
// Discovered by the player-pointer AOB (see craft_hook.cpp).
// The marker is a unique value at a known offset in the component that
// distinguishes the local player from NPCs and other players.
struct BSPlayerMarker {
    uintptr_t statusMarker = 0; // Unique identifier for the local player
    void* componentPtr = nullptr;
};

} // namespace StorageCraft
