#include "ui/ui_hook.h"
#include "ui/storage_indicator.h"
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
    // UCraftingWidget::UpdateMaterialSlot(int32 SlotIndex, int32 ItemId, int32 Count, int32 Required)
    constexpr const char* UpdateMaterialSlot =
        "40 53 48 83 EC 20 44 8B C2 48 8B D9 41 8B D0";

    // UCraftingWidget::OnRecipeSelected(FCraftingRecipe*)
    constexpr const char* OnRecipeSelected =
        "48 89 5C 24 10 48 89 74 24 18 55 48 8D AC 24";
}

namespace Offsets {
    constexpr ptrdiff_t CraftWidget_PlayerInventory = 0x2B0; // UPlayerInventory*
    constexpr ptrdiff_t CraftWidget_LinkedStorages  = 0x2C8; // TArray<UStorageContainer*>
    constexpr ptrdiff_t CraftWidget_MaterialSlots   = 0x310; // TArray<UWidget*>
}

// ============================================================================
// UpdateMaterialSlot hook
// ============================================================================
// Original signature: void UpdateMaterialSlot(UCraftingWidget*, int32 slotIndex, int32 itemId, int32 count, int32 required)
using UpdateMaterialSlotFn = void(*)(void* widget, int32_t slotIndex, int32_t itemId, int32_t count, int32_t required);
static UpdateMaterialSlotFn Original_UpdateMaterialSlot = nullptr;

static void Detour_UpdateMaterialSlot(void* widget, int32_t slotIndex, int32_t itemId, int32_t count, int32_t required) {
    if (!ModConfig::IsEnabled()) {
        Original_UpdateMaterialSlot(widget, slotIndex, itemId, count, required);
        return;
    }

    // Get inventory and storage from the crafting widget
    auto* rawInventory = Memory::ReadOffset<UPlayerInventory*>(
        widget, Offsets::CraftWidget_PlayerInventory);
    InventoryAccessor inventory(rawInventory);

    auto storageArray = Memory::ReadOffset<TArray<UStorageContainer*>>(
        widget, Offsets::CraftWidget_LinkedStorages);

    std::vector<StorageAccessor> storages;
    float maxDistance = ModConfig::Get().maxStorageDistance;

    for (int32_t i = 0; i < storageArray.Count; i++) {
        StorageAccessor accessor(storageArray.Data[i]);
        if (accessor.IsValid() && accessor.IsInRange(maxDistance)) {
            storages.push_back(std::move(accessor));
        }
    }

    MaterialPool pool(inventory, storages);

    // Replace the count with the combined total
    int32_t combinedCount = pool.GetTotalCount(itemId);

    // Call original with the modified count
    Original_UpdateMaterialSlot(widget, slotIndex, itemId, combinedCount, required);

    // Show/hide storage indicator icon based on whether storage is needed
    if (ModConfig::Get().showStorageIcon && pool.RequiresStorage(itemId, required)) {
        // Get the widget for this material slot to attach the icon
        auto slotsArray = Memory::ReadOffset<TArray<void*>>(
            widget, Offsets::CraftWidget_MaterialSlots);
        if (slotIndex >= 0 && slotIndex < slotsArray.Count) {
            StorageIndicator::Show(slotsArray.Data[slotIndex]);
        }
    } else {
        auto slotsArray = Memory::ReadOffset<TArray<void*>>(
            widget, Offsets::CraftWidget_MaterialSlots);
        if (slotIndex >= 0 && slotIndex < slotsArray.Count) {
            StorageIndicator::Hide(slotsArray.Data[slotIndex]);
        }
    }

    Logger::Debug("UIHook: slot {} item {} count {} -> {} (required: {})",
                  slotIndex, itemId, count, combinedCount, required);
}

// ============================================================================
// Install / Uninstall
// ============================================================================
static void* s_updateSlotTarget = nullptr;

void UIHook::Install() {
    auto addr = Memory::PatternScan(nullptr, Patterns::UpdateMaterialSlot);
    if (!addr) {
        Logger::Error("UIHook: failed to find UpdateMaterialSlot pattern");
        return;
    }

    s_updateSlotTarget = reinterpret_cast<void*>(addr);
    Original_UpdateMaterialSlot = HookManager::Hook<UpdateMaterialSlotFn>(
        s_updateSlotTarget, &Detour_UpdateMaterialSlot);

    if (Original_UpdateMaterialSlot) {
        Logger::Info("UIHook: installed UpdateMaterialSlot hook");
    } else {
        Logger::Error("UIHook: failed to install UpdateMaterialSlot hook");
    }

    // Initialize the storage indicator overlay system
    StorageIndicator::Init();
}

void UIHook::Uninstall() {
    if (s_updateSlotTarget) {
        HookManager::Unhook(s_updateSlotTarget);
        s_updateSlotTarget = nullptr;
        Original_UpdateMaterialSlot = nullptr;
    }
    Logger::Info("UIHook: uninstalled");
}

} // namespace StorageCraft
