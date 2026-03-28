#pragma once

#include <safetyhook.hpp>
#include <vector>
#include <string>
#include <functional>

namespace StorageCraft {

// Wraps SafetyHook for mid-function and inline hooks.
// Mid-function hooks (SafetyHookMid) are the standard approach for
// Crimson Desert mods - they insert a callback at a specific instruction
// without replacing the original code flow.
//
// Reference: Orcax-1399/CrimsonDesert-player-status-modifier uses
// SafetyHookMid exclusively for all 5 of its hooks.
class HookManager {
public:
    static bool Init();
    static void Shutdown();

    // Create a mid-function hook at the given address.
    // The callback receives a SafetyHookContext& with register access.
    // Returns an index that can be used to remove the hook later.
    // Returns -1 on failure.
    static int AddMidHook(
        uintptr_t address,
        SafetyHookMid::HookFn callback,
        const std::string& name = ""
    );

    // Create an inline hook (replaces function entry).
    // Less common for CD mods but useful for full function replacement.
    // Returns an index, or -1 on failure.
    static int AddInlineHook(
        uintptr_t address,
        void* detour,
        const std::string& name = ""
    );

    // Get the trampoline (original function) for an inline hook by index.
    template<typename T>
    static T GetOriginal(int hookIndex) {
        if (hookIndex < 0 || hookIndex >= static_cast<int>(s_inlineHooks.size()))
            return nullptr;
        return reinterpret_cast<T>(s_inlineHooks[hookIndex].trampoline().address());
    }

    // Remove a specific hook by index.
    static void RemoveMidHook(int index);
    static void RemoveInlineHook(int index);

private:
    struct MidHookEntry {
        SafetyHookMid hook;
        std::string name;
    };

    struct InlineHookEntry {
        SafetyHookInline hook;
        std::string name;
    };

    static std::vector<MidHookEntry> s_midHooks;
    static std::vector<InlineHookEntry> s_inlineHooks;
    static bool s_initialized;
};

} // namespace StorageCraft
