// ============================================================================
// StorageCraft - Crimson Desert Private Storage Crafting Mod
//
// ASI plugin loaded by an ASI loader (e.g., Ultimate ASI Loader).
// Follows the same initialization pattern as other CD mods
// (CrimsonDesert-player-status-modifier).
//
// Flow:
//   1. ASI loader loads StorageCraft.asi into the game process
//   2. DllMain spawns a background thread (avoids loader lock)
//   3. Thread waits for game to initialize (configurable delay)
//   4. Installs hooks via SafetyHook (mid-function hooks)
//   5. Polls for toggle keybind (F9) and unload hotkey (Ctrl+Shift+U)
// ============================================================================

#include "core/logger.h"
#include "core/hook_manager.h"
#include "config/mod_config.h"
#include "craft/craft_hook.h"
#include "ui/ui_hook.h"
#include "ui/storage_indicator.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace StorageCraft {

static std::atomic<bool> s_running = false;
static HMODULE s_hModule = nullptr;

#ifdef _WIN32
static DWORD WINAPI ModThread(LPVOID) {
    // Initialize logging
    Logger::Init();
    Logger::Info("========================================");
    Logger::Info("StorageCraft v1.0.0");
    Logger::Info("Private Storage Crafting Mod");
    Logger::Info("Crimson Desert (BlackSpace Engine)");
    Logger::Info("========================================");

    // Load configuration from INI
    ModConfig::Load();
    Logger::SetLevel(ModConfig::Get().logLevel);

    // Wait for game to fully initialize before hooking.
    // This delay is critical - hooking too early will crash because
    // the game's code sections aren't fully loaded yet.
    // The player-status-modifier uses 3000ms as default.
    int delay = ModConfig::Get().initDelayMs;
    Logger::Info("Waiting {}ms for game initialization...", delay);
    Sleep(delay);

    // Initialize SafetyHook
    if (!HookManager::Init()) {
        Logger::Error("Failed to initialize HookManager, aborting");
        Logger::Shutdown();
        FreeLibraryAndExitThread(s_hModule, 1);
        return 1;
    }

    // Install hooks
    CraftHook::Install();
    UIHook::Install();

    Logger::Info("All hooks installed. StorageCraft is {}.",
                 ModConfig::IsEnabled() ? "ENABLED" : "DISABLED");
    Logger::Info("Press F9 to toggle. Press Ctrl+Shift+U to unload.");

    s_running = true;

    // Input polling loop (~20 Hz, matching CD mod conventions)
    while (s_running) {
        // Toggle key
        if (GetAsyncKeyState(ModConfig::GetToggleKeyCode()) & 1) {
            ModConfig::Toggle();
        }

        // Unload hotkey: Ctrl+Shift+U
        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
            (GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
            (GetAsyncKeyState('U') & 1)) {
            Logger::Info("Unload hotkey pressed");
            break;
        }

        Sleep(50);
    }

    // Clean shutdown
    Logger::Info("Shutting down StorageCraft...");
    UIHook::Uninstall();
    CraftHook::Uninstall();
    StorageIndicator::Shutdown();
    HookManager::Shutdown();
    Logger::Info("Goodbye!");
    Logger::Shutdown();

    FreeLibraryAndExitThread(s_hModule, 0);
    return 0;
}
#endif

} // namespace StorageCraft

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID /*reserved*/) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            StorageCraft::s_hModule = hModule;
            if (auto hThread = CreateThread(nullptr, 0, StorageCraft::ModThread, nullptr, 0, nullptr)) {
                CloseHandle(hThread);
            }
            break;

        case DLL_PROCESS_DETACH:
            StorageCraft::s_running = false;
            break;
    }
    return TRUE;
}
#endif
