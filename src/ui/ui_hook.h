#pragma once

namespace StorageCraft {

// Hooks the crafting UI's material count display to show combined
// inventory + storage counts when the mod is enabled.
namespace UIHook {
    void Install();
    void Uninstall();
}

} // namespace StorageCraft
