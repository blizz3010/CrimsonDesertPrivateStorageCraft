#pragma once

namespace StorageCraft {

// Hooks the game's craft execution function to intercept material consumption.
// When the mod is enabled, materials are sourced from both inventory and storage.
namespace CraftHook {
    void Install();
    void Uninstall();
}

} // namespace StorageCraft
