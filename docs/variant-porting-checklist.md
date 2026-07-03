# Variant Porting Checklist

Use this checklist whenever adding a new Rogue engine variant.

## Provenance And Distribution

- Confirm the source license allows redistribution in source and binary form.
- Vendor the source under `variants/<variant-id>/`.
- Preserve the upstream license text in the variant folder.
- Add a short `README-RogueTiles.txt` with upstream URL, source archive/hash when available, and redistribution notes.

## Variant Registry

- Add the variant to `variant.c` with release year, era, lineage, license, distribution, tile support, summary, and gameplay-difference feature notes.
- Keep the game picker descriptions about the game version itself, not RogueTiles features.
- Keep startup ordering by historical release year unless the default needs to remain first internally.

## Runtime Bridge

- Prefix variant symbols so globals and functions do not collide with other embedded engines.
- Add bridge functions for hero position, map dimensions, level number, status line data, message text, visible map glyphs, terrain flags, monsters, and objects.
- Route `--More--`, inventory/list prompts, and selection prompts through the shared frontend input path.
- Smoke test both `--tiles-smoke --variant <variant-id>` and manual tile mode.

## Tile Mapping

- Keep terrain, traps, objects, and player roles shared unless the variant changes their glyph semantics.
- Keep map-visible item categories generic unless the game has already revealed the identity to the player.
- If the variant monster alphabet differs from Rogue 5.4.4, add a complete A-Z section to `assets/rltiles/rogue-rltiles-map.json`:

```json
"variantMonsters": {
  "<variant-id>": {
    "A": { "name": "variant monster name", "atlas": "rltiles_key" }
  }
}
```

- Prefer exact RLTiles atlas keys. If no exact key exists, use the closest visual fit and add a `note`.
- Update or confirm `scripts/validate-rltiles.ps1` verifies 26 entries for every variant.
- Confirm `tile_picker` exposes `monster.<variant-id>.<letter>` roles so custom tile packs can override variant-specific monsters.

## GUI Coverage

Audit these commands before removing terminal dependence for the variant:

- `?` command help and `? *` full help
- `/` identify visible symbol
- `i`, `I`, and all item prompts such as wield, wear, quaff, read, eat, throw, zap, call, drop
- `D` discovered item list
- `o` options
- special prompts such as genocide monster selection
- save, restore, quit, death, score, and win screens
- long text/list windows that need scrolling

## Validation

Run:

```powershell
python tests\test_variant_metadata.py
python tests\test_variant_tile_mapping.py
powershell -ExecutionPolicy Bypass -File scripts\validate-rltiles.ps1
powershell -ExecutionPolicy Bypass -File scripts\build-windows-native.ps1 -Tiles
native-build\rogue54.exe --tiles-smoke --variant <variant-id>
```
