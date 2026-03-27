#include "core/hook_manager.h"
#include "core/logger.h"
#include <MinHook.h>

namespace StorageCraft {

std::vector<void*> HookManager::s_hooks;

bool HookManager::Init() {
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        Logger::Error("HookManager: MH_Initialize failed with status {}", static_cast<int>(status));
        return false;
    }
    Logger::Info("HookManager: initialized");
    return true;
}

void HookManager::Shutdown() {
    for (auto* target : s_hooks) {
        MH_DisableHook(target);
        MH_RemoveHook(target);
    }
    s_hooks.clear();
    MH_Uninitialize();
    Logger::Info("HookManager: shut down, all hooks removed");
}

bool HookManager::CreateAndEnable(void* target, void* detour, void** original) {
    MH_STATUS status = MH_CreateHook(target, detour, original);
    if (status != MH_OK) {
        Logger::Error("HookManager: MH_CreateHook failed at {:p} with status {}",
                      target, static_cast<int>(status));
        return false;
    }

    status = MH_EnableHook(target);
    if (status != MH_OK) {
        Logger::Error("HookManager: MH_EnableHook failed at {:p} with status {}",
                      target, static_cast<int>(status));
        MH_RemoveHook(target);
        return false;
    }

    s_hooks.push_back(target);
    Logger::Info("HookManager: hook installed at {:p}", target);
    return true;
}

bool HookManager::Unhook(void* target) {
    MH_STATUS status = MH_DisableHook(target);
    if (status != MH_OK) {
        Logger::Warn("HookManager: MH_DisableHook failed at {:p}", target);
        return false;
    }
    MH_RemoveHook(target);

    std::erase(s_hooks, target);
    Logger::Info("HookManager: hook removed at {:p}", target);
    return true;
}

} // namespace StorageCraft
