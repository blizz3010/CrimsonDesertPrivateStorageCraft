#include "core/memory.h"
#include "core/logger.h"
#include <vector>
#include <string>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace StorageCraft::Memory {

namespace {

struct PatternByte {
    uint8_t value;
    bool wildcard;
};

std::vector<PatternByte> ParsePattern(const char* pattern) {
    std::vector<PatternByte> bytes;
    std::istringstream stream(pattern);
    std::string token;

    while (stream >> token) {
        if (token == "?" || token == "??") {
            bytes.push_back({0, true});
        } else {
            bytes.push_back({
                static_cast<uint8_t>(std::stoul(token, nullptr, 16)),
                false
            });
        }
    }
    return bytes;
}

} // anonymous namespace

uintptr_t PatternScan(const char* moduleName, const char* pattern) {
#ifdef _WIN32
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (!hModule) {
        Logger::Error("PatternScan: module '{}' not found", moduleName);
        return 0;
    }

    auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
    auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<uintptr_t>(hModule) + dosHeader->e_lfanew);

    auto sectionCount = ntHeaders->FileHeader.NumberOfSections;
    auto section = IMAGE_FIRST_SECTION(ntHeaders);

    auto patternBytes = ParsePattern(pattern);
    if (patternBytes.empty()) return 0;

    // Scan all executable sections
    for (WORD i = 0; i < sectionCount; i++, section++) {
        if (!(section->Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;

        auto base = reinterpret_cast<uintptr_t>(hModule) + section->VirtualAddress;
        auto size = section->Misc.VirtualSize;

        for (uintptr_t j = 0; j < size - patternBytes.size(); j++) {
            bool found = true;
            for (size_t k = 0; k < patternBytes.size(); k++) {
                if (patternBytes[k].wildcard) continue;
                if (*reinterpret_cast<uint8_t*>(base + j + k) != patternBytes[k].value) {
                    found = false;
                    break;
                }
            }
            if (found) {
                Logger::Debug("PatternScan: found '{}' at {:X}", pattern, base + j);
                return base + j;
            }
        }
    }

    Logger::Warn("PatternScan: pattern '{}' not found in '{}'", pattern, moduleName);
    return 0;
#else
    (void)moduleName;
    (void)pattern;
    return 0;
#endif
}

uintptr_t ResolveRelative(uintptr_t instrAddr, int operandOffset, int instrLen) {
    auto relativeOffset = *reinterpret_cast<int32_t*>(instrAddr + operandOffset);
    return instrAddr + instrLen + relativeOffset;
}

} // namespace StorageCraft::Memory
