// ============================================================================
// StorageCraft - Crimson Desert Private Storage Crafting Mod
//
// ASI plugin for the BlackSpace Engine.
// Uses verified patterns from Orcax-1399/CrimsonDesert-player-status-modifier.
//
// Init flow (matching CD mod conventions):
//   1. DLL_PROCESS_ATTACH -> spawn thread (avoid loader lock)
//   2. Load INI config
//   3. Initialize logger
//   4. Sleep InitDelayMs (default 3000ms) for game to fully load
//   5. Reset runtime state
//   6. Install hooks (player-pointer, item-gain, item-loss, UI)
//   7. Poll for keybinds (toggle F9, unload Ctrl+Shift+U)
//   8. On unload: remove hooks, shutdown logger, free library
// ============================================================================

#include "core/logger.h"
#include "core/hook_manager.h"
#include "config/mod_config.h"
#include "craft/craft_hook.h"
#include "ui/ui_hook.h"
#include "ui/storage_indicator.h"
#include "game/game_types.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace StorageCraft {

static std::atomic<bool> s_running = false;
static HMODULE s_hModule = nullptr;

static void ResetRuntimeState() {
    g_playerState.Reset();
    Logger::Debug("Runtime state reset");
}

#ifdef _WIN32
static DWORD WINAPI ModThread(LPVOID) {
    // Load config first (needed for delay and log settings)
    ModConfig::Load();

    // Initialize logging
    Logger::Init();
    Logger::SetLevel(ModConfig::Get().logLevel);
    Logger::Info("========================================");
    Logger::Info("StorageCraft v1.1.0");
    Logger::Info("Crimson Desert Private Storage Crafting");
    Logger::Info("Engine: BlackSpace (Pearl Abyss)");
    Logger::Info("Hooks: SafetyHook (mid-function)");
    Logger::Info("========================================");

    // Wait for game to fully initialize
    int delay = ModConfig::Get().initDelayMs;
    Logger::Info("Waiting {}ms for game initialization...", delay);
    Sleep(delay);

    // Reset any stale state
    ResetRuntimeState();

    // Initialize hook engine
    if (!HookManager::Init()) {
        Logger::Error("Failed to initialize HookManager, aborting");
        Logger::Shutdown();
        FreeLibraryAndExitThread(s_hModule, 1);
        return 1;
    }

    // Install hooks - order matters!
    // Player-pointer MUST be first (other hooks depend on g_playerState)
    CraftHook::Install();
    UIHook::Install();

    Logger::Info("Hooks installed. StorageCraft is {}.",
                 ModConfig::IsEnabled() ? "ENABLED" : "DISABLED");
    Logger::Info("Press F9 to toggle. Ctrl+Shift+U to unload.");

    s_running = true;

    // Input polling loop (~20 Hz)
    while (s_running) {
        if (GetAsyncKeyState(ModConfig::GetToggleKeyCode()) & 1) {
            ModConfig::Toggle();
        }

        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
            (GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
            (GetAsyncKeyState('U') & 1)) {
            Logger::Info("Unload hotkey pressed");
            break;
        }

        Sleep(50);
    }

    // Clean shutdown (reverse order of init)
    Logger::Info("Shutting down...");
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
