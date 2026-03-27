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

#ifdef _WIN32
// Main mod thread - runs after DLL injection to avoid loader lock
static DWORD WINAPI ModThread(LPVOID hModule) {
    // Initialize logging first
    Logger::Init();
    Logger::Info("========================================");
    Logger::Info("StorageCraft v1.0.0 - Private Storage Crafting Mod");
    Logger::Info("========================================");

    // Load configuration
    ModConfig::Load();
    Logger::SetLevel(Logger::LevelFromString(ModConfig::Get().logLevel));

    // Initialize hooking engine
    if (!HookManager::Init()) {
        Logger::Error("Failed to initialize hook manager, aborting");
        Logger::Shutdown();
        FreeLibraryAndExitThread(static_cast<HMODULE>(hModule), 1);
        return 1;
    }

    // Install game hooks
    CraftHook::Install();
    UIHook::Install();

    Logger::Info("All hooks installed. Mod is {}.",
                 ModConfig::IsEnabled() ? "ENABLED" : "DISABLED");
    Logger::Info("Press {} to toggle.", "F9"); // TODO: resolve from config

    s_running = true;

    // Input polling loop for toggle keybind
    while (s_running) {
        // Check for toggle key press
        if (GetAsyncKeyState(ModConfig::GetToggleKeyCode()) & 1) {
            ModConfig::Toggle();
            ModConfig::Save();
        }

        // Check for unload key (Ctrl+Shift+U)
        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
            (GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
            (GetAsyncKeyState('U') & 1)) {
            Logger::Info("Unload hotkey pressed, shutting down...");
            break;
        }

        Sleep(50); // ~20 Hz polling
    }

    // Cleanup
    Logger::Info("Shutting down StorageCraft...");

    UIHook::Uninstall();
    CraftHook::Uninstall();
    StorageIndicator::Shutdown();
    HookManager::Shutdown();

    Logger::Info("Goodbye!");
    Logger::Shutdown();

    FreeLibraryAndExitThread(static_cast<HMODULE>(hModule), 0);
    return 0;
}
#endif

} // namespace StorageCraft

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID /*reserved*/) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            // Spawn a separate thread to avoid loader lock issues
            if (auto hThread = CreateThread(nullptr, 0, StorageCraft::ModThread, hModule, 0, nullptr)) {
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
