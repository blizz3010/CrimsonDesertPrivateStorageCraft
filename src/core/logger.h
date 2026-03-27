#pragma once

#include <string>
#include <mutex>
#include <fstream>
#include <format>

namespace StorageCraft {

enum class LogLevel { Debug, Info, Warn, Error };

class Logger {
public:
    static void Init(const std::string& filename = "StorageCraft.log");
    static void Shutdown();
    static void SetLevel(LogLevel level);

    template<typename... Args>
    static void Debug(std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Debug, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void Info(std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Info, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void Warn(std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Warn, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void Error(std::format_string<Args...> fmt, Args&&... args) {
        Log(LogLevel::Error, std::format(fmt, std::forward<Args>(args)...));
    }

    static LogLevel LevelFromString(const std::string& str);

private:
    static void Log(LogLevel level, const std::string& message);

    static std::ofstream s_file;
    static std::mutex s_mutex;
    static LogLevel s_level;
    static bool s_initialized;
};

} // namespace StorageCraft
