#pragma once

#include <cstdint>

namespace StorageCraft {

// Visual indicator that shows when materials are being sourced from storage.
//
// In the BlackSpace Engine, UI elements are rendered through the engine's
// own widget system (not UE5 UMG/Slate). The indicator works by:
//   1. Finding the crafting slot widget memory via AOB
//   2. Modifying the icon/overlay state to show a chest symbol
//   3. Alternatively, overlaying a DirectX texture if widget patching
//      proves too fragile
//
// The simplest approach that other CD mods use is to modify existing
// UI element properties rather than creating new widgets.
class StorageIndicator {
public:
    static void Init();
    static void Shutdown();

    // Mark a crafting slot as having materials from storage.
    // slotIndex corresponds to the material slot in the crafting UI.
    static void MarkSlotAsStorageSourced(int32_t slotIndex);
    static void ClearSlot(int32_t slotIndex);
    static void ClearAll();

    // Check if a slot is currently marked.
    static bool IsSlotMarked(int32_t slotIndex);

private:
    static constexpr int MAX_SLOTS = 16;
    static bool s_markedSlots[MAX_SLOTS];
    static bool s_initialized;
};

} // namespace StorageCraft
