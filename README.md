# CrimsonDesertPrivateStorageCraft

An ASI plugin mod for Crimson Desert that lets you craft using materials from your private storage container without manually moving items to your inventory first.

## How It Works

Crimson Desert uses Pearl Abyss's proprietary **BlackSpace Engine** (not Unreal Engine). This mod hooks into the game at runtime using SafetyHook mid-function hooks, the same approach used by established CD mods like [CrimsonDesert-player-status-modifier](https://github.com/Orcax-1399/CrimsonDesert-player-status-modifier).

When you open any crafting station:

1. The mod hooks the crafting UI to display combined material counts (inventory + storage)
2. A visual indicator appears next to materials sourced from storage
3. When you confirm a craft, materials are consumed from inventory first, then storage
4. If storage is out of range or locked by another player, only inventory is used

## Features

- **Combined Material Counts** - Crafting UI shows inventory + linked private storage totals
- **Inventory-First Consumption** - Materials consumed from inventory before pulling from storage
- **Toggle On/Off** - Press F9 (configurable) to enable/disable at any time
- **Storage Indicator** - Visual marker on materials that need storage sourcing
- **Range Check** - Storage must be within configurable distance
- **Concurrent Access Safety** - Two-phase commit with locking for multi-player safety
- **ASI Plugin** - Standard `.asi` format, compatible with ASI loaders and mod managers

## Installation

1. Install an ASI loader for Crimson Desert (e.g., Ultimate ASI Loader)
2. Copy `StorageCraft.asi` and `StorageCraft.ini` to your game directory
3. Launch the game - the mod loads automatically
4. Press **F9** to toggle on/off
5. Press **Ctrl+Shift+U** to unload the mod entirely

Compatible with [CrimsonDesert-UltimateModsManager](https://github.com/faisalkindi/CrimsonDesert-UltimateModsManager) for ASI plugin management.

## Configuration

Edit `StorageCraft.ini` next to the `.asi` file:

```ini
[General]
Enabled=1
LogEnabled=1
InitDelayMs=3000

[Keybinds]
ToggleKey=F9

[StorageCraft]
MaxStorageDistance=1500.0
ShowStorageIcon=1
InventoryPriority=1

[Logging]
Level=1
```

| Setting | Description | Default |
|---------|-------------|---------|
| `Enabled` | Whether the mod is active on startup | `1` |
| `InitDelayMs` | Delay before hooking (let game load) | `3000` |
| `ToggleKey` | Keybind to toggle (F1-F12, INSERT, DELETE) | `F9` |
| `MaxStorageDistance` | Max distance to storage in game units | `1500.0` |
| `ShowStorageIcon` | Show indicator on storage-sourced materials | `1` |
| `InventoryPriority` | Consume from inventory before storage | `1` |
| `Level` | Log verbosity: 0=Debug, 1=Info, 2=Warn, 3=Error | `1` |

## Building

### Requirements

- CMake 3.20+
- MSVC (Visual Studio 2022 recommended)
- Windows x64 target
- C++23

### Build Steps

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output: `build/bin/Release/StorageCraft.asi`

### Running Tests

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTS=ON
cmake --build build --config Release --target StorageCraftTests
cd build && ctest -C Release
```

## Technical Architecture

### Hooking Approach

Uses **SafetyHook** (mid-function hooks) - the standard for CD modding. Mid-function hooks insert a callback at a specific x86_64 instruction without replacing the original code flow, giving access to CPU registers at the hook site.

### AOB Pattern Scanning

Game functions are located at runtime via Array-of-Bytes (AOB) pattern scanning. Patterns are byte sequences with `??` wildcards that match specific instructions in the game binary. The scanner checks `.text` section first, then falls back to all executable sections.

**Current pattern status:** The AOB patterns in the source are placeholders. They need to be discovered from the actual game binary using a disassembler (IDA Pro, Ghidra) or debugger (x64dbg, Cheat Engine). See the TODO comments in each hook file for the methodology.

### Project Structure

```
src/
  main.cpp                # ASI entry point, init thread, keybind polling
  pch.h                   # Precompiled header
  core/
    hook_manager.*        # SafetyHook wrapper (mid-function + inline hooks)
    memory.*              # AOB pattern scanner, pointer utilities
    logger.*              # File logging with timestamps
  game/
    game_types.h          # BlackSpace Engine structures (BSItemEntry, BSArray, etc.)
    inventory_accessor.*  # Player inventory read/write
    storage_accessor.*    # Storage container read/write with locking
    material_pool.*       # Unified inventory + storage material view
  craft/
    craft_hook.*          # Mid-function hooks on crafting consumption/check
    material_consumer.*   # Two-phase commit consumption with rollback
  ui/
    ui_hook.*             # Hooks crafting UI count display and slot coloring
    storage_indicator.*   # Tracks which slots use storage materials
  config/
    mod_config.*          # INI config via Windows API
config/
  default_config.ini      # Ships with the mod
test/
  mock_inventory.h        # Mock objects for unit testing
  test_material_pool.cpp
  test_material_consumer.cpp
```

### Dependencies

| Library | Purpose | Source |
|---------|---------|--------|
| [SafetyHook](https://github.com/cursey/safetyhook) | x86_64 mid-function hooking | FetchContent |
| Windows API | INI config, keybinds, module loading | System |

### Edge Cases

- **Storage out of range**: Excluded from material pool; only inventory counts shown
- **Concurrent access**: TryLock prevents reading storage another player is modifying
- **Race condition**: Counts re-validated at consumption time (two-phase commit)
- **Mod toggled mid-craft**: Toggle only takes effect at next operation boundary
- **Exception safety**: All hook callbacks wrapped in SEH `__try/__except` (CD mod convention)

## Contributing

### Finding Real AOB Patterns

The main work needed to make this fully functional:

1. **Install x64dbg** or use Cheat Engine with the game running
2. **Find the crafting consumption instruction**: Set a breakpoint on item count decrements during crafting. The instruction that subtracts materials is the target for `CraftConsume`
3. **Find the crafting check instruction**: The instruction that reads material counts to determine if crafting is possible
4. **Find the UI count update**: The instruction that writes available counts to the crafting panel
5. **Generate AOB patterns**: Record the surrounding bytes, replace variable bytes with `??`
6. **Update the `Patterns` namespace** in `craft_hook.cpp` and `ui_hook.cpp`

Reference: [CrimsonDesert-player-status-modifier](https://github.com/Orcax-1399/CrimsonDesert-player-status-modifier) demonstrates this methodology for stats/damage/items.

## License

This project is provided as-is for educational and modding purposes.
