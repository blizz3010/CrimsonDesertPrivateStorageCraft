#pragma once

#include <cstdint>

namespace StorageCraft::Memory {

// Scan a module's .text section for a byte pattern with '?' wildcards.
// Pattern format: "48 8B 05 ? ? ? ? 48 85 C0 74"
// Returns 0 if not found.
uintptr_t PatternScan(const char* moduleName, const char* pattern);

// Resolve a RIP-relative address (common in x64 instructions).
// instrAddr: address of the instruction containing the relative operand
// operandOffset: byte offset from instrAddr to the 4-byte relative operand
// instrLen: total length of the instruction
uintptr_t ResolveRelative(uintptr_t instrAddr, int operandOffset, int instrLen);

// Read a value at an offset from a base pointer, with null checks.
template<typename T>
T ReadOffset(void* base, ptrdiff_t offset) {
    if (!base) return T{};
    return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(base) + offset);
}

// Write a value at an offset from a base pointer.
template<typename T>
void WriteOffset(void* base, ptrdiff_t offset, const T& value) {
    if (!base) return;
    *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(base) + offset) = value;
}

} // namespace StorageCraft::Memory
