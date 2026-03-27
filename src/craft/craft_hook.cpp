#include "craft/craft_hook.h"
#include "craft/material_consumer.h"
#include "game/material_pool.h"
#include "game/game_types.h"
#include "core/hook_manager.h"
#include "core/memory.h"
#include "core/logger.h"
#include "config/mod_config.h"

namespace StorageCraft {

// ============================================================================
// Pattern signatures - update these per game patch
// ============================================================================
namespace Patterns {
    // UCraftingComponent::ExecuteCraft(FCraftingRecipe*)
    constexpr const char* ExecuteCraft =
        "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 48 8B F2 48 8B F9";
}

namespace Offsets {
    // UCraftingComponent offsets
    constexpr ptrdiff_t CraftComp_PlayerInventory = 0x120; // UPlayerInventory*
    constexpr ptrdiff_t CraftComp_LinkedStorages  = 0x138; // TArray<UStorageContainer*>
}

// ============================================================================
// Original function pointer (trampoline)
// ============================================================================
using ExecuteCraftFn = bool(*)(UCraftingComponent* comp, FCraftingRecipe* recipe);
static ExecuteCraftFn Original_ExecuteCraft = nullptr;

// ============================================================================
// Detour implementation
// ============================================================================
static bool Detour_ExecuteCraft(UCraftingComponent* comp, FCraftingRecipe* recipe) {
    // If mod is disabled, pass through to original
    if (!ModConfig::IsEnabled()) {
        return Original_ExecuteCraft(comp, recipe);
    }

    Logger::Debug("CraftHook: intercepted craft for recipe {}", recipe->RecipeId);

    // Get player inventory
    auto* rawInventory = Memory::ReadOffset<UPlayerInventory*>(
        comp, Offsets::CraftComp_PlayerInventory);
    InventoryAccessor inventory(rawInventory);

    if (!inventory.IsValid()) {
        Logger::Warn("CraftHook: invalid inventory pointer, falling through");
        return Original_ExecuteCraft(comp, recipe);
    }

    // Get linked storage containers
    auto storageArray = Memory::ReadOffset<TArray<UStorageContainer*>>(
        comp, Offsets::CraftComp_LinkedStorages);

    std::vector<StorageAccessor> storages;
    float maxDistance = ModConfig::Get().maxStorageDistance;

    for (int32_t i = 0; i < storageArray.Count; i++) {
        StorageAccessor accessor(storageArray.Data[i]);
        if (accessor.IsValid() && accessor.IsInRange(maxDistance)) {
            storages.push_back(std::move(accessor));
        }
    }

    // Build the material pool
    MaterialPool pool(inventory, storages);

    // Plan consumption for all recipe materials
    ConsumptionPlan plan;
    bool canCraft = true;

    for (int32_t i = 0; i < recipe->Materials.Count; i++) {
        const auto& req = recipe->Materials.Data[i];
        auto source = pool.PlanConsumption(req.ItemId, req.Amount);

        if (!source.has_value()) {
            Logger::Debug("CraftHook: insufficient material {} (need {})",
                          req.ItemId, req.Amount);
            canCraft = false;
            break;
        }

        plan.entries.push_back({req.ItemId, source->fromInventory, source->fromStorage});
    }

    // If we can't fulfill with combined sources, let the game handle it normally
    if (!canCraft) {
        return Original_ExecuteCraft(comp, recipe);
    }

    // Lock all storage containers we'll consume from
    bool allLocked = true;
    for (auto& storage : storages) {
        if (!storage.TryLock()) {
            allLocked = false;
            break;
        }
    }

    if (!allLocked) {
        // Unlock any we did lock
        for (auto& storage : storages) {
            if (storage.IsLocked()) storage.Unlock();
        }
        Logger::Warn("CraftHook: could not lock all storages, falling through to original");
        return Original_ExecuteCraft(comp, recipe);
    }

    // Execute the consumption plan
    bool consumed = MaterialConsumer::Execute(plan, inventory, storages);

    // Unlock all storages
    for (auto& storage : storages) {
        storage.Unlock();
    }

    if (!consumed) {
        Logger::Error("CraftHook: consumption failed unexpectedly");
        return Original_ExecuteCraft(comp, recipe);
    }

    // Call the original function - the materials have already been consumed,
    // so we need the original to handle the result item creation.
    // The original will see the depleted inventory and proceed.
    Logger::Info("CraftHook: craft succeeded with storage materials for recipe {}",
                 recipe->RecipeId);
    return Original_ExecuteCraft(comp, recipe);
}

// ============================================================================
// Install / Uninstall
// ============================================================================
static void* s_targetAddr = nullptr;

void CraftHook::Install() {
    auto addr = Memory::PatternScan(nullptr, Patterns::ExecuteCraft);
    if (!addr) {
        Logger::Error("CraftHook: failed to find ExecuteCraft pattern");
        return;
    }

    s_targetAddr = reinterpret_cast<void*>(addr);
    Original_ExecuteCraft = HookManager::Hook<ExecuteCraftFn>(
        s_targetAddr, &Detour_ExecuteCraft);

    if (Original_ExecuteCraft) {
        Logger::Info("CraftHook: installed successfully");
    } else {
        Logger::Error("CraftHook: failed to install hook");
    }
}

void CraftHook::Uninstall() {
    if (s_targetAddr) {
        HookManager::Unhook(s_targetAddr);
        s_targetAddr = nullptr;
        Original_ExecuteCraft = nullptr;
        Logger::Info("CraftHook: uninstalled");
    }
}

} // namespace StorageCraft
