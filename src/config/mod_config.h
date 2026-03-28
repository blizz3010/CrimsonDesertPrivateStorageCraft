#pragma once

#include <string>
#include <mutex>

namespace StorageCraft {

struct ModSettings {
    // [General]
    bool        enabled         = true;
    bool        logEnabled      = true;
    int         initDelayMs     = 3000;   // Wait for game to fully load

    // [Keybinds]
    int         toggleKeyCode   = 0x78;   // VK_F9

    // [StorageCraft]
    float       maxStorageDistance = 1500.0f;
    bool        showStorageIcon   = true;
    bool        inventoryPriority = true;

    // [Logging]
    int         logLevel        = 1;      // 0=Debug, 1=Info, 2=Warn, 3=Error
};

// INI-based configuration matching Crimson Desert modding conventions.
// Uses Windows GetPrivateProfileString/Int APIs (same as player-status-modifier).
class ModConfig {
public:
    static void Load();
    static void Save();

    static bool IsEnabled();
    static void Toggle();
    static const ModSettings& Get();
    static int GetToggleKeyCode();

private:
    static std::wstring GetConfigPath();
    static int ReadInt(const wchar_t* section, const wchar_t* key, int defaultVal);
    static float ReadFloat(const wchar_t* section, const wchar_t* key, float defaultVal);
    static std::wstring ReadString(const wchar_t* section, const wchar_t* key, const wchar_t* defaultVal);

    static ModSettings s_settings;
    static std::mutex s_mutex;
    static std::wstring s_configPath;
};

} // namespace StorageCraft
