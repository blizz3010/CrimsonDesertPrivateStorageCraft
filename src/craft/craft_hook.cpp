#include "craft/craft_hook.h"
#include "craft/material_consumer.h"
#include "game/material_pool.h"
#include "game/storage_registry.h"
#include "game/game_types.h"
#include "core/hook_manager.h"
#include "core/memory.h"
#include "core/logger.h"
#include "config/mod_config.h"

#include <safetyhook.hpp>
#include <mutex>

namespace StorageCraft {

// ============================================================================
// AOB Patterns - sourced from Orcax-1399/CrimsonDesert-player-status-modifier
//
// The player-status-modifier is the only open-source CD mod with verified,
// working AOB patterns. We reuse its player-pointer and item-gain patterns
// directly, and derive our crafting hooks from the item-loss pattern.
// ============================================================================
namespace Patterns {
    // -----------------------------------------------------------------------
    // VERIFIED: Player-pointer capture (from player-status-modifier)
    // Fires when the game accesses the player's status component.
    // We use this to capture the player component pointer and marker.
    //
    // Original: 0F B6 ? ? 8B ? ? 48 8B 58 40 48 8B 43 08 ? 8D ? 08 33 ? ? 85 ? ? 0F 44
    // Hook at +7: "48 8B 58 40" = mov rbx, [rax+0x40]
    // At this point: rax = owner pointer
    //   -> *(owner + 0x20) = component pointer
    //   -> *(component + 0x00) = status marker (vtable/type ID)
    // -----------------------------------------------------------------------
    constexpr const char* PlayerPointer =
        "0F B6 ?? ?? 8B ?? ?? 48 8B 58 40 48 8B 43 08 ?? 8D ?? 08 33 ?? ?? 85 ?? ?? 0F 44";
    constexpr int PlayerPointer_HookOffset = 7;

    // -----------------------------------------------------------------------
    // VERIFIED: Item-gain (from player-status-modifier)
    // Instruction: add [r8+rdi+0x10], rcx
    // Decoded: 49 01 4C 38 10
    //
    // At this point:
    //   r8  = item table base pointer
    //   rdi = slot/entry offset
    //   rcx = amount being added
    //   [r8+rdi+0x10] = the Count field of the item entry
    // -----------------------------------------------------------------------
    constexpr const char* ItemGain =
        "49 01 4C 38 10";
    constexpr int ItemGain_HookOffset = 0;

    // -----------------------------------------------------------------------
    // VERIFIED: Item-loss (from player-status-modifier, documented but unhooked)
    // Instruction: sub [r15+rax+0x10], rcx
    // Decoded: 49 29 4C 07 10
    //
    // At this point:
    //   r15 = item table base pointer
    //   rax = slot/entry offset
    //   rcx = amount being subtracted
    //   [r15+rax+0x10] = the Count field of the item entry
    //
    // NOTE: Different registers than item-gain! (r15+rax vs r8+rdi)
    // -----------------------------------------------------------------------
    constexpr const char* ItemLoss =
        "49 29 4C 07 10";
    constexpr int ItemLoss_HookOffset = 0;

