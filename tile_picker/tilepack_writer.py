#!/usr/bin/env python3
"""Write RogueTiles runtime tile packs."""

from __future__ import annotations

import base64
import json
import re
import shutil
from pathlib import Path
from typing import Any

from .role_catalog import Role, all_roles


class TilePackError(ValueError):
    pass


DEFAULT_TILEPACKS = Path("tilepacks")


def read_json(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise TilePackError(f"{path} must contain a JSON object")
    return data


def write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def role_atlas_from_mapping(mapping: dict[str, Any], role: Role) -> str | None:
    if role.role.startswith("monster."):
        entry = mapping.get("monsters", {}).get(role.key, {})
    else:
        entry = mapping.get(role.group, {}).get(role.key, {})
    if isinstance(entry, dict):
        atlas = entry.get("atlas")
        return str(atlas) if atlas else None
    return None


def atlas_lookup(root: Path) -> dict[str, int]:
    atlas = read_json(root / "assets" / "rltiles" / "rltiles-2d.json")
    tiles = atlas.get("tiles", [])
    if not isinstance(tiles, list):
        raise TilePackError("rltiles-2d.json must contain a tiles list")
    return {str(name): index for index, name in enumerate(tiles)}


def write_pack(
    pack_dir: Path,
    name: str,
    image_source: Path,
    tile_width: int,
    tile_height: int,
    columns: int,
    roles: dict[str, dict[str, Any]],
) -> dict[str, Any]:
    if tile_width <= 0 or tile_height <= 0 or columns <= 0:
        raise TilePackError("Tile width, tile height, and columns must be positive")
    if not image_source.exists():
        raise TilePackError(f"Tile image does not exist: {image_source}")

    pack_dir.mkdir(parents=True, exist_ok=True)
    if image_source.resolve() != (pack_dir / "tiles.png").resolve():
        shutil.copy2(image_source, pack_dir / "tiles.png")
    write_json(pack_dir / "tilepack.json", {
        "schemaVersion": 1,
        "name": name,
        "image": "tiles.png",
        "mapping": "mapping.json",
        "tileWidth": tile_width,
        "tileHeight": tile_height,
        "columns": columns,
    })
    write_json(pack_dir / "mapping.json", {
        "schemaVersion": 1,
        "roles": roles,
    })
    return {"ok": True, "packDir": pack_dir.as_posix()}


def write_default_pack(root: Path, pack_name: str = "default") -> dict[str, Any]:
    root = root.resolve()
    atlas = read_json(root / "assets" / "rltiles" / "rltiles-2d.json")
    mapping = read_json(root / "assets" / "rltiles" / "rogue-rltiles-map.json")
    lookup = atlas_lookup(root)
    roles: dict[str, dict[str, Any]] = {}

    for role in all_roles():
        atlas_name = role_atlas_from_mapping(mapping, role)
        if atlas_name is None:
            continue
        roles[role.role] = {
            "index": int(lookup.get(atlas_name, -1)),
            "name": atlas_name,
        }

    return write_pack(
        root / DEFAULT_TILEPACKS / pack_name,
        "Default RL Tiles",
        root / "assets" / "rltiles" / "rltiles-2d.png",
        32,
        32,
        int(atlas.get("width", 30)),
        roles,
    )


def image_from_data_url(root: Path, data_url: str, output: Path) -> Path:
    match = re.match(r"data:image/[^;]+;base64,(?P<data>.+)$", data_url)
    if not match:
        raise TilePackError("Custom tilemap dataUrl is not a base64 image")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(base64.b64decode(match.group("data")))
    return output


def write_active_custom_pack(root: Path, profile: dict[str, Any], pack_name: str = "active") -> dict[str, Any]:
    root = root.resolve()
    source = profile.get("source")
    tiles = profile.get("tiles")
    if not isinstance(source, dict):
        raise TilePackError("Custom profile must include a source object")
    if not isinstance(tiles, dict):
        raise TilePackError("Custom profile must include a tiles object")

    image_path = root / "tilepacks" / pack_name / "tiles.png"
    data_url = source.get("dataUrl")
    if isinstance(data_url, str):
        image_source = image_from_data_url(root, data_url, image_path)
    else:
        source_path = source.get("path")
        if not source_path:
            raise TilePackError("Custom profile source must include dataUrl or path")
        image_source = root / str(source_path)

    roles: dict[str, dict[str, Any]] = {}
    for role_id, selected in tiles.items():
        if isinstance(selected, dict):
            index = selected.get("index")
            if isinstance(index, int) and index >= 0:
                roles[str(role_id)] = {
                    "index": index,
                    "name": str(selected.get("name") or role_id),
                }
        elif isinstance(selected, int) and selected >= 0:
            roles[str(role_id)] = {"index": selected, "name": str(role_id)}

    return write_pack(
        root / DEFAULT_TILEPACKS / pack_name,
        str(profile.get("name") or "Custom Tiles"),
        image_source,
        int(source.get("tileWidth") or 32),
        int(source.get("tileHeight") or source.get("tileWidth") or 32),
        int(source.get("columns") or 1),
        roles,
    )
