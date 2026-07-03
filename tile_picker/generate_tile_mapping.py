#!/usr/bin/env python3
"""Generate C tile mapping tables from RLTiles mapping or picker profiles."""

from __future__ import annotations

import argparse
import base64
import json
import re
from pathlib import Path
from typing import Any

from .role_catalog import MONSTER_NAMES, Role, all_roles, role_by_id, variant_monster_roles


DEFAULT_ACTIVE_SOURCE = Path("tile_picker/data/active_tile_source.json")
DEFAULT_CUSTOM_PROFILE = Path("tile_picker/data/applied_custom_tilemap_profile.json")
GENERATED_DIR = Path("generated")
HEADER_NAME = "rogue_tile_mapping.h"
SOURCE_NAME = "rogue_tile_mapping.c"


def read_json(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError(f"{path} must contain a JSON object")
    return data


def atlas_lookup(root: Path) -> dict[str, int]:
    atlas = read_json(root / "assets" / "rltiles" / "rltiles-2d.json")
    return {str(name): index for index, name in enumerate(atlas.get("tiles", []))}


def c_string(value: Any) -> str:
    if value is None:
        return "NULL"
    text = str(value)
    text = text.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{text}"'


def role_atlas_from_mapping(mapping: dict[str, Any], role: Role) -> str | None:
    if role.role.startswith("monster.") and role.key.count(".") == 1:
        variant_id, glyph = role.key.split(".", 1)
        entry = mapping.get("variantMonsters", {}).get(variant_id, {}).get(glyph, {})
    elif role.role.startswith("monster."):
        entry = mapping.get("monsters", {}).get(role.key, {})
    else:
        entry = mapping.get(role.group, {}).get(role.key, {})
    if isinstance(entry, dict):
        atlas = entry.get("atlas")
        return str(atlas) if atlas else None
    return None


def selection_for_role(profile: dict[str, Any], role: Role) -> tuple[str | None, int]:
    tiles = profile.get("tiles", {})
    if not isinstance(tiles, dict):
        return None, -1
    selected = tiles.get(role.role)
    if isinstance(selected, dict):
        index = selected.get("index")
        name = selected.get("name")
        return (str(name) if name is not None else None,
                int(index) if isinstance(index, int) else -1)
    if isinstance(selected, int):
        return None, selected
    return None, -1


def custom_source_from_profile(root: Path, profile: dict[str, Any]) -> dict[str, Any]:
    source = profile.get("source")
    if not isinstance(source, dict):
        raise ValueError("Custom profile must include a source object")

    generated_assets = root / "assets" / "generated"
    generated_assets.mkdir(parents=True, exist_ok=True)

    path = str(source.get("path") or "assets/generated/custom-tiles.png")
    data_url = source.get("dataUrl")
    if isinstance(data_url, str) and data_url.startswith("data:image/"):
        match = re.match(r"data:image/[^;]+;base64,(?P<data>.+)$", data_url)
        if not match:
            raise ValueError("Custom tilemap dataUrl is not a base64 image")
        image_bytes = base64.b64decode(match.group("data"))
        (root / path).parent.mkdir(parents=True, exist_ok=True)
        (root / path).write_bytes(image_bytes)

    return {
        "path": path.replace("\\", "/"),
        "columns": int(source.get("columns") or 1),
        "tileWidth": int(source.get("tileWidth") or 32),
        "tileHeight": int(source.get("tileHeight") or source.get("tileWidth") or 32),
    }


def rltiles_profile(root: Path) -> tuple[dict[str, Any], dict[str, Any]]:
    mapping = read_json(root / "assets" / "rltiles" / "rogue-rltiles-map.json")
    lookup = atlas_lookup(root)
    profile: dict[str, Any] = {"tiles": {}}
    for role in all_roles() + variant_monster_roles(mapping):
        atlas = role_atlas_from_mapping(mapping, role)
        if atlas is None:
            continue
        profile["tiles"][role.role] = {"index": lookup.get(atlas, -1), "name": atlas}
    source = {
        "path": "assets/rltiles/rltiles-2d.png",
        "columns": int(read_json(root / "assets" / "rltiles" / "rltiles-2d.json").get("width", 30)),
        "tileWidth": 32,
        "tileHeight": 32,
    }
    return profile, source


def configured_variant_monster_roles(root: Path) -> list[Role]:
    mapping_path = root / "assets" / "rltiles" / "rogue-rltiles-map.json"
    if not mapping_path.exists():
        return []
    return variant_monster_roles(read_json(mapping_path))


def active_custom_profile(root: Path) -> Path | None:
    active_path = root / DEFAULT_ACTIVE_SOURCE
    if active_path.exists():
        active = read_json(active_path)
        if active.get("mode") == "custom":
            return root / str(active.get("profile") or DEFAULT_CUSTOM_PROFILE)
        if active.get("mode") == "rltiles":
            return None
    profile_path = root / DEFAULT_CUSTOM_PROFILE
    return profile_path if profile_path.exists() else None


def write_generated(root: Path, profile: dict[str, Any], source: dict[str, Any], mode: str) -> dict[str, Any]:
    out_dir = root / GENERATED_DIR
    out_dir.mkdir(parents=True, exist_ok=True)
    header_path = out_dir / HEADER_NAME
    source_path = out_dir / SOURCE_NAME
    roles = all_roles()
    monsters = [(glyph, MONSTER_NAMES[glyph]) for glyph in sorted(MONSTER_NAMES)]
    variant_monsters = configured_variant_monster_roles(root)

    glyph_rows = []
    monster_rows = []
    variant_monster_rows = []
    by_id = role_by_id()
    for role in roles:
        if role.role.startswith("monster."):
            continue
        atlas_key, atlas_index = selection_for_role(profile, role)
        glyph_rows.append(
            f"    {{ {role.glyph}, {role.c_layer}, {c_string(role.role)}, "
            f"{c_string(atlas_key)}, {atlas_index}, {c_string(role.name)} }}"
        )
    for glyph, monster_name in monsters:
        role = by_id[f"monster.{glyph}"]
        atlas_key, atlas_index = selection_for_role(profile, role)
        monster_rows.append(
            f"    {{ '{glyph}', {c_string(role.role)}, {c_string(atlas_key)}, "
            f"{atlas_index}, {c_string(monster_name)} }}"
        )
    for role in variant_monsters:
        variant_id, glyph = role.key.split(".", 1)
        atlas_key, atlas_index = selection_for_role(profile, role)
        variant_monster_rows.append(
            f"    {{ {c_string(variant_id)}, '{glyph}', {c_string(role.role)}, "
            f"{c_string(atlas_key)}, {atlas_index}, {c_string(role.name)} }}"
        )
    glyph_table = ",\n".join(glyph_rows)
    monster_table = ",\n".join(monster_rows)
    if variant_monster_rows:
        variant_monster_table = ",\n".join(variant_monster_rows)
    else:
        variant_monster_table = "    { NULL, '\\0', NULL, NULL, -1, NULL }"
    variant_monster_count = len(variant_monster_rows)

    header_path.write_text(
        """/*
 * Generated by tile_picker/generate_tile_mapping.py. Do not edit by hand.
 */

#ifndef ROGUE_GENERATED_TILE_MAPPING_H
#define ROGUE_GENERATED_TILE_MAPPING_H

#include "../tiles.h"

typedef struct rogue_generated_tile_mapping {
    char glyph;
    ROGUE_TILE_LAYER layer;
    const char *role;
    const char *atlas_key;
    int atlas_index;
    const char *name;
} ROGUE_GENERATED_TILE_MAPPING;

typedef struct rogue_generated_monster_mapping {
    char glyph;
    const char *role;
    const char *atlas_key;
    int atlas_index;
    const char *name;
} ROGUE_GENERATED_MONSTER_MAPPING;

typedef struct rogue_generated_variant_monster_mapping {
    const char *variant_id;
    char glyph;
    const char *role;
    const char *atlas_key;
    int atlas_index;
    const char *name;
} ROGUE_GENERATED_VARIANT_MONSTER_MAPPING;

extern const ROGUE_GENERATED_TILE_MAPPING rogue_tile_glyph_mappings[];
extern const int rogue_tile_glyph_mapping_count;
extern const ROGUE_GENERATED_MONSTER_MAPPING rogue_tile_monster_mappings[];
extern const int rogue_tile_monster_mapping_count;
extern const ROGUE_GENERATED_VARIANT_MONSTER_MAPPING rogue_tile_variant_monster_mappings[];
extern const int rogue_tile_variant_monster_mapping_count;

const char *rogue_tile_atlas_path(void);
int rogue_tile_atlas_columns(void);
int rogue_tile_atlas_source_width(void);
int rogue_tile_atlas_source_height(void);

#endif
""",
        encoding="utf-8",
    )

    source_path.write_text(
        f"""/*
 * Generated by tile_picker/generate_tile_mapping.py. Do not edit by hand.
 * Active source: {mode}
 */

#include <stdio.h>
#include <curses.h>
#include "../rogue.h"
#include "rogue_tile_mapping.h"

const ROGUE_GENERATED_TILE_MAPPING rogue_tile_glyph_mappings[] = {{
{glyph_table}
}};

const int rogue_tile_glyph_mapping_count =
    sizeof(rogue_tile_glyph_mappings) / sizeof(rogue_tile_glyph_mappings[0]);

const ROGUE_GENERATED_MONSTER_MAPPING rogue_tile_monster_mappings[] = {{
{monster_table}
}};

const int rogue_tile_monster_mapping_count =
    sizeof(rogue_tile_monster_mappings) / sizeof(rogue_tile_monster_mappings[0]);

const ROGUE_GENERATED_VARIANT_MONSTER_MAPPING rogue_tile_variant_monster_mappings[] = {{
{variant_monster_table}
}};

const int rogue_tile_variant_monster_mapping_count = {variant_monster_count};

const char *
rogue_tile_atlas_path(void)
{{
    return {c_string(source["path"])};
}}

int
rogue_tile_atlas_columns(void)
{{
    return {int(source["columns"])};
}}

int
rogue_tile_atlas_source_width(void)
{{
    return {int(source["tileWidth"])};
}}

int
rogue_tile_atlas_source_height(void)
{{
    return {int(source["tileHeight"])};
}}
""",
        encoding="utf-8",
    )

    return {
        "mode": mode,
        "header": str(header_path.relative_to(root)),
        "source": str(source_path.relative_to(root)),
        "atlas": source,
    }


def generate(root: Path, custom_profile: Path | None = None, force_rltiles: bool = False) -> dict[str, Any]:
    root = root.resolve()
    if custom_profile is None and not force_rltiles:
        custom_profile = active_custom_profile(root)

    if custom_profile is not None:
        profile = read_json(custom_profile if custom_profile.is_absolute() else root / custom_profile)
        source = custom_source_from_profile(root, profile)
        return write_generated(root, profile, source, "custom")

    profile, source = rltiles_profile(root)
    return write_generated(root, profile, source, "rltiles")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--custom-profile", type=Path)
    parser.add_argument("--rltiles", action="store_true", help="force bundled RLTiles mapping")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    result = generate(args.root, custom_profile=args.custom_profile, force_rltiles=args.rltiles)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
