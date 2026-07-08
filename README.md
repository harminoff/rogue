# RogueTiles

RogueTiles is a Windows-friendly Rogue collection with an optional Allegro 5 tile frontend, custom tile sets, visual settings, and a packaged tile picker. The classic terminal Rogue path is still present, but the downloadable build is meant to be extract-and-play.

Original Rogue was created by Michael Toy, Ken Arnold, and Glenn Wichman. This fork vendors public-domain RL Tiles assets from `statico/rltiles` and keeps attribution in `assets/rltiles/ATTRIBUTION.txt`.

## Download And Play

1. Open the latest GitHub Release.
2. Download `RogueTiles-windows-x64.zip`.
3. Extract the zip.
4. Run `RogueTiles.exe`.
5. Pick a Rogue version from the startup menu.

The release zip includes the game executable, required DLLs, RL Tiles assets, runtime tile packs, variant license/provenance files, and `TilePicker.exe`.

## Rogue Versions

`RogueTiles.exe` includes a startup version picker.

- **Rogue 5.4.4**: the default RogueTiles ruleset, based on the later classic Unix lineage with the broadest command/menu surface in this collection.
- **Rogue 5.2.1**: bundled from the BSD-style restoration source archive. This first adapter runs from the same executable and uses the shared tile frontend for the dungeon, status bar, camera, and input path.
- **Rogue 3.6.2**: bundled from the BSD-style early public Unix source archive. This version is much closer to the first widely released Rogue experience, with an older monster table, leaner command set, and bundled period manual/guide text.
- **Super-Rogue 9.0.1**: bundled from the redistributable Super-Rogue source archive. This expanded variant adds a wider dungeon/status layout, extended attributes, pack volume/carry stats, extra commands such as dip, and a bundled Super-Rogue tutorial guide.

The picker shows each version's era, lineage, license/distribution status, tile support, and feature notes before you start.

Developer note: new engine variants should follow the checklist in `docs/variant-porting-checklist.md`, including variant-specific monster tile mappings and GUI menu coverage.

## Controls

Core Rogue controls still work:

- Arrow keys or numpad: move
- `h j k l y u b n`: vi movement
- `.`: wait
- `i`: inventory
- `w`: wield weapon
- `W`: wear armor
- `q`: quaff potion
- `r`: read scroll
- `e`: eat food
- `t`: throw item
- `z`: zap wand or staff
- `?`: command help
- `/`: identify a visible symbol
- `Q`: quit

Tile frontend controls:

- `F1`: read bundled manuals and guides
- `F10`: choose tile set or switch to in-window ASCII glyph mode
- `F11`: toggle fullscreen
- `+` / `-`: zoom map in or out
- `0`: reset map zoom
- `F12`: open visual settings

## Visual Features

Open the settings menu with `F12`.

- Side Panel Log: moves the message log from the bottom area into a right panel.
- Stylized Log: color-codes and separates log messages.
- Stylized Bottom Bar: applies the styled status treatment to the bottom status bar.
- Blood Spatter: adds visual-only blood marks on walkable tiles after combat.
- Shader Settings: opens optional visual shader effects.
- Wall Thickness: cycles wall rendering from thin edges up to full tile walls.
- Enemy Health Overlay: draws a compact HP bar and stats above visible, undisguised enemies.

Shader settings:

- Dungeon Gloom: adds a vignette-like dungeon darkness pass.
- Damage Flash: flashes the screen when the player takes damage.
- Low HP Pulse: adds a low-health warning pulse.
- Pixel Sharpen: sharpens the rendered scene.
- Posterize: reduces color levels for a chunkier pixel-art look.
- CRT Effect: cycles Off/Subtle/Balanced/Dramatic CRT-style scanlines, vignette, curvature, and color separation.

All visual settings are optional and do not change Rogue gameplay rules.

## Tile Sets And Tile Picker

Run `TilePicker.exe` from the release folder to edit tile mappings.

The picker lets you:

