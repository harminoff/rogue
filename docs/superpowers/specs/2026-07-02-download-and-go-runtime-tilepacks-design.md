# Download-And-Go Runtime Tile Packs Design

## Goal

Ship RogueTiles as a download-and-go Windows app where players can use both the game and tile picker without installing Python, MSYS2, Allegro, compilers, or other developer tools.

The packaged app should let a user unzip or install RogueTiles, open the picker, upload or select tiles, save the active tile pack, and launch the graphical game immediately.

## Non-Goals

- Do not require end users to rebuild `rogue54.exe` after changing tiles.
- Do not require a terminal window for normal tile game or picker use.
- Do not remove the developer build scripts; they remain useful for us.
- Do not replace the current Allegro game frontend in this milestone.

## Recommended Approach

Move from compile-time generated tile mappings to runtime tile packs.

The picker writes files under `tilepacks/active/`, and the game loads those files at startup. If the active tile pack is missing, malformed, or references an unreadable image, the game falls back to `tilepacks/default/`.

This keeps all player-facing customization in data files and makes packaging simple: the shipped executable only needs bundled DLLs, assets, default tile pack data, and the picker launcher.

## Packaged Layout

```text
RogueTiles/
  RogueTiles.exe
  TilePicker.exe
  assets/
    fonts/
  tilepacks/
    default/
      tilepack.json
      mapping.json
      tiles.png
    active/
      tilepack.json
      mapping.json
      tiles.png
  dlls...
```

`RogueTiles.exe` launches the game in graphical tile mode by default. A separate developer or compatibility path can still expose the legacy ASCII terminal build, but the user package should prioritize the graphical app.

`TilePicker.exe` opens the picker UI without requiring Python. It edits `tilepacks/active/` and can launch `RogueTiles.exe`.

## Tile Pack Format

`tilepack.json` describes the selected sheet and basic metadata:

```json
{
  "schemaVersion": 1,
  "name": "Default RL Tiles",
  "image": "tiles.png",
  "mapping": "mapping.json",
  "tileWidth": 32,
  "tileHeight": 32,
  "columns": 30
}
```

`mapping.json` maps Rogue semantic role IDs to atlas entries:

```json
{
  "schemaVersion": 1,
  "roles": {
    "terrain.floor": { "index": 0, "name": "dngn_floor" },
    "actor.player": { "index": 123, "name": "player" },
    "monster.H": { "index": 456, "name": "hobgoblin" }
  }
}
```

The role IDs come from the existing Rogue semantic layer, not from atlas names. This preserves hidden information behavior and prevents a tilesheet from becoming gameplay logic.

## Game Runtime Behavior

At startup, the Allegro frontend loads tile-pack metadata in this order:

1. `tilepacks/active/tilepack.json`
2. `tilepacks/default/tilepack.json`
3. Built-in emergency fallback metadata for the vendored RL Tiles asset

The renderer uses the loaded tile width, tile height, and column count when drawing sprites. If a specific role mapping is missing, the renderer draws the existing visible fallback glyph tile and logs the missing role.

The semantic map source remains `rogue_tile_describe_cell()`. Movement, combat, item identity, hidden traps, disguised monsters, saves, and Rogue rules do not move into the tile-pack system.

## Picker Runtime Behavior

The picker UI keeps the current role browser and upload workflow, but its apply action writes runtime files instead of regenerating C.

For RL Tiles selections:

- Copy or reference the bundled RL Tiles image into the active pack.
- Write `tilepack.json`.
- Write `mapping.json`.

For uploaded sheets:

- Copy the uploaded PNG to `tilepacks/active/tiles.png`.
- Persist tile dimensions, column count, display names, and role assignments.
- Write `tilepack.json` and `mapping.json`.

The picker can launch the game after saving the active pack. No build step appears in the user-facing picker.

## Packaging Strategy

Create a packaging script that produces a clean `dist/RogueTiles/` directory from the native tile build.

The package script should:

- Run the native tile build for developers.
- Copy `rogue54.exe` to `RogueTiles.exe`.
- Copy required Allegro and MinGW runtime DLLs.
- Copy fonts and default tile assets.
- Create `tilepacks/default/` from the current RL Tiles mapping.
- Seed `tilepacks/active/` from default on first package creation.
- Include a no-install `TilePicker.exe` or equivalent bundled picker launcher.

The picker launcher will initially be a bundled executable produced from the existing Python picker server with PyInstaller. This is the fastest path because it preserves the current HTML picker, upload flow, save APIs, and game launch APIs while removing the user's Python requirement.

A native or webview wrapper can replace the PyInstaller launcher later without changing the tile-pack format.

## Error Handling

The game should never fail to start solely because a custom tile pack is broken.

Failure cases and behavior:

- Missing active tile pack: load default.
- Invalid active JSON: load default and log the reason.
- Missing custom PNG: load default and log the reason.
- Missing role mapping: draw fallback glyph for that role.
- Invalid tile dimensions or columns: load default.

The picker should validate before saving:

- PNG is readable.
- Tile width, tile height, and columns are positive.
- Every required Rogue role is assigned or intentionally falls back to default.
- Mapping indices are within the tile image bounds.

## Testing

Automated checks:

- Validate default tile pack JSON.
- Validate active tile pack JSON with a sample uploaded sheet.
- Validate every required Rogue role resolves to an atlas index or fallback.
- Smoke test `RogueTiles.exe --tiles-smoke` from the packaged directory.

Manual checks:

- Launch packaged game on a machine without MSYS2 or Python in `PATH`.
- Launch picker from packaged directory.
- Upload a custom sheet, assign a few roles, save, launch game, and confirm the custom tiles appear.
- Corrupt or delete `tilepacks/active/tilepack.json` and confirm the game falls back to default.

## Implementation Phases

1. Add runtime tile-pack loader in C and keep generated mappings as a temporary fallback.
2. Update Allegro renderer to read atlas image path and dimensions from the loaded runtime pack.
3. Update picker apply actions to write `tilepacks/active/` instead of requiring rebuilds.
4. Add package script for `dist/RogueTiles/` with game, assets, DLLs, and tile packs.
5. Bundle the picker as `TilePicker.exe`.
6. Verify the packaged folder works without Python, MSYS2, or repo source files.

## Picker Packaging Decision

Package `TilePicker.exe` with PyInstaller for the first download-and-go release. The executable starts the local picker server, opens the user's browser to the picker UI, and serves only files inside the packaged RogueTiles directory.
