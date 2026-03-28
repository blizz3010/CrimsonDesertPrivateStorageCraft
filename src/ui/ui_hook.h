#pragma once

namespace StorageCraft {

// Hooks the crafting UI's material count display to show combined
// inventory + storage counts when the mod is enabled.
//
// BlackSpace Engine uses its own UI framework (not UMG/Slate).
// The rendering approach will differ from UE5 mods.
namespace UIHook {
    void Install();
    void Uninstall();
}

} // namespace StorageCraft
