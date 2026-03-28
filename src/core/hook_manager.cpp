#include "core/hook_manager.h"
#include "core/logger.h"

namespace StorageCraft {

std::vector<HookManager::MidHookEntry> HookManager::s_midHooks;
std::vector<HookManager::InlineHookEntry> HookManager::s_inlineHooks;
bool HookManager::s_initialized = false;

bool HookManager::Init() {
    if (s_initialized) return true;
    s_midHooks.clear();
    s_inlineHooks.clear();
    s_initialized = true;
    Logger::Info("HookManager: initialized (SafetyHook)");
    return true;
}

void HookManager::Shutdown() {
    // SafetyHook objects unhook on destruction
    for (auto& entry : s_midHooks) {
        if (entry.hook) {
            Logger::Debug("HookManager: removing mid-hook '{}'", entry.name);
            entry.hook = {};
        }
    }
    for (auto& entry : s_inlineHooks) {
        if (entry.hook) {
            Logger::Debug("HookManager: removing inline-hook '{}'", entry.name);
            entry.hook = {};
        }
    }
    s_midHooks.clear();
    s_inlineHooks.clear();
    s_initialized = false;
    Logger::Info("HookManager: shut down, all hooks removed");
}

int HookManager::AddMidHook(uintptr_t address, SafetyHookMid::HookFn callback, const std::string& name) {
    if (!s_initialized) return -1;

    auto hook = safetyhook::create_mid(reinterpret_cast<void*>(address), callback);
    if (!hook) {
        Logger::Error("HookManager: failed to create mid-hook '{}' at {:X}", name, address);
        return -1;
    }

    int index = static_cast<int>(s_midHooks.size());
    s_midHooks.push_back({std::move(hook), name});
    Logger::Info("HookManager: mid-hook '{}' installed at {:X} (index {})", name, address, index);
    return index;
}

int HookManager::AddInlineHook(uintptr_t address, void* detour, const std::string& name) {
    if (!s_initialized) return -1;

    auto hook = safetyhook::create_inline(reinterpret_cast<void*>(address), detour);
    if (!hook) {
        Logger::Error("HookManager: failed to create inline-hook '{}' at {:X}", name, address);
        return -1;
    }

    int index = static_cast<int>(s_inlineHooks.size());
    s_inlineHooks.push_back({std::move(hook), name});
    Logger::Info("HookManager: inline-hook '{}' installed at {:X} (index {})", name, address, index);
    return index;
}

void HookManager::RemoveMidHook(int index) {
    if (index >= 0 && index < static_cast<int>(s_midHooks.size())) {
        Logger::Debug("HookManager: removing mid-hook '{}' (index {})", s_midHooks[index].name, index);
        s_midHooks[index].hook = {};
    }
}

void HookManager::RemoveInlineHook(int index) {
    if (index >= 0 && index < static_cast<int>(s_inlineHooks.size())) {
        Logger::Debug("HookManager: removing inline-hook '{}' (index {})", s_inlineHooks[index].name, index);
        s_inlineHooks[index].hook = {};
    }
}

} // namespace StorageCraft
