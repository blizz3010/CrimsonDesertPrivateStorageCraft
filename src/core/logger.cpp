#include "core/logger.h"
#include <chrono>
#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace StorageCraft {

std::ofstream Logger::s_file;
std::mutex Logger::s_mutex;
LogLevel Logger::s_level = LogLevel::Info;
bool Logger::s_initialized = false;

void Logger::Init(const std::string& filename) {
    std::lock_guard lock(s_mutex);
    if (s_initialized) return;

#ifdef _WIN32
    HMODULE hModule = nullptr;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(&Init),
        &hModule
    );
    char path[MAX_PATH];
    GetModuleFileNameA(hModule, path, MAX_PATH);
    auto logPath = std::filesystem::path(path).parent_path() / filename;
#else
    auto logPath = std::filesystem::path(filename);
#endif

    s_file.open(logPath, std::ios::out | std::ios::trunc);
    s_initialized = true;
    Log(LogLevel::Info, "StorageCraft logger initialized");
}

void Logger::Shutdown() {
    std::lock_guard lock(s_mutex);
    if (!s_initialized) return;
    Log(LogLevel::Info, "StorageCraft logger shutting down");
    s_file.close();
    s_initialized = false;
}

void Logger::SetLevel(LogLevel level) {
    std::lock_guard lock(s_mutex);
    s_level = level;
}

void Logger::SetLevel(int level) {
    SetLevel(static_cast<LogLevel>(std::clamp(level, 0, 3)));
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (level < s_level) return;

    const char* tag = "INFO";
    switch (level) {
        case LogLevel::Debug: tag = "DEBUG"; break;
        case LogLevel::Info:  tag = "INFO";  break;
        case LogLevel::Warn:  tag = "WARN";  break;
        case LogLevel::Error: tag = "ERROR"; break;
    }

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", std::localtime(&time));

    auto line = std::format("[{}.{:03d}] [{}] {}\n", timeBuf, ms.count(), tag, message);

    if (s_initialized && s_file.is_open()) {
        s_file << line;
        s_file.flush();
    }

#ifdef _WIN32
    OutputDebugStringA(line.c_str());
#endif
}

} // namespace StorageCraft
