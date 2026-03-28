#include "ui/ui_hook.h"
#include "ui/storage_indicator.h"
#include "game/material_pool.h"
#include "game/storage_registry.h"
#include "game/game_types.h"
#include "core/hook_manager.h"
#include "core/memory.h"
#include "core/logger.h"
#include "config/mod_config.h"

#include <safetyhook.hpp>

namespace StorageCraft {

// ============================================================================
// AOB Patterns for crafting UI
//
// The BlackSpace Engine has its own UI rendering pipeline. We need to find:
//   1. The function that formats material count text for crafting slots
//   2. The instruction that writes the count value to the UI element
//
// Approach:
//   - Search for the text formatting function that converts item counts
//     to display strings in the crafting panel
//   - Alternatively, hook at the point where the crafting UI reads
//     inventory counts (which will be the same read the CraftCheck
//     pattern targets, but in the UI update path)
//
// PLACEHOLDER patterns - need discovery from game binary.
// ============================================================================
namespace Patterns {
    // UI material count update - the instruction that writes the
    // available count to a crafting slot's text element.
    // This pattern targets the BlackSpace UI text setter.
    //
    // PLACEHOLDER - needs real AOB
    constexpr const char* UICountUpdate =
        "89 44 24 ?? 48 8B ?? ?? 48 85 C0 74";

    // Crafting slot color/state update - determines if a slot shows
    // green (sufficient) or red (insufficient) based on material count.
    // We hook this to use our combined count for the color logic.
    //
    // PLACEHOLDER - needs real AOB
    constexpr const char* UISlotState =
        "3B C1 7C ?? 48 8B ?? ?? 44 8B";
}

static int s_countUpdateHookIndex = -1;
static int s_slotStateHookIndex = -1;

// ============================================================================
// Mid-function hook: UI count display
//
// At this instruction, a register holds the item count that will be
// displayed in the crafting UI. We replace it with the combined count.
// ============================================================================
static void OnUICountUpdate(SafetyHookContext& ctx) {
    if (!ModConfig::IsEnabled()) return;

    __try {
        // eax typically holds the count being written to the UI text.
        // We inflate it with the storage count so the crafting panel
        // shows the combined total.
        auto inventoryCount = static_cast<int32_t>(ctx.rax);

        // The item ID for the current slot should be in another register.
        // Common BlackSpace UI patterns pass item ID in edx or ecx.
        // This needs verification from the actual discovered AOB.
        auto itemId = static_cast<int32_t>(ctx.rdx);
        if (itemId <= 0) return;

        // Query storage for additional count
        if (!StorageRegistry::HasStorages()) return;

        float maxDist = ModConfig::Get().maxStorageDistance;
        auto storages = StorageRegistry::GetActiveStorages(maxDist);

        int32_t storageCount = 0;
        for (auto& storage : storages) {
            storageCount += storage.GetItemCount(itemId);
        }

        if (storageCount > 0) {
            // Replace the count register with combined total
            ctx.rax = static_cast<uint64_t>(inventoryCount + storageCount);

            // Track which slots use storage materials for the indicator
            if (ModConfig::Get().showStorageIcon) {
                // The slot index may be in r8 or another register depending
                // on the actual AOB. Using r8d as a common candidate.
                auto slotIndex = static_cast<int32_t>(ctx.r8);
                if (slotIndex >= 0) {
                    StorageIndicator::MarkSlotAsStorageSourced(slotIndex);
                }
            }

            Logger::Debug("UICountUpdate: item {} - inv:{} + storage:{} = {}",
                          itemId, inventoryCount, storageCount,
                          inventoryCount + storageCount);
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        static bool reported = false;
        if (!reported) {
            Logger::Error("UICountUpdate: exception in hook");
            reported = true;
        }
    }
}

// ============================================================================
// Mid-function hook: Slot state (green/red color)
//
// The game compares available count (eax/ecx) against required count.
// We ensure the comparison uses our combined count so slots show green
// when storage has enough materials even if inventory alone doesn't.
// ============================================================================
static void OnUISlotState(SafetyHookContext& ctx) {
    if (!ModConfig::IsEnabled()) return;

    __try {
        // cmp eax, ecx -> eax = available, ecx = required (or vice versa)
        // We inflate the "available" register with storage counts so the
        // slot shows green (sufficient) even when only storage fills the gap.
        auto available = static_cast<int32_t>(ctx.rax);
        auto required = static_cast<int32_t>(ctx.rcx);

        // If already sufficient, no need to check storage
        if (available >= required) return;

        if (!StorageRegistry::HasStorages()) return;

        // We need the item ID for this slot. The UI comparison function
        // typically has the item ID passed in a register or on the stack.
        // Using rdx as a candidate - needs verification from actual AOB.
        auto itemId = static_cast<int32_t>(ctx.rdx);
        if (itemId <= 0) return;

        float maxDist = ModConfig::Get().maxStorageDistance;
        auto storages = StorageRegistry::GetActiveStorages(maxDist);

        int32_t storageCount = 0;
        for (auto& storage : storages) {
            storageCount += storage.GetItemCount(itemId);
        }

        if (storageCount > 0) {
            ctx.rax = static_cast<uint64_t>(available + storageCount);
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        static bool reported = false;
        if (!reported) {
            Logger::Error("UISlotState: exception in hook");
            reported = true;
        }
    }
}

// ============================================================================
// Install / Uninstall
// ============================================================================
void UIHook::Install() {
    auto countAddr = Memory::PatternScan(nullptr, Patterns::UICountUpdate);
    if (countAddr) {
        s_countUpdateHookIndex = HookManager::AddMidHook(countAddr, OnUICountUpdate, "UICountUpdate");
    } else {
        Logger::Warn("UIHook: failed to find UICountUpdate pattern (UI overlay disabled)");
    }

    auto stateAddr = Memory::PatternScan(nullptr, Patterns::UISlotState);
    if (stateAddr) {
        s_slotStateHookIndex = HookManager::AddMidHook(stateAddr, OnUISlotState, "UISlotState");
    } else {
        Logger::Warn("UIHook: failed to find UISlotState pattern (slot coloring disabled)");
    }

    StorageIndicator::Init();
}

void UIHook::Uninstall() {
    if (s_countUpdateHookIndex >= 0) {
        HookManager::RemoveMidHook(s_countUpdateHookIndex);
        s_countUpdateHookIndex = -1;
    }
    if (s_slotStateHookIndex >= 0) {
        HookManager::RemoveMidHook(s_slotStateHookIndex);
        s_slotStateHookIndex = -1;
    }
    StorageIndicator::Shutdown();
    Logger::Info("UIHook: uninstalled");
}

} // namespace StorageCraft
