#include "config/mod_config.h"
#include "core/logger.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace StorageCraft {

ModSettings ModConfig::s_settings;
std::mutex ModConfig::s_mutex;

const std::unordered_map<std::string, int> ModConfig::s_keyMap = {
    {"F1",  0x70}, {"F2",  0x71}, {"F3",  0x72}, {"F4",  0x73},
    {"F5",  0x74}, {"F6",  0x75}, {"F7",  0x76}, {"F8",  0x77},
    {"F9",  0x78}, {"F10", 0x79}, {"F11", 0x7A}, {"F12", 0x7B},
    {"INSERT", 0x2D}, {"DELETE", 0x2E}, {"HOME", 0x24}, {"END", 0x23},
    {"PAGEUP", 0x21}, {"PAGEDOWN", 0x22},
    {"NUMPAD0", 0x60}, {"NUMPAD1", 0x61}, {"NUMPAD2", 0x62},
    {"NUMPAD3", 0x63}, {"NUMPAD4", 0x64}, {"NUMPAD5", 0x65},
    {"NUMPAD6", 0x66}, {"NUMPAD7", 0x67}, {"NUMPAD8", 0x68},
    {"NUMPAD9", 0x69},
};

void ModConfig::Load() {
    std::lock_guard lock(s_mutex);

    auto configPath = GetConfigPath();

    if (!std::filesystem::exists(configPath)) {
        Logger::Info("ModConfig: no config file found, creating defaults at {}", configPath);
        Save();
        return;
    }

    try {
        std::ifstream file(configPath);
        auto json = nlohmann::json::parse(file);

        if (json.contains("enabled"))
            s_settings.enabled = json["enabled"].get<bool>();
        if (json.contains("toggleKey"))
            s_settings.toggleKeyCode = KeyNameToVK(json["toggleKey"].get<std::string>());
        if (json.contains("maxStorageDistance"))
            s_settings.maxStorageDistance = json["maxStorageDistance"].get<float>();
        if (json.contains("showStorageIcon"))
            s_settings.showStorageIcon = json["showStorageIcon"].get<bool>();
        if (json.contains("logLevel"))
            s_settings.logLevel = json["logLevel"].get<std::string>();

        Logger::Info("ModConfig: loaded from {}", configPath);
        Logger::Info("ModConfig: enabled={}, toggleKey=0x{:X}, maxDist={:.0f}, showIcon={}",
                     s_settings.enabled, s_settings.toggleKeyCode,
                     s_settings.maxStorageDistance, s_settings.showStorageIcon);

    } catch (const std::exception& e) {
        Logger::Error("ModConfig: failed to parse config: {}", e.what());
        Logger::Info("ModConfig: using default settings");
    }
}

void ModConfig::Save() {
    auto configPath = GetConfigPath();

    try {
        nlohmann::json json;
        json["enabled"] = s_settings.enabled;
        json["toggleKey"] = VKToKeyName(s_settings.toggleKeyCode);
        json["maxStorageDistance"] = s_settings.maxStorageDistance;
        json["showStorageIcon"] = s_settings.showStorageIcon;
        json["logLevel"] = s_settings.logLevel;

        // Ensure directory exists
        auto dir = std::filesystem::path(configPath).parent_path();
        if (!dir.empty()) {
            std::filesystem::create_directories(dir);
        }

        std::ofstream file(configPath);
        file << json.dump(4) << std::endl;

        Logger::Debug("ModConfig: saved to {}", configPath);

    } catch (const std::exception& e) {
        Logger::Error("ModConfig: failed to save config: {}", e.what());
    }
}

bool ModConfig::IsEnabled() {
    std::lock_guard lock(s_mutex);
    return s_settings.enabled;
}

void ModConfig::Toggle() {
    std::lock_guard lock(s_mutex);
    s_settings.enabled = !s_settings.enabled;
    Logger::Info("ModConfig: storage craft {}", s_settings.enabled ? "ENABLED" : "DISABLED");
}

const ModSettings& ModConfig::Get() {
    return s_settings;
}

int ModConfig::GetToggleKeyCode() {
    std::lock_guard lock(s_mutex);
    return s_settings.toggleKeyCode;
}

std::string ModConfig::GetConfigPath() {
#ifdef _WIN32
    HMODULE hModule = nullptr;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(&Load),
        &hModule
    );
    char path[MAX_PATH];
    GetModuleFileNameA(hModule, path, MAX_PATH);
    return (std::filesystem::path(path).parent_path() / "StorageCraft.json").string();
#else
    return "StorageCraft.json";
#endif
}

int ModConfig::KeyNameToVK(const std::string& name) {
    auto it = s_keyMap.find(name);
    if (it != s_keyMap.end()) return it->second;

    // Single character keys (A-Z, 0-9)
    if (name.length() == 1) {
        char c = name[0];
        if (c >= 'A' && c <= 'Z') return static_cast<int>(c);
        if (c >= '0' && c <= '9') return static_cast<int>(c);
    }

    Logger::Warn("ModConfig: unknown key name '{}', defaulting to F9", name);
    return 0x78; // VK_F9
}

std::string ModConfig::VKToKeyName(int vk) {
    for (const auto& [name, code] : s_keyMap) {
        if (code == vk) return name;
    }
    if (vk >= 'A' && vk <= 'Z') return std::string(1, static_cast<char>(vk));
    if (vk >= '0' && vk <= '9') return std::string(1, static_cast<char>(vk));
    return "F9";
}

} // namespace StorageCraft