    // -----------------------------------------------------------------------
    // DISCOVERY NEEDED: Storage container open
    //
    // When a player opens a private storage container, the game loads its
    // component into memory. We hook this to capture the BSStorageComponent
    // pointer for later use during crafting.
    //
    // Approach to find this pattern:
    //   1. Open a storage container in-game
    //   2. Set a data breakpoint on a known item count in the container
    //   3. Add/remove items to trigger the breakpoint
    //   4. Trace the code back to the function that loads the component
    //   5. AOB the surrounding bytes with register wildcards
    //
    // The storage component pointer will be in a register at hook time.
    // PLACEHOLDER - needs discovery from game binary via debugger.
    // -----------------------------------------------------------------------
    constexpr const char* StorageOpen =
        "48 8B ?? ?? ?? 48 85 ?? 74 ?? 48 8B ?? ?? 48 89 ?? ?? ?? 48 83";
    constexpr int StorageOpen_HookOffset = 0;
}

// ============================================================================
// Hook indices and state
// ============================================================================
static int s_playerPtrHookIdx = -1;
static int s_itemGainHookIdx = -1;
static int s_itemLossHookIdx = -1;
static int s_storageOpenHookIdx = -1;

static std::mutex s_stateMutex;
static std::atomic<int> s_ptrSamples = 0;
static std::atomic<int> s_gainSamples = 0;
static std::atomic<int> s_lossSamples = 0;
static std::atomic<int> s_storageOpenSamples = 0;
constexpr int MAX_LOG_SAMPLES = 24; // Match player-status-modifier convention

// ============================================================================
// Hook: Player-pointer capture
//
// This is the foundation hook - it captures the player's component pointer
// and status marker so other hooks can identify player-owned data.
// Directly adapted from player-status-modifier's PlayerPointerCallback.
// ============================================================================
static void OnPlayerPointer(SafetyHookContext& ctx) {
    __try {
        auto owner = reinterpret_cast<void*>(ctx.rax);
        if (!Memory::IsValidPtr(owner)) return;

        // Walk: owner + 0x20 -> component
        auto* componentPtr = Memory::ReadOffset<void*>(owner, 0x20);
        if (!Memory::IsValidPtr(componentPtr)) return;

        // Read marker at component + 0x00 (vtable / type ID)
        auto marker = *reinterpret_cast<uintptr_t*>(componentPtr);
        if (marker == 0) return;

        // Only capture once (or if component changed, e.g., respawn)
        auto currentMarker = g_playerState.statusMarker.load();
        if (currentMarker == 0 || currentMarker != marker) {
            std::lock_guard lock(s_stateMutex);
            g_playerState.statusMarker.store(marker);
            g_playerState.componentPtr.store(reinterpret_cast<uintptr_t>(componentPtr));
            g_playerState.ownerPtr.store(reinterpret_cast<uintptr_t>(owner));

            Logger::Info("PlayerPointer: captured component={:X} marker={:X}",
                         reinterpret_cast<uintptr_t>(componentPtr), marker);
        }

        if (s_ptrSamples < MAX_LOG_SAMPLES) {
            Logger::Debug("PlayerPointer: rax={:X} rsi={:X} rdx={:X}",
                          ctx.rax, ctx.rsi, ctx.rdx);
            s_ptrSamples++;
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        static bool reported = false;
        if (!reported) {
            Logger::Error("PlayerPointer: exception in hook");
            reported = true;
        }
    }
}

// ============================================================================
// Hook: Item-gain intercept
//
// Fires on: add [r8+rdi+0x10], rcx
// We use this primarily for monitoring and to capture the item table
// base pointer (r8) for later use by the crafting system.
// ============================================================================
static std::atomic<uintptr_t> s_lastItemTableBase{0};

static void OnItemGain(SafetyHookContext& ctx) {
    __try {
        auto* itemTableBase = reinterpret_cast<void*>(ctx.r8);
        auto slotOffset = ctx.rdi;
        auto amount = static_cast<int64_t>(ctx.rcx);

        if (!Memory::IsValidPtr(itemTableBase)) return;

        // Cache the item table base for use by other hooks
        s_lastItemTableBase.store(ctx.r8);

        if (s_gainSamples < MAX_LOG_SAMPLES) {
            // Read the current count at [r8+rdi+0x10] before the add
            auto* countPtr = reinterpret_cast<int64_t*>(ctx.r8 + ctx.rdi + 0x10);
            int64_t currentCount = Memory::IsValidPtr(countPtr) ? *countPtr : -1;

            // Read item ID at [r8+rdi+0x00] (first field of entry)
            auto* idPtr = reinterpret_cast<int32_t*>(ctx.r8 + ctx.rdi);
            int32_t itemId = Memory::IsValidPtr(idPtr) ? *idPtr : -1;

            Logger::Debug("ItemGain: r8={:X} rdi={:X} rcx={} itemId={} count={} -> {}",
                          ctx.r8, ctx.rdi, amount, itemId, currentCount, currentCount + amount);
            s_gainSamples++;
        }

        // If mod is enabled, we could apply a gain multiplier here
        // (like the player-status-modifier does). For now, passthrough.

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        static bool reported = false;
        if (!reported) {
            Logger::Error("ItemGain: exception in hook");
            reported = true;
        }
    }
}

// ============================================================================
// Hook: Item-loss intercept (CORE CRAFTING HOOK)
//
// Fires on: sub [r15+rax+0x10], rcx
// This is where materials are consumed during crafting.
//
// When StorageCraft is enabled:
//   1. Read the item entry at [r15+rax] to get item ID and current count
//   2. If inventory has enough, let original sub proceed
//   3. If inventory is short, reduce rcx to only consume what's available
//      in inventory, and consume the remainder from storage
// ============================================================================
static void OnItemLoss(SafetyHookContext& ctx) {
    if (!ModConfig::IsEnabled()) return;

    __try {
        auto* itemTableBase = reinterpret_cast<void*>(ctx.r15);
        auto slotOffset = ctx.rax;
        auto amount = static_cast<int64_t>(ctx.rcx);

        if (!Memory::IsValidPtr(itemTableBase) || amount <= 0) return;

        // Read current count at [r15+rax+0x10]
        auto* countPtr = reinterpret_cast<int64_t*>(ctx.r15 + ctx.rax + 0x10);
        if (!Memory::IsValidPtr(countPtr)) return;
        int64_t currentCount = *countPtr;

        // Read item ID at [r15+rax+0x00]
        auto* idPtr = reinterpret_cast<int32_t*>(ctx.r15 + ctx.rax);
        if (!Memory::IsValidPtr(idPtr)) return;
        int32_t itemId = *idPtr;

        if (s_lossSamples < MAX_LOG_SAMPLES) {
            Logger::Debug("ItemLoss: r15={:X} rax={:X} rcx={} itemId={} count={}",
                          ctx.r15, ctx.rax, amount, itemId, currentCount);
            s_lossSamples++;
        }

        // If inventory has enough, let the game handle it normally
        if (currentCount >= amount) {
            return;
        }

        // ============================================================
        // STORAGE CRAFTING LOGIC
        //
        // Inventory doesn't have enough. Consume what we can from
        // inventory, and pull the rest from storage.
        // ============================================================

        // Check if we have any registered storage containers
        if (!StorageRegistry::HasStorages()) {
            Logger::Debug("ItemLoss: no storage registered, letting game handle shortage");
            return;
        }

        int64_t fromInventory = currentCount;  // Take everything in inventory
        int64_t fromStorage = amount - fromInventory;

        // Get storage containers within range
        float maxDist = ModConfig::Get().maxStorageDistance;
        auto storages = StorageRegistry::GetActiveStorages(maxDist);

        if (storages.empty()) {
            Logger::Debug("ItemLoss: no storage in range (max={}), skipping", maxDist);
            return;
        }

        // Verify storage has enough of this item
        int64_t storageAvailable = 0;
        for (auto& storage : storages) {
            storageAvailable += storage.GetItemCount(itemId);
        }

        if (storageAvailable < fromStorage) {
            Logger::Debug("ItemLoss: storage only has {} of item {} (need {})",
                          storageAvailable, itemId, fromStorage);
            return;
        }

        // Modify rcx to only subtract what's in inventory.
        // The original instruction will execute: sub [r15+rax+0x10], rcx
        // By reducing rcx, we only consume the inventory portion.
        ctx.rcx = static_cast<uint64_t>(fromInventory);

        // Consume the remainder from storage containers.
        // Lock, consume, unlock - each storage in order.
        int64_t remaining = fromStorage;
        for (auto& storage : storages) {
            if (remaining <= 0) break;

            if (!storage.TryLock()) {
                Logger::Warn("ItemLoss: storage {:X} locked, skipping",
                             reinterpret_cast<uintptr_t>(storage.GetRaw()));
                continue;
            }

            int32_t available = storage.GetItemCount(itemId);
            if (available > 0) {
                auto take = std::min(static_cast<int64_t>(available), remaining);
                if (storage.ConsumeItem(itemId, static_cast<int32_t>(take))) {
                    remaining -= take;
                    Logger::Debug("ItemLoss: consumed {} of item {} from storage {:X}",
                                  take, itemId, reinterpret_cast<uintptr_t>(storage.GetRaw()));
                }
            }

            storage.Unlock();
        }

        if (remaining > 0) {
            // Partial failure - we already modified rcx so inventory portion
            // will be consumed. Log the shortage. This shouldn't happen if
            // validation was correct, but handle gracefully.
            Logger::Warn("ItemLoss: short by {} of item {} from storage", remaining, itemId);
        }

        Logger::Info("ItemLoss: item {} - consumed {} from inventory, {} from storage",
                     itemId, fromInventory, fromStorage - remaining);

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        static bool reported = false;
        if (!reported) {
            Logger::Error("ItemLoss: exception in hook");
            reported = true;
        }
    }
}

// ============================================================================
// Hook: Storage container open
//
// Fires when the player opens a private storage container. Captures the
// BSStorageComponent pointer and registers it in the StorageRegistry.
//
// The exact register containing the storage pointer depends on the
// discovered AOB pattern. Common BlackSpace patterns pass the component
// in rcx (first argument) or load it into rax from the actor chain.
//
// PLACEHOLDER: Register assignment needs verification from debugger.
// ============================================================================
static void OnStorageOpen(SafetyHookContext& ctx) {
    __try {
        // The storage component is likely in rcx (first arg to the UI open
        // function) or rax (loaded from the actor being interacted with).
        // Try rcx first as BlackSpace follows Microsoft x64 calling convention.
        auto* storage = reinterpret_cast<BSStorageComponent*>(ctx.rcx);
        if (!Memory::IsValidPtr(storage)) {
            // Fallback: try rax
            storage = reinterpret_cast<BSStorageComponent*>(ctx.rax);
            if (!Memory::IsValidPtr(storage)) return;
        }

        StorageRegistry::RegisterStorage(storage);

        if (s_storageOpenSamples < MAX_LOG_SAMPLES) {
            Logger::Debug("StorageOpen: rcx={:X} rax={:X} rdx={:X} r8={:X}",
                          ctx.rcx, ctx.rax, ctx.rdx, ctx.r8);
            s_storageOpenSamples++;
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        static bool reported = false;
        if (!reported) {
            Logger::Error("StorageOpen: exception in hook");
            reported = true;
        }
    }
}

// ============================================================================
// Install / Uninstall
// ============================================================================
void CraftHook::Install() {
    // 1. Player-pointer hook (required for identifying player data)
    auto ptrAddr = Memory::PatternScanOffset(
        nullptr, Patterns::PlayerPointer, Patterns::PlayerPointer_HookOffset);
    if (ptrAddr) {
        s_playerPtrHookIdx = HookManager::AddMidHook(ptrAddr, OnPlayerPointer, "PlayerPointer");
    } else {
        Logger::Error("CraftHook: PlayerPointer AOB not found");
    }

    // 2. Item-gain hook (monitoring + item table base capture)
    auto gainAddr = Memory::PatternScanOffset(
        nullptr, Patterns::ItemGain, Patterns::ItemGain_HookOffset);
    if (gainAddr) {
        s_itemGainHookIdx = HookManager::AddMidHook(gainAddr, OnItemGain, "ItemGain");
    } else {
        Logger::Error("CraftHook: ItemGain AOB not found");
    }

    // 3. Item-loss hook (core crafting interception)
    auto lossAddr = Memory::PatternScanOffset(
        nullptr, Patterns::ItemLoss, Patterns::ItemLoss_HookOffset);
    if (lossAddr) {
        s_itemLossHookIdx = HookManager::AddMidHook(lossAddr, OnItemLoss, "ItemLoss");
    } else {
        Logger::Error("CraftHook: ItemLoss AOB not found");
    }

    // 4. Storage-open hook (discovers storage container pointers)
    auto storageAddr = Memory::PatternScanOffset(
        nullptr, Patterns::StorageOpen, Patterns::StorageOpen_HookOffset);
    if (storageAddr) {
        s_storageOpenHookIdx = HookManager::AddMidHook(storageAddr, OnStorageOpen, "StorageOpen");
    } else {
        Logger::Warn("CraftHook: StorageOpen AOB not found (storage discovery disabled)");
        Logger::Warn("CraftHook: This pattern needs discovery via debugger - see README");
    }

    int activeCount = (s_playerPtrHookIdx >= 0 ? 1 : 0) +
                      (s_itemGainHookIdx >= 0 ? 1 : 0) +
                      (s_itemLossHookIdx >= 0 ? 1 : 0) +
                      (s_storageOpenHookIdx >= 0 ? 1 : 0);
    Logger::Info("CraftHook: installed ({}/4 hooks active)", activeCount);
}

void CraftHook::Uninstall() {
    if (s_playerPtrHookIdx >= 0) { HookManager::RemoveMidHook(s_playerPtrHookIdx); s_playerPtrHookIdx = -1; }
    if (s_itemGainHookIdx >= 0) { HookManager::RemoveMidHook(s_itemGainHookIdx); s_itemGainHookIdx = -1; }
    if (s_itemLossHookIdx >= 0) { HookManager::RemoveMidHook(s_itemLossHookIdx); s_itemLossHookIdx = -1; }
    if (s_storageOpenHookIdx >= 0) { HookManager::RemoveMidHook(s_storageOpenHookIdx); s_storageOpenHookIdx = -1; }

    StorageRegistry::Clear();
    g_playerState.Reset();
    Logger::Info("CraftHook: uninstalled");
}

} // namespace StorageCraft
