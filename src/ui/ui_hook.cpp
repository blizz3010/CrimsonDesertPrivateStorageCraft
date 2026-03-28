#include "ui/ui_hook.h"
#include "ui/storage_indicator.h"
#include "game/material_pool.h"
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
        // eax typically holds the count being written to the UI text
        auto inventoryCount = static_cast<int32_t>(ctx.rax);

        // TODO: Look up the item ID for this slot and add storage count
        // For now, this is a passthrough until storage pointers are resolved
        // int32_t storageCount = GetStorageCountForCurrentSlot();
        // ctx.rax = inventoryCount + storageCount;

        // When storage contributes, trigger the indicator
        // if (storageCount > 0 && ModConfig::Get().showStorageIcon) {
        //     StorageIndicator::MarkSlotAsStorageSourced(currentSlotIndex);
        // }

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
        // We inflate the "available" register with storage counts

        // TODO: Add storage count to the available register
        // auto available = static_cast<int32_t>(ctx.rax);
        // ctx.rax = available + storageCount;

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
