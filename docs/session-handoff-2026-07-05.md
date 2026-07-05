# RogueTiles Session Handoff - 2026-07-05

## Current Branch

- Branch: `codex/rogue36-variant`
- Remote: `origin https://github.com/harminoff/rogue.git`
- Latest pushed commit: `8e55790 feat: add variant tile picker overrides`
- Prior pushed commit: `d547b09 feat: add Super-Rogue variant`

## Recently Added

- Added Super-Rogue 9.0.1 as a selectable variant with extended status rows, menus, guides, and tile rendering.
- Added Global / Shared versus per-variant tile picker role support.
- Added variant-specific tile role sections:
  - `variantTerrain.srogue90`
  - `variantTraps.srogue90`
  - existing `variantMonsters.<variant-id>`
- Added Super-Rogue map-feature roles for magic pool, trading post, revealed secret door, maze trap, and trap glyphs.
- Updated generated C mapping and runtime tilepack mappings so in-game rendering and F10 tilepack switching can resolve the new variant roles.
- Kept visible item categories generic to avoid revealing hidden potion, scroll, ring, wand, weapon, or armor identity.

## Important Files

- `assets/rltiles/rogue-rltiles-map.json`: source semantic mapping.
- `tile_picker/role_catalog.py`: canonical picker/generator role catalog.
- `tile_picker/build_picker.py`: static picker data generation.
- `tile_picker/generate_tile_mapping.py`: generated C mapping tables.
- `tile_picker/serve_picker.py`: picker apply/save endpoints.
- `tile_picker/tilepack_writer.py`: default/custom runtime tilepack writing.
- `tile_picker/index.html`: picker UI filters.
- `tiles.c`: runtime semantic cell descriptors and variant tile lookup.
- `generated/rogue_tile_mapping.c` / `.h`: generated tables committed to repo.
- `tilepacks/default/mapping.json` and `tilepacks/active/mapping.json`: regenerated runtime mappings.
- `docs/variant-porting-checklist.md`: update this before adding more variants.

## Verified Commands

```powershell
powershell -ExecutionPolicy Bypass -File scripts\validate-rltiles.ps1
python -m py_compile tile_picker\role_catalog.py tile_picker\build_picker.py tile_picker\generate_tile_mapping.py tile_picker\serve_picker.py tile_picker\tilepack_writer.py
python tests\test_tile_picker_generation.py
python tests\test_tilepack_writer.py
python tests\test_variant_tile_mapping.py
powershell -ExecutionPolicy Bypass -File scripts\build-windows-native.ps1 -Tiles
native-build\rogue54.exe --tiles-smoke
```

## Next Session Priorities

- Manually test the tile picker filters:
  - Global / Shared
  - Rogue 3.6.2 Overrides
  - Rogue 5.2.1 Overrides
  - Super-Rogue 9.0.1 Overrides
- In Super-Rogue tile mode, try to encounter or wizard/debug-spawn the new visible feature roles:
  - magic pool (`"`)
  - trading post (`^`)
  - maze trap (`\`)
  - trapdoor, arrow, sleeping gas, bear, teleport, and poison dart traps
- Confirm custom tilepacks can assign the new roles and that unassigned custom roles fall back gracefully instead of rendering black.
- Decide whether to add variant-specific role support for Rogue 3.6.2 or Rogue 5.2.1 traps beyond the shared trap table.
- Package to `dist\RogueTiles` after manual picker/runtime checks if the user wants a fresh downloadable build.

## Notes For Future Variants

- Start from `docs/variant-porting-checklist.md`.
- Use global/shared roles unless a glyph has different meaning in that variant.
- Add variant terrain/trap/object roles only for visible map semantics, not hidden item subtype identity.
- If a future variant adds new item categories that are visible as new glyphs, add `variantObjects.<variant-id>` support using the same pattern as `variantTerrain` and `variantTraps`.
