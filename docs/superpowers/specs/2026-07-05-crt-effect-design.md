# CRT Effect Design

## Goal

Add a visual-only CRT postprocess effect to RogueTiles tile mode. The effect should be adjustable through compact intensity modes rather than several independent low-level toggles.

## User Experience

The Shader Settings menu gets one new row:

```text
f) CRT Effect: Off
```

Selecting `f` cycles:

```text
Off -> Subtle -> Balanced -> Dramatic -> Off
```

The modes should mean:

- `Off`: no CRT processing.
- `Subtle`: playable scanlines and a mild vignette.
- `Balanced`: visible scanlines, mild barrel-style sampling, and light RGB separation.
- `Dramatic`: stronger scanlines, vignette, and color separation for a showcase look.

The effect remains visual-only. It must not affect Rogue rules, map state, input, visibility, combat, or tile identity.

## Architecture

The feature should follow the existing shader/settings pattern in `allegro_frontend.c`.

- Add a small CRT mode enum or integer-backed mode in the Allegro settings.
- Persist the mode in `settings.json` with key `crtEffect`.
- Load older settings safely by treating a missing or invalid value as `Off`.
- Add `crtEffect` to the Shader Settings menu and save after cycling.
- Include CRT in `postprocess_enabled()` and `scene_effects_need_bitmap()` through the existing postprocess path.
- Keep status, side panel, text overlays, and death overlays outside the CRT pass, matching the current render order.

The current postprocess implementation has a GLSL source string but performs pixel sharpen and posterize through a CPU bitmap fallback path. CRT should fit that compatibility-first approach:

- Keep the GLSL source updated with CRT uniforms/source logic so the shader definition documents the intended GPU effect.
- Implement the actual first-pass effect in the existing CPU postprocess loop so it works consistently with the current renderer behavior.
- Avoid introducing a new shader object unless a later refactor switches the postprocess path to GPU execution.

## Visual Model

For each pixel in `scene_source_bitmap`, derive normalized screen coordinates and apply mode-dependent parameters:

- Scanlines: darken alternating horizontal rows with a smooth or periodic factor.
- Vignette: darken toward the edges of the scene.
- Curvature feel: for `Balanced` and `Dramatic`, sample from a mildly warped coordinate so edges bend inward.
- RGB separation: for `Balanced` and `Dramatic`, sample red and blue channels with a small horizontal offset while keeping green centered.

The CPU path should clamp all sampled coordinates to the source bitmap bounds and preserve alpha.

## Testing

Use test-first changes in `tests/test_combat_damage_log.py`.

Add focused assertions that:

- `crt_effect_mode` or equivalent state exists.
- `crtEffect` is loaded and saved in settings JSON.
- The Shader Settings menu shows `CRT Effect`.
- A cycle helper or equivalent logic advances through `Off`, `Subtle`, `Balanced`, and `Dramatic`.
- `postprocess_enabled()` includes the CRT setting.
- The postprocess implementation includes CRT-specific scanline, vignette, curvature, and RGB separation logic.
- `--tiles-shader-smoke` enables CRT so diagnostic images exercise the new effect.

After implementation, run the focused test file, then the native build and tile smoke path if available.

## Verification

Expected verification commands:

```powershell
python tests\test_combat_damage_log.py
powershell -ExecutionPolicy Bypass -File scripts\build-windows-native.ps1 -Tiles
native-build\rogue54.exe --tiles-shader-smoke --variant rogue54
```

If the shader smoke path writes diagnostic bitmaps, confirm the files are produced and that the run exits successfully.

## Out Of Scope

- No separate toggles for scanlines, mask, curvature, or vignette.
- No gameplay changes.
- No new tile picker behavior.
- No changes to variant-specific tile mapping.
- No GPU-only dependency for the first pass.
