#pragma once

#include <string>
#include <mutex>
#include <unordered_map>

namespace StorageCraft {

struct ModSettings {
    bool        enabled           = true;
    int         toggleKeyCode     = 0x78;    // VK_F9
    float       maxStorageDistance = 1500.0f; // Unreal units (~15 meters)
    bool        showStorageIcon   = true;
    std::string logLevel          = "Info";
};

class ModConfig {
public:
    // Load config from JSON file next to the DLL.
    // Creates default config if none exists.
    static void Load();

    // Save current settings to disk.
    static void Save();

    // Quick accessors
    static bool IsEnabled();
    static void Toggle();
    static const ModSettings& Get();

    // Get the toggle key's virtual key code.
    static int GetToggleKeyCode();

private:
    static std::string GetConfigPath();
    static int KeyNameToVK(const std::string& name);
    static std::string VKToKeyName(int vk);

    static ModSettings s_settings;
    static std::mutex s_mutex;

    // Key name -> VK code mapping
    static const std::unordered_map<std::string, int> s_keyMap;
};

} // namespace StorageCraft
