#include "ui/storage_indicator.h"
#include "core/memory.h"
#include "core/logger.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace StorageCraft {

void* StorageIndicator::s_iconTexture = nullptr;
bool StorageIndicator::s_initialized = false;

// ============================================================================
// UE5 UMG function signatures (resolved via pattern scan)
// ============================================================================
namespace Patterns {
    // UWidgetTree::ConstructWidget<UImage>(UClass*, FName)
    constexpr const char* ConstructWidget =
        "48 89 5C 24 08 57 48 83 EC 30 49 8B F8 48 8B DA";

    // UImage::SetBrushFromTexture(UTexture2D*, bool)
    constexpr const char* SetBrushFromTexture =
        "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 41 0F B6 F0";

    // UPanelWidget::AddChild(UWidget*)
    constexpr const char* AddChild =
        "48 89 5C 24 10 48 89 6C 24 18 56 57 41 56 48 83 EC 20";
}

namespace Offsets {
    // Tag value we use to identify our injected icon widgets
    constexpr int32_t StorageIconTag = 0x5C5C; // "SC" in hex - StorageCraft marker

    // UWidget offsets for positioning
    constexpr ptrdiff_t Widget_Slot        = 0x58;
    constexpr ptrdiff_t Slot_LayoutOffset  = 0x10; // FVector2D offset within parent
    constexpr ptrdiff_t Widget_Tag         = 0x88; // int32 custom tag
    constexpr ptrdiff_t Widget_Visibility  = 0x90; // ESlateVisibility
}

// Function pointer types
using ConstructWidgetFn = void*(*)(void* widgetTree, void* widgetClass, void* name);
using SetBrushFromTextureFn = void(*)(void* imageWidget, void* texture, bool matchSize);
using AddChildFn = void*(*)(void* panelWidget, void* childWidget);

// Cached function pointers (resolved on Init)
static ConstructWidgetFn s_constructWidget = nullptr;
static SetBrushFromTextureFn s_setBrushFromTexture = nullptr;
static AddChildFn s_addChild = nullptr;

void StorageIndicator::Init() {
    if (s_initialized) return;

    // Resolve UMG function pointers via pattern scan
    auto addr1 = Memory::PatternScan(nullptr, Patterns::ConstructWidget);
    auto addr2 = Memory::PatternScan(nullptr, Patterns::SetBrushFromTexture);
    auto addr3 = Memory::PatternScan(nullptr, Patterns::AddChild);

    if (!addr1 || !addr2 || !addr3) {
        Logger::Warn("StorageIndicator: could not resolve all UMG functions, icon display disabled");
        return;
    }

    s_constructWidget = reinterpret_cast<ConstructWidgetFn>(addr1);
    s_setBrushFromTexture = reinterpret_cast<SetBrushFromTextureFn>(addr2);
    s_addChild = reinterpret_cast<AddChildFn>(addr3);

    // TODO: Load chest icon texture from DLL resources or from disk
    // For now, we'll use the game's built-in chest/container icon if available
    // s_iconTexture = LoadTextureFromResource(...);

    s_initialized = true;
    Logger::Info("StorageIndicator: initialized");
}

void StorageIndicator::Show(void* materialSlotWidget) {
    if (!s_initialized || !materialSlotWidget) return;

    // Check if we already added an icon to this slot (by checking for our tag)
    int32_t existingTag = Memory::ReadOffset<int32_t>(
        materialSlotWidget, Offsets::Widget_Tag);

    if (existingTag == Offsets::StorageIconTag) {
        // Icon already present, just make sure it's visible
        Memory::WriteOffset<uint8_t>(materialSlotWidget, Offsets::Widget_Visibility, 0); // Visible
        return;
    }

    // TODO: Create and attach a UImage widget as a child of the material slot.
    // This requires calling into UE's widget construction API:
    //   1. ConstructWidget to create a UImage
    //   2. SetBrushFromTexture to set the chest icon
    //   3. Configure size (16x16) and position (right-aligned, small offset)
    //   4. AddChild to attach it to the slot's panel
    //   5. Tag it with our marker so we can find it later

    Logger::Debug("StorageIndicator: showing icon on slot {:p}", materialSlotWidget);
}

void StorageIndicator::Hide(void* materialSlotWidget) {
    if (!materialSlotWidget) return;

    // Find our tagged icon widget and hide it
    int32_t existingTag = Memory::ReadOffset<int32_t>(
        materialSlotWidget, Offsets::Widget_Tag);

    if (existingTag == Offsets::StorageIconTag) {
        // Set visibility to Hidden (2) or Collapsed (1)
        Memory::WriteOffset<uint8_t>(materialSlotWidget, Offsets::Widget_Visibility, 1); // Collapsed
        Logger::Debug("StorageIndicator: hiding icon on slot {:p}", materialSlotWidget);
    }
}

void StorageIndicator::Shutdown() {
    s_iconTexture = nullptr;
    s_initialized = false;
    Logger::Info("StorageIndicator: shut down");
}

} // namespace StorageCraft
