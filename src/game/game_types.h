#pragma once

#include <cstdint>

namespace StorageCraft {

// ============================================================================
// Mirrored game structures
// These structs mirror the in-memory layout of Crimson Desert / UE5 types.
// Offsets are determined through reverse engineering and may need updating
// with each game patch. See offset constants in accessor .cpp files.
// ============================================================================

struct FVector {
    float X = 0.f;
    float Y = 0.f;
    float Z = 0.f;

    float DistanceTo(const FVector& other) const {
        float dx = X - other.X;
        float dy = Y - other.Y;
        float dz = Z - other.Z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
};

struct FItemStack {
    int32_t ItemId = 0;
    int32_t Count = 0;
    int32_t Quality = 0;      // 0 = normal, 1 = fine, 2 = superior, etc.
    float   Durability = 0.f; // -1 for non-durability items
};

// Mirrors UE5 TArray<T> in memory layout
template<typename T>
struct TArray {
    T* Data = nullptr;
    int32_t Count = 0;
    int32_t Max = 0;
};

// Opaque game object types - we access these via known offsets
// rather than fully reconstructing their class layouts.
struct UPlayerInventory;
struct UStorageContainer;
struct UCraftingComponent;
struct AActor;
struct APlayerCharacter;

// Recipe material requirement
struct FMaterialRequirement {
    int32_t ItemId = 0;
    int32_t Amount = 0;
};

// Crafting recipe definition
struct FCraftingRecipe {
    int32_t RecipeId = 0;
    int32_t ResultItemId = 0;
    int32_t ResultCount = 0;
    TArray<FMaterialRequirement> Materials;
};

} // namespace StorageCraft
