#include "ui/storage_indicator.h"
#include "core/logger.h"

namespace StorageCraft {

bool StorageIndicator::s_markedSlots[MAX_SLOTS] = {};
bool StorageIndicator::s_initialized = false;

void StorageIndicator::Init() {
    if (s_initialized) return;
    ClearAll();

    // Visual indicator approach:
    //
    // We use the text modification approach - the UICountUpdate hook
    // inflates the material count with storage totals, and this module
    // tracks which slots received storage contributions. The UI hook
    // can then append a marker to the count string if needed.
    //
    // For more advanced rendering (icon overlay, border color), a
    // DirectX hook on Present() would be needed. That's a significant
    // addition and optional - the combined count display is the primary
    // user-facing feature. The slot tracking here enables future
    // visual enhancements without changing the hook architecture.

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
