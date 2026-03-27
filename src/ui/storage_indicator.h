#pragma once

namespace StorageCraft {

// Manages the small chest icon overlay that appears next to material counts
// in the crafting UI when items are being sourced from storage.
class StorageIndicator {
public:
    // Initialize the indicator system (loads icon texture).
    static void Init();

    // Show the chest icon on a material slot widget.
    static void Show(void* materialSlotWidget);

    // Hide the chest icon on a material slot widget.
    static void Hide(void* materialSlotWidget);

    // Clean up resources.
    static void Shutdown();

private:
    static void* s_iconTexture;
    static bool s_initialized;
};

} // namespace StorageCraft
