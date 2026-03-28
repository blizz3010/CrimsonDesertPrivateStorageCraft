#include "config/mod_config.h"
#include "core/logger.h"

#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace StorageCraft {

ModSettings ModConfig::s_settings;
std::mutex ModConfig::s_mutex;
std::wstring ModConfig::s_configPath;

void ModConfig::Load() {
    std::lock_guard lock(s_mutex);

    s_configPath = GetConfigPath();

    if (!std::filesystem::exists(s_configPath)) {
        Logger::Info("ModConfig: no config found, will use defaults");
        // The default_config.ini ships with the mod and is typically
        // copied alongside the .asi file during installation.
    }

    // [General]
    s_settings.enabled      = ReadInt(L"General", L"Enabled", 1) != 0;
    s_settings.logEnabled   = ReadInt(L"General", L"LogEnabled", 1) != 0;
    s_settings.initDelayMs  = ReadInt(L"General", L"InitDelayMs", 3000);

    // [Keybinds]
    auto keyName = ReadString(L"Keybinds", L"ToggleKey", L"F9");
    // Map key names to VK codes
    if (keyName == L"F1")  s_settings.toggleKeyCode = VK_F1;
    else if (keyName == L"F2")  s_settings.toggleKeyCode = VK_F2;
    else if (keyName == L"F3")  s_settings.toggleKeyCode = VK_F3;
    else if (keyName == L"F4")  s_settings.toggleKeyCode = VK_F4;
    else if (keyName == L"F5")  s_settings.toggleKeyCode = VK_F5;
    else if (keyName == L"F6")  s_settings.toggleKeyCode = VK_F6;
    else if (keyName == L"F7")  s_settings.toggleKeyCode = VK_F7;
    else if (keyName == L"F8")  s_settings.toggleKeyCode = VK_F8;
    else if (keyName == L"F9")  s_settings.toggleKeyCode = VK_F9;
    else if (keyName == L"F10") s_settings.toggleKeyCode = VK_F10;
    else if (keyName == L"F11") s_settings.toggleKeyCode = VK_F11;
    else if (keyName == L"F12") s_settings.toggleKeyCode = VK_F12;
    else if (keyName == L"INSERT") s_settings.toggleKeyCode = VK_INSERT;
    else if (keyName == L"DELETE") s_settings.toggleKeyCode = VK_DELETE;
    else s_settings.toggleKeyCode = VK_F9;

    // [StorageCraft]
    s_settings.maxStorageDistance = ReadFloat(L"StorageCraft", L"MaxStorageDistance", 1500.0f);
    s_settings.showStorageIcon   = ReadInt(L"StorageCraft", L"ShowStorageIcon", 1) != 0;
    s_settings.inventoryPriority = ReadInt(L"StorageCraft", L"InventoryPriority", 1) != 0;

    // [Logging]
    s_settings.logLevel = ReadInt(L"Logging", L"Level", 1);

    Logger::Info("ModConfig: loaded (enabled={}, toggleKey=0x{:X}, maxDist={:.0f})",
                 s_settings.enabled, s_settings.toggleKeyCode, s_settings.maxStorageDistance);
}

void ModConfig::Save() {
    // INI writing via WritePrivateProfileString
    // For now, we don't auto-save - users edit the INI manually.
    // This matches the convention of other CD mods.
}

bool ModConfig::IsEnabled() {
    std::lock_guard lock(s_mutex);
    return s_settings.enabled;
}

void ModConfig::Toggle() {
    std::lock_guard lock(s_mutex);
    s_settings.enabled = !s_settings.enabled;
    Logger::Info("StorageCraft: {}", s_settings.enabled ? "ENABLED" : "DISABLED");
}

const ModSettings& ModConfig::Get() {
    return s_settings;
}

int ModConfig::GetToggleKeyCode() {
    std::lock_guard lock(s_mutex);
    return s_settings.toggleKeyCode;
}

std::wstring ModConfig::GetConfigPath() {
#ifdef _WIN32
    HMODULE hModule = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&Load),
        &hModule
    );
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(hModule, path, MAX_PATH);
    auto dir = std::filesystem::path(path).parent_path();
    return (dir / L"StorageCraft.ini").wstring();
#else
    return L"StorageCraft.ini";
#endif
}

int ModConfig::ReadInt(const wchar_t* section, const wchar_t* key, int defaultVal) {
#ifdef _WIN32
    return GetPrivateProfileIntW(section, key, defaultVal, s_configPath.c_str());
#else
    return defaultVal;
#endif
}

float ModConfig::ReadFloat(const wchar_t* section, const wchar_t* key, float defaultVal) {
#ifdef _WIN32
    wchar_t buf[64];
    GetPrivateProfileStringW(section, key, L"", buf, 64, s_configPath.c_str());
    if (buf[0] == L'\0') return defaultVal;
    try { return std::stof(buf); } catch (...) { return defaultVal; }
#else
    return defaultVal;
#endif
}

std::wstring ModConfig::ReadString(const wchar_t* section, const wchar_t* key, const wchar_t* defaultVal) {
#ifdef _WIN32
    wchar_t buf[256];
    GetPrivateProfileStringW(section, key, defaultVal, buf, 256, s_configPath.c_str());
    return buf;
#else
    return defaultVal;
#endif
}

} // namespace StorageCraft
