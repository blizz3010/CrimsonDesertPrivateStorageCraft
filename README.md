# CrimsonDesertPrivateStorageCraft

A Crimson Desert mod that lets you craft using materials from your private storage container without manually moving items to your inventory first.

## Features

- **Combined Material Counts** - Crafting UI shows materials from both inventory and linked private storage
- **Inventory-First Consumption** - Materials are consumed from inventory before pulling from storage
- **Toggle On/Off** - Press F9 (configurable) to enable/disable the mod at any time
- **Storage Indicator** - Small chest icon appears next to material counts when items are sourced from storage
- **Range Check** - Storage must be within configurable distance (default: 1500 UE units)
- **Concurrent Access Safety** - Two-phase commit with locking prevents issues when another player accesses storage mid-craft

## How It Works

When you open any crafting station (blacksmith, alchemy table, cooking station, etc.):

1. The mod hooks the crafting UI to display combined material counts (inventory + storage)
2. A chest icon appears next to materials that would need to pull from storage
3. When you confirm a craft, materials are consumed from inventory first, then storage
4. If storage is out of range or locked by another player, only inventory materials are used

## Configuration

The mod creates a `StorageCraft.json` config file next to the DLL on first run:

```json
{
    "enabled": true,
    "toggleKey": "F9",
    "maxStorageDistance": 1500.0,
    "showStorageIcon": true,
    "logLevel": "Info"
}
```

| Setting | Description | Default |
|---------|-------------|---------|
| `enabled` | Whether the mod is active | `true` |
| `toggleKey` | Keybind to toggle on/off (F1-F12, A-Z, INSERT, etc.) | `F9` |
| `maxStorageDistance` | Max distance to storage in UE units (~100 units = 1m) | `1500.0` |
| `showStorageIcon` | Show chest icon on storage-sourced materials | `true` |
| `logLevel` | Log verbosity: Debug, Info, Warn, Error | `Info` |

## Building

### Requirements

- CMake 3.20+
- MSVC (Visual Studio 2022 recommended)
- Windows x64 target

### Build Steps

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The output `StorageCraft.dll` will be in `build/bin/Release/`.

### Running Tests

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTS=ON
cmake --build build --config Release --target StorageCraftTests
cd build && ctest -C Release
```

## Installation

1. Copy `StorageCraft.dll` to your Crimson Desert mods folder
2. Use a DLL injector to load the mod into the game process
3. Press F9 in-game to toggle the feature on/off
4. Press Ctrl+Shift+U to unload the mod

## Project Structure

```
src/
  main.cpp              # DLL entry point, input polling thread
  pch.h                 # Precompiled header
  core/
    hook_manager.*      # MinHook wrapper for function hooking
    memory.*            # Pattern scanning, pointer utilities
    logger.*            # File logging with timestamps
  game/
    game_types.h        # Mirrored UE5 game structures
    inventory_accessor.* # Player inventory read/write
    storage_accessor.*  # Storage container read/write with locking
    material_pool.*     # Unified inventory + storage material view
  craft/
    craft_hook.*        # Hooks ExecuteCraft to intercept material consumption
    material_consumer.* # Two-phase commit consumption with rollback
  ui/
    ui_hook.*           # Hooks crafting UI to show combined counts
    storage_indicator.* # Chest icon overlay on material slots
  config/
    mod_config.*        # JSON config load/save, keybind mapping
test/
  mock_inventory.h      # Mock objects for unit testing
  test_material_pool.cpp
  test_material_consumer.cpp
```

## Edge Cases Handled

- **Storage out of range**: Excluded from material pool; only inventory counts shown
- **Concurrent access**: TryLock prevents reading storage another player is modifying
- **Race condition**: Counts are re-validated at consumption time (two-phase commit)
- **Mod toggled mid-craft**: Toggle only takes effect at the start of the next operation
- **Insufficient combined materials**: Falls through to the original game logic gracefully

## License

This project is provided as-is for educational and modding purposes.
