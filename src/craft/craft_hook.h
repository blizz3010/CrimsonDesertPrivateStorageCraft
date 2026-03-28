#pragma once

namespace StorageCraft {

// Hooks the game's crafting execution path using SafetyHookMid.
//
// Approach (modeled on CrimsonDesert-player-status-modifier):
//   1. AOB scan for the crafting confirmation instruction
//   2. Install mid-function hook at the material consumption point
//   3. Read recipe requirements from registers at hook site
//   4. Replace material consumption with our inventory-first logic
//
// The mid-function hook fires at the exact instruction where materials
// are deducted, giving us access to the relevant registers (recipe ptr,
// item ID, quantity) via SafetyHookContext.
namespace CraftHook {
    void Install();
    void Uninstall();
}

} // namespace StorageCraft
