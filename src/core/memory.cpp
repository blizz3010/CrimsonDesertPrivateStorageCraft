#include "core/memory.h"
#include "core/logger.h"
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

// Parse pattern string like "48 8B ?? ?? 74 0A" into byte array.
// Supports both '?' and '??' as wildcards (matching CD mod conventions).
std::vector<PatternByte> ParsePattern(const char* pattern) {
    std::vector<PatternByte> bytes;
    std::istringstream stream(pattern);
    std::string token;

    while (stream >> token) {
        if (token == "?" || token == "??" || token == "*") {
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
        Logger::Error("PatternScan: module '{}' not found", moduleName ? moduleName : "main");
        return 0;
    }

    auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
    auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<uintptr_t>(hModule) + dosHeader->e_lfanew);

    auto sectionCount = ntHeaders->FileHeader.NumberOfSections;
    auto section = IMAGE_FIRST_SECTION(ntHeaders);

    auto patternBytes = ParsePattern(pattern);
    if (patternBytes.empty()) return 0;

    // Two-pass scan: .text first (most likely), then all executable sections
    // This matches the approach in CrimsonDesert-player-status-modifier
    for (int pass = 0; pass < 2; pass++) {
        auto* sec = IMAGE_FIRST_SECTION(ntHeaders);
        for (WORD i = 0; i < sectionCount; i++, sec++) {
            bool isText = (strncmp(reinterpret_cast<const char*>(sec->Name), ".text", 5) == 0);

            if (pass == 0 && !isText) continue;        // First pass: .text only
            if (pass == 1 && isText) continue;          // Second pass: skip .text (already done)
            if (!(sec->Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;

            auto base = reinterpret_cast<uintptr_t>(hModule) + sec->VirtualAddress;
            auto size = sec->Misc.VirtualSize;

            for (uintptr_t j = 0; j + patternBytes.size() <= size; j++) {
                bool found = true;
                for (size_t k = 0; k < patternBytes.size(); k++) {
                    if (patternBytes[k].wildcard) continue;
                    if (*reinterpret_cast<uint8_t*>(base + j + k) != patternBytes[k].value) {
                        found = false;
                        break;
                    }
                }
                if (found) {
                    auto addr = base + j;
                    Logger::Debug("PatternScan: found at {:X} (section: {})",
                                  addr, reinterpret_cast<const char*>(sec->Name));
                    return addr;
                }
            }
        }
    }

    Logger::Warn("PatternScan: pattern not found in '{}'", moduleName ? moduleName : "main");
    return 0;
#else
    (void)moduleName;
    (void)pattern;
    return 0;
#endif
}

uintptr_t PatternScanOffset(const char* moduleName, const char* pattern, int offset) {
    auto addr = PatternScan(moduleName, pattern);
    if (!addr) return 0;
    return addr + offset;
}

uintptr_t ResolveRelative(uintptr_t instrAddr, int operandOffset, int instrLen) {
    auto relativeOffset = *reinterpret_cast<int32_t*>(instrAddr + operandOffset);
    return instrAddr + instrLen + relativeOffset;
}

} // namespace StorageCraft::Memory
