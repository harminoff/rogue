#!/usr/bin/env python3
"""Build static data for the RogueTiles tile picker."""

from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path
from typing import Any

from .role_catalog import (
    Role,
    all_roles,
    c_glyph_display,
    trap_roles,
    variant_entry,
    variant_monster_entry,
    variant_monster_roles,
    variant_terrain_roles,
    variant_trap_roles,
)


def read_json(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError(f"{path} must contain a JSON object")
    return data


def role_entry(mapping: dict[str, Any], lookup: dict[str, int], role: Role) -> dict[str, Any]:
    variant_id = None
    if role.group in ("variantTerrain", "variantTraps") and role.key.count(".") == 1:
        variant_id, key = role.key.split(".", 1)
        raw = variant_entry(mapping, role.group, variant_id, key)
    elif role.role.startswith("monster.") and role.key.count(".") == 1:
        variant_id, glyph = role.key.split(".", 1)
        raw = variant_monster_entry(mapping, variant_id, glyph)
    elif role.role.startswith("monster."):
        raw = mapping.get("monsters", {}).get(role.key, {})
    else:
        raw = mapping.get(role.group, {}).get(role.key, {})
    raw = raw if isinstance(raw, dict) else {}
    atlas = raw.get("atlas")
    display_group = "monsters" if role.role.startswith("monster.") else role.layer + "s"
    if role.role.startswith("terrain."):
        display_group = "terrain"
    elif role.role.startswith("trap."):
        display_group = "traps"
    elif role.role.startswith("object."):
        display_group = "objects"
    elif role.role.startswith("actor."):
        display_group = "actors"
    return {
        "role": role.role,
        "group": display_group,
        "key": role.key,
        "glyph": raw.get("glyph", c_glyph_display(role.glyph)),
        "scope": "variant" if variant_id else "global",
        "variantId": variant_id,
        "label": role.label,
        "name": raw.get("name", role.name),
        "currentRltilesName": atlas,
        "currentRltilesIndex": lookup.get(str(atlas)) if atlas else None,
    }


def build_data(root: Path) -> dict[str, Any]:
    root = root.resolve()
    assets = root / "assets" / "rltiles"
    atlas = read_json(assets / "rltiles-2d.json")
    mapping = read_json(assets / "rogue-rltiles-map.json")
    names = [str(name) for name in atlas.get("tiles", [])]
    lookup = {name: index for index, name in enumerate(names)}
    columns = int(atlas.get("width", 30))
    tile_size = int(atlas.get("tileSize", 32))

    roles = (
        all_roles()
        + trap_roles()
        + variant_terrain_roles(mapping)
        + variant_trap_roles(mapping)
        + variant_monster_roles(mapping)
    )

    return {
        "version": 1,
        "generatedFrom": {
            "rltilesManifest": "assets/rltiles/rltiles-2d.json",
            "currentMapping": "assets/rltiles/rogue-rltiles-map.json",
        },
        "rogue": {
            "roles": [role_entry(mapping, lookup, role) for role in roles],
        },
        "rltiles": {
            "atlas": {
                "path": "assets/rltiles/rltiles-2d.png",
                "columns": columns,
                "tileWidth": tile_size,
                "tileHeight": tile_size,
            },
            "tiles": [
                {
                    "index": index,
                    "name": name,
                    "column": index % columns,
                    "row": index // columns,
                }
                for index, name in enumerate(names)
            ],
        },
        "currentMapping": mapping,
    }


def write_json(path: Path, data: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")


def build(root: Path) -> dict[str, Any]:
    root = root.resolve()
    out_dir = root / "tile_picker"
    data_dir = out_dir / "data"
    assets_dir = out_dir / "assets"
    data = build_data(root)

    data_dir.mkdir(parents=True, exist_ok=True)
    assets_dir.mkdir(parents=True, exist_ok=True)
    write_json(data_dir / "picker_data.json", data)
    (data_dir / "picker_data.js").write_text(
        "window.ROGUE_TILE_PICKER_DATA = "
        + json.dumps(data, separators=(",", ":"))
        + ";\n",
        encoding="utf-8",
    )
    shutil.copyfile(root / "assets" / "rltiles" / "rltiles-2d.png",
                    assets_dir / "rltiles-2d.png")
    return {
        "roles": len(data["rogue"]["roles"]),
        "rltiles": len(data["rltiles"]["tiles"]),
        "data": str((data_dir / "picker_data.json").relative_to(root)),
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    return parser.parse_args()


def main() -> None:
    result = build(parse_args().root)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
