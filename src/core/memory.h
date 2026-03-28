#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace StorageCraft::Memory {

// Scan the main game executable for a byte pattern with '?' wildcards.
// Pattern format: "48 8B 05 ?? ?? ?? ?? 48 85 C0 74"
// Uses '??' for wildcard bytes (matching CD modding conventions).
// Scans .text section first, then falls back to all executable sections.
// Returns 0 if not found.
uintptr_t PatternScan(const char* moduleName, const char* pattern);

// Convenience: scan with an offset applied to the result.
// Useful when the AOB is found N bytes before the instruction you want to hook.
uintptr_t PatternScanOffset(const char* moduleName, const char* pattern, int offset);

// Resolve a RIP-relative address (common in x64 instructions).
uintptr_t ResolveRelative(uintptr_t instrAddr, int operandOffset, int instrLen);

// Validate a pointer is in reasonable game memory range.
// The player-status-modifier uses 0x10000000 as minimum threshold.
inline bool IsValidPtr(uintptr_t ptr) {
    return ptr > 0x10000000;
}
inline bool IsValidPtr(void* ptr) {
    return IsValidPtr(reinterpret_cast<uintptr_t>(ptr));
}

// Read a value at an offset from a base pointer, with null/validity checks.
template<typename T>
T ReadOffset(void* base, ptrdiff_t offset) {
    if (!IsValidPtr(base)) return T{};
    return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(base) + offset);
}

// Write a value at an offset from a base pointer.
template<typename T>
void WriteOffset(void* base, ptrdiff_t offset, const T& value) {
    if (!IsValidPtr(base)) return;
    *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(base) + offset) = value;
}

} // namespace StorageCraft::Memory
