#!/usr/bin/env python3
"""Serve the RogueTiles tile picker and apply/build/run selected tiles."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import threading
import webbrowser
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any
from urllib.parse import parse_qs, urlparse

from . import build_picker, generate_tile_mapping, tilepack_writer
from .role_catalog import role_by_id


ACTIVE_SOURCE = Path("tile_picker/data/active_tile_source.json")
APPLIED_CUSTOM_PROFILE = Path("tile_picker/data/applied_custom_tilemap_profile.json")
PICKER_PROFILE_DIR = Path("tile_picker/profiles")


class BadRequest(ValueError):
    pass


def repo_root() -> Path:
    if getattr(sys, "frozen", False):
        return Path(sys.executable).resolve().parent
    return Path(__file__).resolve().parents[1]


def read_json(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise BadRequest(f"{path} must contain a JSON object")
    return data


def write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def relative_path(root: Path, path: Path) -> str:
    return path.resolve().relative_to(root.resolve()).as_posix()


def safe_profile_name(name: Any) -> str:
    raw = str(name or "").strip()
    if not raw:
        raise BadRequest("Profile name is required")
    safe = "".join(ch if ch.isalnum() or ch in ("-", "_", " ") else "_" for ch in raw)
    safe = "_".join(safe.split())
    if not safe:
        raise BadRequest("Profile name must contain letters or numbers")
    return safe[:80]


def validate_tiles(payload: dict[str, Any]) -> dict[str, Any]:
    tiles = payload.get("tiles")
    if not isinstance(tiles, dict):
        raise BadRequest("Payload must include a top-level tiles object")
    return tiles


def apply_rltiles_mapping(root: Path, payload: dict[str, Any]) -> dict[str, Any]:
    tiles = validate_tiles(payload)
    mapping_path = root / "assets" / "rltiles" / "rogue-rltiles-map.json"
    mapping = read_json(mapping_path)
    roles = role_by_id()

    for role_id, atlas_name in tiles.items():
        role = roles.get(role_id)
        if role is None or role.role == "terrain.empty":
            continue
        if role.role.startswith("monster."):
            entry = mapping.setdefault("monsters", {}).setdefault(role.key, {})
        else:
            entry = mapping.setdefault(role.group, {}).setdefault(role.key, {})
        if isinstance(entry, dict):
            entry["atlas"] = str(atlas_name) if atlas_name else None

    write_json(mapping_path, mapping)
    write_json(root / ACTIVE_SOURCE, {"mode": "rltiles"})
    tilepack_writer.write_default_pack(root, "default")
    tilepack_writer.write_default_pack(root, "active")
    generated = generate_tile_mapping.generate(root, force_rltiles=True)
    build_picker.build(root)
    return {
        "ok": True,
        "mappingPath": relative_path(root, mapping_path),
        "tilepackPath": "tilepacks/active/tilepack.json",
        "generated": generated,
    }


def apply_custom_profile(root: Path, payload: dict[str, Any]) -> dict[str, Any]:
    validate_tiles(payload)
    if not isinstance(payload.get("source"), dict):
        raise BadRequest("Custom profile must include a source object")
    profile_path = root / APPLIED_CUSTOM_PROFILE
    write_json(profile_path, payload)
    write_json(root / ACTIVE_SOURCE, {
        "mode": "custom",
        "profile": APPLIED_CUSTOM_PROFILE.as_posix(),
    })
    tilepack_writer.write_active_custom_pack(root, payload, "active")
    generated = generate_tile_mapping.generate(root, custom_profile=profile_path)
    return {
        "ok": True,
        "profilePath": relative_path(root, profile_path),
        "tilepackPath": "tilepacks/active/tilepack.json",
        "generated": generated,
    }


def build_game(root: Path) -> dict[str, Any]:
    command = [
        "powershell",
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        str(root / "scripts" / "build-windows-native.ps1"),
        "-Tiles",
    ]
    completed = subprocess.run(
        command,
        cwd=root,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    return {
        "ok": completed.returncode == 0,
        "command": " ".join(command),
        "output": completed.stdout,
        "exePath": "native-build/rogue54.exe",
    }


def run_game(root: Path) -> dict[str, Any]:
    packaged = root / "RogueTiles.exe"
    dev = root / "native-build" / "rogue54.exe"
    exe = packaged if packaged.exists() else dev
    if not exe.exists():
        raise BadRequest("RogueTiles.exe or native-build/rogue54.exe was not found.")
    env = os.environ.copy()
    env["PATH"] = str(exe.parent) + ";" + env.get("PATH", "")
    process = subprocess.Popen(
        [str(exe), "--tiles"],
        cwd=exe.parent,
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    return {"ok": True, "pid": process.pid, "exePath": relative_path(root, exe)}


def picker_profile_path(root: Path, name: Any) -> Path:
    return root / PICKER_PROFILE_DIR / f"{safe_profile_name(name)}.json"


def list_profiles(root: Path) -> dict[str, Any]:
    profile_dir = root / PICKER_PROFILE_DIR
    profiles = []
    if profile_dir.exists():
        for path in sorted(profile_dir.glob("*.json")):
            profiles.append({"name": path.stem, "path": relative_path(root, path)})
    return {"ok": True, "profiles": profiles}


def save_profile(root: Path, payload: dict[str, Any]) -> dict[str, Any]:
    profile = payload.get("profile")
    if not isinstance(profile, dict):
        raise BadRequest("profile must be a JSON object")
    path = picker_profile_path(root, payload.get("name"))
    write_json(path, profile)
    return {"ok": True, "name": path.stem, "path": relative_path(root, path)}


def load_profile(root: Path, name: Any) -> dict[str, Any]:
    path = picker_profile_path(root, name)
    if not path.exists():
        raise BadRequest(f"Profile was not found: {safe_profile_name(name)}")
    return {"ok": True, "name": path.stem, "profile": read_json(path)}


def delete_profile(root: Path, payload: dict[str, Any]) -> dict[str, Any]:
    path = picker_profile_path(root, payload.get("name"))
    if path.exists():
        path.unlink()
    return {"ok": True, "name": path.stem}


def status(root: Path) -> dict[str, Any]:
    packaged = root / "RogueTiles.exe"
    dev = root / "native-build" / "rogue54.exe"
    return {
        "ok": True,
        "repoRoot": root.as_posix(),
        "buildScript": (root / "scripts" / "build-windows-native.ps1").exists(),
        "exe": packaged.exists() or dev.exists(),
        "generated": (root / "generated" / "rogue_tile_mapping.c").exists(),
        "activeSource": read_json(root / ACTIVE_SOURCE) if (root / ACTIVE_SOURCE).exists() else {"mode": "rltiles"},
    }


class PickerHandler(SimpleHTTPRequestHandler):
    server_version = "RogueTilePicker/1.0"

    @property
    def root(self) -> Path:
        return self.server.repo_root  # type: ignore[attr-defined]

    def end_headers(self) -> None:
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def translate_path(self, path: str) -> str:
        parsed = urlparse(path)
        clean = parsed.path
        if clean == "/":
            clean = "/tile_picker/index.html"
        return str((self.root / clean.lstrip("/")).resolve())

    def read_json_body(self) -> dict[str, Any]:
        length = int(self.headers.get("Content-Length", "0"))
        if length <= 0:
            raise BadRequest("Request body is required")
        data = json.loads(self.rfile.read(length).decode("utf-8"))
        if not isinstance(data, dict):
            raise BadRequest("Request body must be a JSON object")
        return data

    def write_json(self, code: int, payload: dict[str, Any]) -> None:
        raw = json.dumps(payload, indent=2).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        try:
            if parsed.path == "/api/status":
                self.write_json(200, status(self.root))
                return
            if parsed.path == "/api/list-picker-configs":
                self.write_json(200, list_profiles(self.root))
                return
            if parsed.path == "/api/load-picker-config":
                query = parse_qs(parsed.query)
                self.write_json(200, load_profile(self.root, query.get("name", [""])[0]))
                return
        except BadRequest as exc:
            self.write_json(400, {"ok": False, "error": str(exc)})
            return
        super().do_GET()

    def do_POST(self) -> None:
        parsed = urlparse(self.path)
        try:
            payload = self.read_json_body()
            if parsed.path == "/api/apply-rltiles-mapping":
                result = apply_rltiles_mapping(self.root, payload)
            elif parsed.path == "/api/apply-custom-profile":
                result = apply_custom_profile(self.root, payload)
            elif parsed.path == "/api/build-game":
                result = build_game(self.root)
            elif parsed.path == "/api/run-game":
                result = run_game(self.root)
            elif parsed.path == "/api/save-picker-config":
                result = save_profile(self.root, payload)
            elif parsed.path == "/api/delete-picker-config":
                result = delete_profile(self.root, payload)
            else:
                self.write_json(404, {"ok": False, "error": "Unknown endpoint"})
                return
        except BadRequest as exc:
            self.write_json(400, {"ok": False, "error": str(exc)})
            return
        except Exception as exc:
            self.write_json(500, {"ok": False, "error": str(exc)})
            return
        self.write_json(200 if result.get("ok") else 500, result)


def run_server(root: Path, host: str, port: int, open_browser: bool = False) -> None:
    root = root.resolve()
    build_picker.build(root)
    generate_tile_mapping.generate(root)
    server = ThreadingHTTPServer((host, port), PickerHandler)
    server.repo_root = root  # type: ignore[attr-defined]
    url = f"http://{host}:{port}/tile_picker/index.html"
    print(f"Rogue tile picker running at {url}")
    if open_browser:
        threading.Timer(0.5, lambda: webbrowser.open(url)).start()
    server.serve_forever()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    default_open = bool(getattr(sys, "frozen", False))
    parser.add_argument("--root", type=Path, default=repo_root())
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8788)
    parser.add_argument("--open", action="store_true", default=default_open)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    run_server(args.root, args.host, args.port, open_browser=args.open)


if __name__ == "__main__":
    main()
