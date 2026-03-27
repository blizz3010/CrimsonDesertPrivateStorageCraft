#pragma once

#include <vector>
#include <cstdint>

// Forward declare MinHook status
typedef enum MH_STATUS MH_STATUS;

namespace StorageCraft {

class HookManager {
public:
    // Initialize MinHook. Call once at mod startup.
    static bool Init();

    // Remove all hooks and uninitialize MinHook. Call at mod shutdown.
    static void Shutdown();

    // Create and enable a hook. Returns the trampoline (original function pointer).
    // Usage:
    //   using Fn = void(*)(int);
    //   static Fn Original = nullptr;
    //   Original = HookManager::Hook<Fn>(targetAddr, &MyDetour);
    template<typename T>
    static T Hook(void* target, T detour) {
        void* original = nullptr;
        if (CreateAndEnable(target, reinterpret_cast<void*>(detour), &original)) {
            return reinterpret_cast<T>(original);
        }
        return nullptr;
    }

    // Disable and remove a specific hook.
    static bool Unhook(void* target);

private:
    static bool CreateAndEnable(void* target, void* detour, void** original);
    static std::vector<void*> s_hooks;
};

} // namespace StorageCraft
