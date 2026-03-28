#include "craft/craft_hook.h"
#include "craft/material_consumer.h"
#include "game/material_pool.h"
#include "game/game_types.h"
#include "core/hook_manager.h"
#include "core/memory.h"
#include "core/logger.h"
#include "config/mod_config.h"

#include <safetyhook.hpp>

namespace StorageCraft {

// ============================================================================
// AOB Patterns for crafting functions
//
// These patterns target CrimsonDesert.exe's crafting system in the
// BlackSpace Engine. They must be discovered via reverse engineering:
//
//   1. Set a breakpoint on item count changes during crafting
//   2. Trace back to the crafting confirmation handler
//   3. Identify the instruction that subtracts materials
//   4. Generate an AOB pattern from surrounding bytes
//
// The patterns below are PLACEHOLDERS. Replace with real AOBs after RE.
//
// Methodology reference: The player-status-modifier found its item-gain
// AOB ("49 01 4C 38 10") by watching for ADD instructions on item counts.
// For crafting, we need the corresponding SUB/DEC instruction.
// ============================================================================
namespace Patterns {
    // Crafting material consumption: the instruction that decrements
    // an item count during crafting. Expected form: sub [reg+offset], reg
    // Hook offset: +0 (hook at the sub instruction itself)
    //
    // PLACEHOLDER - needs real AOB from game binary
    constexpr const char* CraftConsume =
        "49 29 4C 38 10";  // sub [r8+rdi+10h], rcx (mirrors item-gain pattern)

    // Crafting availability check: the instruction that reads material
    // counts to determine if crafting is possible. We hook this to
    // inject our combined inventory+storage count.
    //
    // PLACEHOLDER - needs real AOB from game binary
    constexpr const char* CraftCheck =
        "49 8B 4C 38 10";  // mov rcx, [r8+rdi+10h] (read item count)
}

namespace HookOffsets {
    constexpr int CraftConsume = 0;
    constexpr int CraftCheck = 0;
}

// ============================================================================
// Hook state
// ============================================================================
static int s_consumeHookIndex = -1;
static int s_checkHookIndex = -1;
static std::atomic<int> s_sampleCount = 0;
constexpr int MAX_LOG_SAMPLES = 16;

// ============================================================================
// Mid-function hook: Material consumption
//
// At the consumption instruction, registers contain:
//   rcx = amount being subtracted
//   r8  = item table base pointer
//   rdi = slot index (item ID lookup)
//
// We intercept to:
//   1. Check if mod is enabled
//   2. If the item is available in inventory, let original consume happen
//   3. If inventory is short, reduce the consumption amount and consume
//      the remainder from storage via our MaterialConsumer
// ============================================================================
static void OnCraftConsume(SafetyHookContext& ctx) {
    if (!ModConfig::IsEnabled()) return;

    __try {
        auto amount = static_cast<int32_t>(ctx.rcx);
        auto* itemTableBase = reinterpret_cast<void*>(ctx.r8);
        auto slotIndex = static_cast<int32_t>(ctx.rdi);

        if (!Memory::IsValidPtr(itemTableBase) || amount <= 0) return;

        // Read the item ID from the slot
        // Item table layout: each entry is 16 bytes (BSItemEntry)
        // ItemId is at offset 0x00 within each entry
        auto* entry = reinterpret_cast<BSItemEntry*>(
            reinterpret_cast<uintptr_t>(itemTableBase) + slotIndex * sizeof(BSItemEntry) + 0x10
        );

        if (!Memory::IsValidPtr(entry)) return;
        int32_t itemId = entry->ItemId;
        int32_t currentCount = entry->Count;

        // If inventory has enough, let the original instruction handle it
        if (currentCount >= amount) {
            if (s_sampleCount < MAX_LOG_SAMPLES) {
                Logger::Debug("CraftConsume: item {} has {} in inventory (need {}), passing through",
                              itemId, currentCount, amount);
                s_sampleCount++;
            }
            return;
        }

        // Inventory is short - we need to pull from storage
        int32_t fromInventory = currentCount;  // Take all from inventory
        int32_t fromStorage = amount - fromInventory;

        // Modify rcx to only consume what's in inventory
        // The storage portion will be consumed separately
        ctx.rcx = static_cast<uint64_t>(fromInventory);

        // TODO: Consume fromStorage amount from linked storage containers
        // This requires resolving the player's linked storage list,
        // which needs the storage-pointer AOB to be discovered.
        //
        // For now, log what would happen:
        Logger::Info("CraftConsume: item {} - {} from inventory, {} from storage",
                     itemId, fromInventory, fromStorage);

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        // Swallow exceptions in hot-path hooks (matching CD mod conventions)
        static bool reported = false;
        if (!reported) {
            Logger::Error("CraftConsume: exception in hook callback");
            reported = true;
        }
    }
}

// ============================================================================
// Mid-function hook: Material availability check
//
// This fires when the crafting UI checks if the player has enough materials.
// We intercept to add storage counts to the returned value.
// ============================================================================
static void OnCraftCheck(SafetyHookContext& ctx) {
    if (!ModConfig::IsEnabled()) return;

    __try {
        // rcx will contain the item count read from inventory
        // We add the storage count to make the check pass
        auto inventoryCount = static_cast<int32_t>(ctx.rcx);
        auto* itemTableBase = reinterpret_cast<void*>(ctx.r8);
        auto slotIndex = static_cast<int32_t>(ctx.rdi);

        if (!Memory::IsValidPtr(itemTableBase)) return;

        // TODO: Look up storage count for this item and add it
        // ctx.rcx = inventoryCount + storageCount;
        //
        // This requires the storage accessor to be wired up with
        // real pointers from the game.

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        static bool reported = false;
        if (!reported) {
            Logger::Error("CraftCheck: exception in hook callback");
            reported = true;
        }
    }
}

// ============================================================================
// Install / Uninstall
// ============================================================================
void CraftHook::Install() {
    // Scan for crafting consumption instruction
    auto consumeAddr = Memory::PatternScanOffset(
        nullptr, Patterns::CraftConsume, HookOffsets::CraftConsume);
    if (consumeAddr) {
        s_consumeHookIndex = HookManager::AddMidHook(consumeAddr, OnCraftConsume, "CraftConsume");
    } else {
        Logger::Error("CraftHook: failed to find CraftConsume pattern");
    }

    // Scan for crafting check instruction
    auto checkAddr = Memory::PatternScanOffset(
        nullptr, Patterns::CraftCheck, HookOffsets::CraftCheck);
    if (checkAddr) {
        s_checkHookIndex = HookManager::AddMidHook(checkAddr, OnCraftCheck, "CraftCheck");
    } else {
        Logger::Error("CraftHook: failed to find CraftCheck pattern");
    }
}

void CraftHook::Uninstall() {
    if (s_consumeHookIndex >= 0) {
        HookManager::RemoveMidHook(s_consumeHookIndex);
        s_consumeHookIndex = -1;
    }
    if (s_checkHookIndex >= 0) {
        HookManager::RemoveMidHook(s_checkHookIndex);
        s_checkHookIndex = -1;
    }
    Logger::Info("CraftHook: uninstalled");
}

} // namespace StorageCraft
