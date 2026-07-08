# Android Graphics Touch Controls Design

## Goal

Create an Android build of RogueTiles that lives alongside the existing Windows build, uses the graphical tile frontend only, excludes the tile map editor, and presents a touch-first control layout inspired by the provided screenshot.

## Scope

This design covers the first playable Android slice:

- Add an Android build target beside the Windows packaging path.
- Launch directly into graphical tile mode.
- Package game assets and default tilepacks with the APK.
- Provide a mobile layout with a compact status area, framed dungeon view, message log, and large touch controls.
- Support an 8-direction movement grid plus a small action row for core commands.

This design does not include:

- The tile map editor or tile picker UI on Android.
- Runtime tilepack editing on Android.
- A redesigned game rules engine.
- A full Android-native rewrite of the Allegro renderer.
- Cloud saves, achievements, monetization, controller support, or alternate input modes.

## Architecture

The Android build should preserve the current C game and Allegro tile renderer. The Android project will live in a new `android/` directory and build a native shared library from the existing RogueTiles sources. This keeps the Windows scripts and release package intact while letting Android have its own Gradle/CMake configuration.

The mobile build should force graphical tile mode at startup. Android users should never see the curses terminal path or the desktop tile editor. Existing gameplay input already converges into single-character Rogue commands through `rogue_frontend_readchar()`, so the Android controls should emit those same command characters instead of changing command handling in the game core.

## Platform Layout

The top-level repo will support two packaging paths:

- Windows: existing `scripts/build-windows-native.ps1`, `scripts/package-windows.ps1`, and `dist/RogueTiles-windows-x64.zip`.
- Android: new Gradle/CMake project under `android/`, producing a debug APK first and a release APK/AAB later.

The Android project should load Allegro and the RogueTiles native library through an Android activity. It should package the required assets from:

- `assets/`
- `tilepacks/default/`
- variant manuals and metadata needed by the graphical frontend
- generated tile mapping files compiled into the native library

Android should not package:

- `tile_picker/`
- `TilePicker.exe`
- Windows DLLs
- Windows release zip artifacts

## Mobile UI

The mobile layout should resemble the reference image in structure and density:

- A dark, pixel-art-friendly screen.
- A compact top status strip with player/variant identity, floor, HP, XP, and a few key stats.
- A framed dungeon viewport using the existing tile renderer.
- A small message log below the dungeon.
- A large bottom control panel.

The first version can draw the HUD and buttons in the Allegro frontend. This avoids introducing a second UI toolkit and keeps game rendering and command input in one loop.

The bottom control panel should reserve stable touch targets:

- Top row: northwest, north, northeast.
- Middle row: west, center, east.
- Bottom row: southwest, south, southeast.
- Center button: visible as `Use`, mapped to wait (`.`) in the first slice.
- Action row: Attack, Wait, Look, Down.

For the first implementation, the movement grid and action row only need to send Rogue command chars. Labels can be polished later, but the target regions must be large enough for thumbs and stable across common phone aspect ratios.

## Command Mapping

The Android touch buttons should map to the same command characters as keyboard movement:

| Button | Command |
| --- | --- |
| Northwest | `y` |
| North | `k` |
| Northeast | `u` |
| West | `h` |
| Center | `.` |
| East | `l` |
| Southwest | `b` |
| South | `j` |
| Southeast | `n` |
| Attack | no game command in the first slice; direct combat remains movement into enemies |
| Wait | `.` |
| Look | `/` |
| Down | `>` |

The first playable Android build should prioritize movement, wait, look, and descend. Attack should not invent new combat behavior. Rogue combat already happens by moving into enemies, so the Attack button is visible for layout parity but disabled until a later directional attack intent model exists.

## Input Handling

The current Allegro frontend listens for keyboard events and returns command chars from `rogue_allegro_readchar()`. Android touch handling should add a parallel command source:

1. Install Allegro touch input when available.
2. Register the touch event source with the existing event queue.
3. Convert touch begin/up events inside button bounds into pending command chars.
4. Return pending command chars through the same `rogue_allegro_readchar()` path used by keyboard input.

Movement repeat should be conservative in the first build. A tap should emit one move. Press-and-hold repeat can be added after the first APK is playable and tested on device.

## Assets And Storage

Android asset loading needs a platform abstraction because the desktop code uses relative filesystem paths for assets and tilepacks. The first Android build should make bundled assets readable through the Allegro Android/APK file interface or copy required assets to an app-accessible directory at startup.

Settings and save files must use app-writable storage, not the APK asset area. The Android path should provide helper functions for:

- bundled read-only assets
- app-writable settings
- app-writable score/save files

## Error Handling

Startup failures should show a clear in-app message when possible:

- missing Allegro initialization
- missing tile atlas
- missing default tilepack
- failed native library load

If the graphical frontend cannot start on Android, the app should exit with a visible failure message rather than falling back to curses.

## Testing

The first implementation should include tests or checks for:

- Android source list includes the same required game files as the Windows tile build, minus Windows-only packaging.
- Touch button hit testing maps stable rectangles to expected command chars.
- Desktop keyboard command mapping remains unchanged.
- Existing Windows tile build still compiles after shared frontend changes.
- APK builds in debug mode.

Manual smoke testing should verify:

- APK installs on a connected Android device or emulator.
- App launches directly into graphical tile mode.
- Dungeon renders with bundled tiles.
- Movement buttons move in all 8 directions.
- Wait, Look, and Down send the expected commands.
- Windows build still works after Android additions.

## Open Implementation Notes

Allegro Android integration should follow current Allegro Android documentation. The likely path is an Android activity that extends Allegro's activity class, loads Allegro shared libraries and the RogueTiles native library, and calls the existing C `main()` through Allegro's Android startup path.

The Android build should start as a debug-only developer build. Release signing, Play Store packaging, and final mobile visual polish should happen after the first playable APK is proven on device.
