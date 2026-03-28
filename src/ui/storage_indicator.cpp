#include "ui/storage_indicator.h"
#include "core/logger.h"

namespace StorageCraft {

bool StorageIndicator::s_markedSlots[MAX_SLOTS] = {};
bool StorageIndicator::s_initialized = false;

void StorageIndicator::Init() {
    if (s_initialized) return;
    ClearAll();

    // TODO: Resolve BlackSpace UI widget pointers for the crafting panel.
    // Potential approaches:
    //
    // 1. Widget property patching:
    //    Find the crafting slot widgets via AOB, then modify their
    //    icon/color properties to indicate storage sourcing.
    //    (e.g., change slot border color or add an overlay icon index)
    //
    // 2. DirectX overlay:
    //    Hook the D3D11/D3D12 Present function and draw a small chest
    //    icon at the screen coordinates of marked slots. This is more
    //    invasive but doesn't depend on the BlackSpace widget layout.
    //
    // 3. Text modification:
    //    Append a marker character (e.g., "*" or "[S]") to the material
    //    count text string. Simplest approach but least visual.
    //
    // For now, we track state and log. The visual rendering will be
    // implemented once the BlackSpace UI structure is better understood.

    s_initialized = true;
    Logger::Info("StorageIndicator: initialized (tracking {} slots)", MAX_SLOTS);
}

void StorageIndicator::Shutdown() {
    ClearAll();
    s_initialized = false;
}

void StorageIndicator::MarkSlotAsStorageSourced(int32_t slotIndex) {
    if (slotIndex >= 0 && slotIndex < MAX_SLOTS) {
        s_markedSlots[slotIndex] = true;
    }
}

void StorageIndicator::ClearSlot(int32_t slotIndex) {
    if (slotIndex >= 0 && slotIndex < MAX_SLOTS) {
        s_markedSlots[slotIndex] = false;
    }
}

void StorageIndicator::ClearAll() {
    for (int i = 0; i < MAX_SLOTS; i++) {
        s_markedSlots[i] = false;
    }
}

bool StorageIndicator::IsSlotMarked(int32_t slotIndex) {
    if (slotIndex >= 0 && slotIndex < MAX_SLOTS) {
        return s_markedSlots[slotIndex];
    }
    return false;
}

} // namespace StorageCraft