- Load the included RL Tiles sheet.
- Upload additional tile sheets.
- Append more than one tile set.
- Hide blank tiles while browsing.
- Assign tiles to Rogue terrain, items, player, and monsters.
- Assign variant-specific monster tiles when a Rogue version uses a different A-Z monster table.
- Save tile packs that `RogueTiles.exe` can use at runtime.

After saving a tile pack, run the game and press `F10` to select it.

## Packaged Files

The release package is laid out like this:

```text
RogueTiles/
  RogueTiles.exe
  TilePicker.exe
  README.md
  LICENSE.TXT
  assets/
  tilepacks/
  tile_picker/
  variants/
  *.dll
```

## Building Locally On Windows

The native Windows build uses MSYS2 MinGW64, ncurses, and Allegro 5.

Install MSYS2 dependencies:

```powershell
C:\msys64\usr\bin\pacman.exe -S --needed `
  mingw-w64-x86_64-gcc `
  mingw-w64-x86_64-make `
  mingw-w64-x86_64-pkgconf `
  mingw-w64-x86_64-ncurses `
  mingw-w64-x86_64-allegro
```

Build the game:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-windows-native.ps1 -Tiles
```

Run from the build folder:

```powershell
native-build\rogue54.exe --tiles
```

Build the tile picker executable:

```powershell
python -m pip install pyinstaller
powershell -ExecutionPolicy Bypass -File scripts\build-tile-picker-exe.ps1
```

Create the release folder and zip:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\package-windows.ps1 -Zip
```

The zip is written to:

```text
dist\RogueTiles-windows-x64.zip
```

## Building Locally For Android

The Android build is a graphics-only developer APK. It lives beside the Windows build and does not include `TilePicker.exe` or the desktop tile editor.

Before building, add an Android Gradle wrapper under `android\` and place the Allegro Android artifacts listed in `android\vendor\allegro\README.md`.

Build the debug APK:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-android-debug.ps1
```

The debug APK is written to:

```text
android\app\build\outputs\apk\debug\app-debug.apk
```

The first Android slice uses the default RogueTiles ruleset, bundled default tiles, and touch controls for movement, wait, look, and descend.

## GitHub Release Builds

This repo includes a Windows release workflow at `.github/workflows/release-windows.yml`.

It runs on:

- Manual `workflow_dispatch`
- Version tags matching `v*`

On a tag such as `v0.1.0-alpha`, the workflow builds and uploads `RogueTiles-windows-x64.zip` to the GitHub Release. That zip is the file to upload to itch.io.

Current release highlights:

- Adds Super-Rogue 9.0.1 as a selectable game variant with tile rendering, extended HUD stats, GUI menu coverage, and bundled guide text.
- Improves in-game manual reading with a taller F1 reader, chapter selection, and cleaner formatting for nroff-style guide files.
- Expands custom tile pack support across variants, including variant-specific monster assignments in the tile picker.
- Fixes several tile frontend polish issues, including Super-Rogue wide-map rendering, hidden object bleed-through, missing actor fallbacks, shader rendering, blood layering, and packaged runtime assets.

## Developer Checks

Useful validation commands:

```powershell
python tests\test_combat_damage_log.py
python tests\test_variant_metadata.py
python tests\test_variant_tile_mapping.py
python tests\test_tilepack_writer.py
python tests\test_tile_picker_packaged.py
python tests\test_tile_picker_generation.py
powershell -ExecutionPolicy Bypass -File scripts\validate-rltiles.ps1
powershell -ExecutionPolicy Bypass -File scripts\build-windows-native.ps1 -Tiles
```

## License And Attribution

Rogue is licensed under a BSD-style license. See `LICENSE.TXT`.

Bundled in-game manuals and guides, including `A Guide to the Dungeons of
Doom`, are taken from the Rogue source distribution and covered by the same
BSD-style redistribution terms.

Original Rogue copyright:

- Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman

Additional source history includes work by:

- Nicholas J. Kisseberth
- David Burren

RL Tiles assets are public domain and attributed under `assets/rltiles/ATTRIBUTION.txt`.
