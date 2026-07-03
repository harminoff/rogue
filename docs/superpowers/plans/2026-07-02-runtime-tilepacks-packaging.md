# Runtime Tilepacks Packaging Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make RogueTiles and its tile picker download-and-go by moving player tile changes into runtime `tilepacks/` data and packaging the game plus picker without requiring Python, MSYS2, Allegro, or compilers on the player's machine.

**Architecture:** Keep `tiles.c` as the semantic map producer. Add a small runtime tile-pack loader used only by the Allegro frontend; it resolves role IDs to atlas indices and image metadata, falling back to generated defaults when runtime data is absent or invalid. Update the picker server to write `tilepacks/active/`, then add a package script that emits `dist/RogueTiles/` with the game, DLLs, assets, tile packs, and a bundled picker launcher.

**Tech Stack:** C89/MinGW, Allegro 5, PowerShell packaging scripts, Python standard library picker tooling, PyInstaller for the first no-install `TilePicker.exe`.

---

## File Structure

- Create `tilepack.h`: public C API for loading runtime tile packs and resolving role IDs.
- Create `tilepack.c`: C runtime loader, conservative JSON field extraction for controlled `tilepack.json` and `mapping.json`, fallback logic, and path handling.
- Modify `tiles.h`: add `under_role` to `ROGUE_TILE_CELL`.
- Modify `tiles.c`: populate `under_role` while preserving existing generated fallback atlas fields.
- Modify `allegro_frontend.c`: load the runtime tile pack before loading the atlas bitmap, use tile-pack metadata for source rectangles, and resolve each cell by role at draw time.
- Modify `Makefile.std`: compile/link `tilepack.o`.
- Modify `scripts/build-windows-native.ps1`: overlay `tilepack.c` and `tilepack.h`, copy `tilepacks/` when present.
- Create `tile_picker/tilepack_writer.py`: shared Python writer/validator for `tilepacks/default/` and `tilepacks/active/`.
- Modify `tile_picker/serve_picker.py`: apply actions write runtime tile packs and launch packaged or dev game without rebuilding.
- Modify `tile_picker/index.html`: replace user-facing Build/Apply+Run flow with Save/Run flow while keeping developer profile save/load.
- Create `tests/test_tilepack_writer.py`: tests for default RL Tiles pack generation, custom upload pack generation, and validation failures.
- Create `scripts/build-runtime-tilepacks.ps1`: generates default and active tile packs from current RL Tiles data.
- Create `scripts/package-windows.ps1`: builds tile mode and creates `dist/RogueTiles/`.
- Create `scripts/build-tile-picker-exe.ps1`: wraps PyInstaller invocation for `TilePicker.exe`.

---

### Task 1: Add Python Tile Pack Writer

**Files:**
- Create: `tile_picker/tilepack_writer.py`
- Create: `tests/test_tilepack_writer.py`
- Modify: none

- [ ] **Step 1: Write failing tests for runtime pack generation**

Create `tests/test_tilepack_writer.py` with:

```python
import base64
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tile_picker import tilepack_writer


PNG_1X1 = base64.b64decode(
    "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+/p9sAAAAASUVORK5CYII="
)


class TilePackWriterTests(unittest.TestCase):
    def test_write_default_pack_from_rltiles_mapping(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            assets = root / "assets" / "rltiles"
            assets.mkdir(parents=True)
            (assets / "rltiles-2d.png").write_bytes(PNG_1X1)
            (assets / "rltiles-2d.json").write_text(
                json.dumps({"width": 2, "tiles": ["floor", "human", "hobgoblin"]}),
                encoding="utf-8",
            )
            (assets / "rogue-rltiles-map.json").write_text(
                json.dumps({
                    "terrain": {"floor": {"atlas": "floor"}},
                    "actors": {"player": {"atlas": "human"}},
                    "objects": {},
                    "monsters": {"H": {"atlas": "hobgoblin"}},
                }),
                encoding="utf-8",
            )

            result = tilepack_writer.write_default_pack(root)

            self.assertEqual(result["ok"], True)
            tilepack = json.loads((root / "tilepacks" / "default" / "tilepack.json").read_text(encoding="utf-8"))
            mapping = json.loads((root / "tilepacks" / "default" / "mapping.json").read_text(encoding="utf-8"))
            self.assertEqual(tilepack["tileWidth"], 32)
            self.assertEqual(tilepack["tileHeight"], 32)
            self.assertEqual(tilepack["columns"], 2)
            self.assertEqual(tilepack["image"], "tiles.png")
            self.assertEqual(mapping["roles"]["terrain.floor"]["index"], 0)
            self.assertEqual(mapping["roles"]["actor.player"]["index"], 1)
            self.assertEqual(mapping["roles"]["monster.H"]["index"], 2)
            self.assertTrue((root / "tilepacks" / "default" / "tiles.png").exists())

    def test_write_active_custom_pack_decodes_uploaded_image(self):
        data_url = "data:image/png;base64," + base64.b64encode(PNG_1X1).decode("ascii")
        profile = {
            "source": {
                "dataUrl": data_url,
                "tileWidth": 16,
                "tileHeight": 24,
                "columns": 4,
            },
            "tiles": {
                "terrain.floor": {"index": 3, "name": "custom floor"},
                "actor.player": {"index": 4, "name": "custom player"},
            },
        }

        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)

            result = tilepack_writer.write_active_custom_pack(root, profile)

            self.assertEqual(result["ok"], True)
            tilepack = json.loads((root / "tilepacks" / "active" / "tilepack.json").read_text(encoding="utf-8"))
            mapping = json.loads((root / "tilepacks" / "active" / "mapping.json").read_text(encoding="utf-8"))
            self.assertEqual(tilepack["tileWidth"], 16)
            self.assertEqual(tilepack["tileHeight"], 24)
            self.assertEqual(tilepack["columns"], 4)
            self.assertEqual(mapping["roles"]["actor.player"]["index"], 4)
            self.assertTrue((root / "tilepacks" / "active" / "tiles.png").exists())

    def test_custom_pack_rejects_missing_source(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            with self.assertRaises(tilepack_writer.TilePackError):
                tilepack_writer.write_active_custom_pack(root, {"tiles": {}})


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run tests to verify failure**

Run:

```powershell
python tests\test_tilepack_writer.py
```

Expected: fail with `ImportError` or `ModuleNotFoundError` because `tile_picker.tilepack_writer` does not exist.

- [ ] **Step 3: Implement `tile_picker/tilepack_writer.py`**

Create `tile_picker/tilepack_writer.py`:

```python
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


def write_pack(pack_dir: Path, name: str, image_source: Path, tile_width: int, tile_height: int, columns: int, roles: dict[str, dict[str, Any]]) -> dict[str, Any]:
    if tile_width <= 0 or tile_height <= 0 or columns <= 0:
        raise TilePackError("Tile width, tile height, and columns must be positive")
    if not image_source.exists():
        raise TilePackError(f"Tile image does not exist: {image_source}")

    pack_dir.mkdir(parents=True, exist_ok=True)
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
```

- [ ] **Step 4: Run tests to verify pass**

Run:

```powershell
python tests\test_tilepack_writer.py
```

Expected: `Ran 3 tests` and `OK`.

- [ ] **Step 5: Commit**

```powershell
git add tile_picker\tilepack_writer.py tests\test_tilepack_writer.py
git commit -m "Add runtime tilepack writer"
```

---

### Task 2: Generate Default And Active Tile Packs For Developers

**Files:**
- Create: `scripts/build-runtime-tilepacks.ps1`
- Modify: none
- Test: `tests/test_tilepack_writer.py`

- [ ] **Step 1: Write the script**

Create `scripts/build-runtime-tilepacks.ps1`:

```powershell
param(
    [string]$Python = "python"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

Push-Location $repoRoot
try {
    & $Python -c "from pathlib import Path; from tile_picker import tilepack_writer; root=Path('.').resolve(); tilepack_writer.write_default_pack(root, 'default'); tilepack_writer.write_default_pack(root, 'active'); print('Wrote tilepacks/default and tilepacks/active')"
    if ($LASTEXITCODE -ne 0) {
        throw "runtime tilepack generation failed"
    }
}
finally {
    Pop-Location
}
```

- [ ] **Step 2: Run script**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-runtime-tilepacks.ps1
```

Expected: prints `Wrote tilepacks/default and tilepacks/active`.

- [ ] **Step 3: Inspect generated files**

Run:

```powershell
Get-ChildItem tilepacks -Recurse | Select-Object FullName,Length
```

Expected: `tilepacks\default\tilepack.json`, `tilepacks\default\mapping.json`, `tilepacks\default\tiles.png`, `tilepacks\active\tilepack.json`, `tilepacks\active\mapping.json`, and `tilepacks\active\tiles.png`.

- [ ] **Step 4: Run writer tests again**

Run:

```powershell
python tests\test_tilepack_writer.py
```

Expected: `OK`.

- [ ] **Step 5: Commit**

```powershell
git add scripts\build-runtime-tilepacks.ps1 tilepacks
git commit -m "Generate runtime tilepacks"
```

---

### Task 3: Add C Runtime Tile Pack Loader

**Files:**
- Create: `tilepack.h`
- Create: `tilepack.c`
- Modify: `Makefile.std`
- Modify: `scripts/build-windows-native.ps1`
- Test: native build and tile smoke

- [ ] **Step 1: Add public C API**

Create `tilepack.h`:

```c
#ifndef ROGUE_TILEPACK_H
#define ROGUE_TILEPACK_H

#include <stdbool.h>

typedef struct rogue_tilepack_entry {
    char role[64];
    char name[128];
    int index;
} ROGUE_TILEPACK_ENTRY;

bool rogue_tilepack_load(void);
const char *rogue_tilepack_atlas_path(void);
int rogue_tilepack_columns(void);
int rogue_tilepack_source_width(void);
int rogue_tilepack_source_height(void);
int rogue_tilepack_lookup_index(const char *role, int fallback_index);
const char *rogue_tilepack_lookup_name(const char *role, const char *fallback_name);
const char *rogue_tilepack_status(void);

#endif
```

- [ ] **Step 2: Add C loader implementation**

Create `tilepack.c` with these functions:

```c
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tilepack.h"
#include "generated/rogue_tile_mapping.h"

#define ROGUE_TILEPACK_MAX_TEXT 262144
#define ROGUE_TILEPACK_MAX_ENTRIES 128

static ROGUE_TILEPACK_ENTRY entries[ROGUE_TILEPACK_MAX_ENTRIES];
static int entry_count = 0;
static char atlas_path[512] = "assets/rltiles/rltiles-2d.png";
static int atlas_columns = 30;
static int source_width = 32;
static int source_height = 32;
static char status_text[256] = "built-in generated tile mapping";
static bool loaded = FALSE;

static char *read_text_file(const char *path)
{
    FILE *file;
    long size;
    char *text;

    file = fopen(path, "rb");
    if (file == NULL)
        return NULL;
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    if (size < 0 || size > ROGUE_TILEPACK_MAX_TEXT)
    {
        fclose(file);
        return NULL;
    }
    fseek(file, 0, SEEK_SET);
    text = (char *)calloc((size_t)size + 1, 1);
    if (text == NULL)
    {
        fclose(file);
        return NULL;
    }
    fread(text, 1, (size_t)size, file);
    fclose(file);
    return text;
}

static const char *skip_ws(const char *p)
{
    while (*p != '\0' && isspace((unsigned char)*p))
        p++;
    return p;
}

static bool json_string_field(const char *json, const char *key, char *out, size_t out_size)
{
    char pattern[96];
    const char *p;
    char *w;
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(json, pattern);
    if (p == NULL)
        return FALSE;
    p = strchr(p + strlen(pattern), ':');
    if (p == NULL)
        return FALSE;
    p = skip_ws(p + 1);
    if (*p != '"')
        return FALSE;
    p++;
    w = out;
    while (*p != '\0' && *p != '"' && (size_t)(w - out) + 1 < out_size)
    {
        if (*p == '\\' && p[1] != '\0')
            p++;
        *w++ = *p++;
    }
    *w = '\0';
    return TRUE;
}

static bool json_int_field(const char *json, const char *key, int *out)
{
    char pattern[96];
    const char *p;
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(json, pattern);
    if (p == NULL)
        return FALSE;
    p = strchr(p + strlen(pattern), ':');
    if (p == NULL)
        return FALSE;
    p = skip_ws(p + 1);
    *out = atoi(p);
    return TRUE;
}

static void dirname_of(const char *path, char *out, size_t out_size)
{
    const char *slash;
    strncpy(out, path, out_size - 1);
    out[out_size - 1] = '\0';
    slash = strrchr(out, '/');
    if (slash == NULL)
        slash = strrchr(out, '\\');
    if (slash == NULL)
    {
        strncpy(out, ".", out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }
    *slash = '\0';
}

static void join_path(const char *dir, const char *name, char *out, size_t out_size)
{
    snprintf(out, out_size, "%s/%s", dir, name);
}

static void reset_to_generated(void)
{
    entry_count = 0;
    strncpy(atlas_path, rogue_tile_atlas_path(), sizeof(atlas_path) - 1);
    atlas_path[sizeof(atlas_path) - 1] = '\0';
    atlas_columns = rogue_tile_atlas_columns();
    source_width = rogue_tile_atlas_source_width();
    source_height = rogue_tile_atlas_source_height();
}

static bool add_entry(const char *role, int index, const char *name)
{
    ROGUE_TILEPACK_ENTRY *entry;
    if (entry_count >= ROGUE_TILEPACK_MAX_ENTRIES)
        return FALSE;
    entry = &entries[entry_count++];
    strncpy(entry->role, role, sizeof(entry->role) - 1);
    entry->role[sizeof(entry->role) - 1] = '\0';
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';
    entry->index = index;
    return TRUE;
}
```

Then add parser helpers that scan the controlled `mapping.json` role object:

```c
static void parse_mapping_roles(const char *json)
{
    const char *p;
    char role[64];
    char name[128];
    int index;

    p = json;
    while ((p = strstr(p, "\"index\"")) != NULL)
    {
        const char *role_start;
        const char *quote;
        role_start = p;
        while (role_start > json && *role_start != '}')
        {
            if (*role_start == '"' && role_start > json && role_start[-1] != '\\')
                break;
            role_start--;
        }
        while (role_start > json && *role_start != '"')
            role_start--;
        if (*role_start != '"')
        {
            p += 7;
            continue;
        }
        quote = strchr(role_start + 1, '"');
        if (quote == NULL)
            break;
        if ((size_t)(quote - role_start - 1) >= sizeof(role))
        {
            p += 7;
            continue;
        }
        memcpy(role, role_start + 1, (size_t)(quote - role_start - 1));
        role[quote - role_start - 1] = '\0';

        if (!json_int_field(p, "index", &index))
        {
            p += 7;
            continue;
        }
        if (!json_string_field(p, "name", name, sizeof(name)))
            strncpy(name, role, sizeof(name) - 1);
        add_entry(role, index, name);
        p += 7;
    }
}
```

Finish with the public API:

```c
static bool load_tilepack_file(const char *tilepack_path)
{
    char *tilepack_json;
    char *mapping_json;
    char dir[512];
    char image_name[256];
    char mapping_name[256];
    char mapping_path[512];

    tilepack_json = read_text_file(tilepack_path);
    if (tilepack_json == NULL)
        return FALSE;

    if (!json_string_field(tilepack_json, "image", image_name, sizeof(image_name)) ||
        !json_string_field(tilepack_json, "mapping", mapping_name, sizeof(mapping_name)) ||
        !json_int_field(tilepack_json, "tileWidth", &source_width) ||
        !json_int_field(tilepack_json, "tileHeight", &source_height) ||
        !json_int_field(tilepack_json, "columns", &atlas_columns) ||
        source_width <= 0 || source_height <= 0 || atlas_columns <= 0)
    {
        free(tilepack_json);
        return FALSE;
    }

    dirname_of(tilepack_path, dir, sizeof(dir));
    join_path(dir, image_name, atlas_path, sizeof(atlas_path));
    join_path(dir, mapping_name, mapping_path, sizeof(mapping_path));

    mapping_json = read_text_file(mapping_path);
    if (mapping_json == NULL)
    {
        free(tilepack_json);
        return FALSE;
    }

    entry_count = 0;
    parse_mapping_roles(mapping_json);
    snprintf(status_text, sizeof(status_text), "loaded %s with %d role mappings", tilepack_path, entry_count);
    free(mapping_json);
    free(tilepack_json);
    return entry_count > 0;
}

bool rogue_tilepack_load(void)
{
    if (loaded)
        return TRUE;
    loaded = TRUE;
    reset_to_generated();
    if (load_tilepack_file("tilepacks/active/tilepack.json"))
        return TRUE;
    if (load_tilepack_file("tilepacks/default/tilepack.json"))
        return TRUE;
    snprintf(status_text, sizeof(status_text), "using generated fallback tile mapping");
    return FALSE;
}

const char *rogue_tilepack_atlas_path(void)
{
    rogue_tilepack_load();
    return atlas_path;
}

int rogue_tilepack_columns(void)
{
    rogue_tilepack_load();
    return atlas_columns;
}

int rogue_tilepack_source_width(void)
{
    rogue_tilepack_load();
    return source_width;
}

int rogue_tilepack_source_height(void)
{
    rogue_tilepack_load();
    return source_height;
}

int rogue_tilepack_lookup_index(const char *role, int fallback_index)
{
    int i;
    rogue_tilepack_load();
    if (role == NULL)
        return fallback_index;
    for (i = 0; i < entry_count; i++)
        if (strcmp(entries[i].role, role) == 0 && entries[i].index >= 0)
            return entries[i].index;
    return fallback_index;
}

const char *rogue_tilepack_lookup_name(const char *role, const char *fallback_name)
{
    int i;
    rogue_tilepack_load();
    if (role == NULL)
        return fallback_name;
    for (i = 0; i < entry_count; i++)
        if (strcmp(entries[i].role, role) == 0)
            return entries[i].name;
    return fallback_name;
}

const char *rogue_tilepack_status(void)
{
    rogue_tilepack_load();
    return status_text;
}
```

- [ ] **Step 3: Update build files**

In `Makefile.std`:

- Add `tilepack.h` to `HDRS`.
- Add `tilepack.$(O)` to `OBJS2` after `tiles.$(O)`.
- Add `tilepack.c` to `CFILES` after `tiles.c`.

In `scripts/build-windows-native.ps1`, add these paths to the working-tree overlay list:

```powershell
"tilepack.c",
"tilepack.h",
```

Add this copy block after the existing `generated` copy block:

```powershell
$tilepackSource = Join-Path $repoRoot "tilepacks"
if (Test-Path $tilepackSource) {
    Copy-Item -LiteralPath $tilepackSource -Destination (Join-Path $buildDir "tilepacks") -Recurse -Force
}
```

- [ ] **Step 4: Build**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-windows-native.ps1 -Tiles
```

Expected: `native-build\rogue54.exe` exists and no link error for `rogue_tilepack_*`.

- [ ] **Step 5: Smoke test**

Run:

```powershell
.\native-build\rogue54.exe --tiles-smoke
```

Expected: exits `0`.

- [ ] **Step 6: Commit**

```powershell
git add tilepack.c tilepack.h Makefile.std scripts\build-windows-native.ps1
git commit -m "Load runtime tilepack metadata"
```

---

### Task 4: Resolve Cell Roles Through Runtime Tile Packs

**Files:**
- Modify: `tiles.h`
- Modify: `tiles.c`
- Modify: `allegro_frontend.c`
- Test: native tile smoke

- [ ] **Step 1: Add underlay role to cells**

In `tiles.h`, add this field after `under_glyph`:

```c
const char *under_role;
```

In `tiles.c`, set `cell->under_role = NULL;` in every initialization path that currently sets `under_atlas_key`.

- [ ] **Step 2: Populate underlay role**

In `tiles.c::apply_underlay`, after assigning `cell->under_glyph`, add:

```c
cell->under_role = mapping->role;
```

- [ ] **Step 3: Include tilepack API in Allegro renderer**

In `allegro_frontend.c`, replace:

```c
#include "generated/rogue_tile_mapping.h"
```

with:

```c
#include "tilepack.h"
```

- [ ] **Step 4: Use runtime atlas metadata**

In `allegro_frontend.c`, replace calls:

```c
rogue_tile_atlas_columns()
rogue_tile_atlas_source_width()
rogue_tile_atlas_source_height()
rogue_tile_atlas_path()
```

with:

```c
rogue_tilepack_columns()
rogue_tilepack_source_width()
rogue_tilepack_source_height()
rogue_tilepack_atlas_path()
```

- [ ] **Step 5: Resolve role indices at draw time**

In `allegro_frontend.c`, add:

```c
static int
resolved_cell_index(const ROGUE_TILE_CELL *cell)
{
    if (cell == NULL)
        return -1;
    return rogue_tilepack_lookup_index(cell->role, cell->atlas_index);
}

static int
resolved_underlay_index(const ROGUE_TILE_CELL *cell)
{
    if (cell == NULL)
        return -1;
    return rogue_tilepack_lookup_index(cell->under_role, cell->under_atlas_index);
}
```

In `draw_cell`, replace direct checks/draws for `cell->atlas_index` and `cell->under_atlas_index` with local resolved values:

```c
int atlas_index;
int underlay_index;

atlas_index = resolved_cell_index(cell);
underlay_index = resolved_underlay_index(cell);

if (cell->has_underlay && underlay_index >= 0 && atlas != NULL)
    draw_atlas_tile(underlay_index, dx, dy);

if (atlas_index < 0 || atlas == NULL)
{
    fallback = al_map_rgb(100, 20, 60);
    al_draw_filled_rectangle(dx, dy, dx + ROGUE_TILE_DRAW_SIZE,
                             dy + ROGUE_TILE_DRAW_SIZE, fallback);
    draw_glyph_cell(screen_x, screen_y, cell);
    return;
}

draw_atlas_tile(atlas_index, dx, dy);
```

In `draw_wall_cell`, resolve the wall index before checking/drawing:

```c
int wall_index;
wall_index = resolved_cell_index(cell);
if (wall_index < 0 || atlas == NULL)
{
    draw_glyph_cell(screen_x, screen_y, cell);
    return;
}
```

Then pass `wall_index` to each `draw_atlas_tile_region` call.

- [ ] **Step 6: Build and smoke**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-runtime-tilepacks.ps1
powershell -ExecutionPolicy Bypass -File scripts\build-windows-native.ps1 -Tiles
.\native-build\rogue54.exe --tiles-smoke
```

Expected: all commands pass.

- [ ] **Step 7: Commit**

```powershell
git add tiles.h tiles.c allegro_frontend.c
git commit -m "Resolve tiles from runtime tilepacks"
```

---

### Task 5: Update Picker Apply/Run Flow For Runtime Packs

**Files:**
- Modify: `tile_picker/serve_picker.py`
- Modify: `tile_picker/index.html`
- Test: `tests/test_tilepack_writer.py`, picker API smoke

- [ ] **Step 1: Import writer and replace apply behavior**

In `tile_picker/serve_picker.py`, change imports:

```python
from . import build_picker, generate_tile_mapping
```

to:

```python
from . import build_picker, generate_tile_mapping, tilepack_writer
```

In `apply_rltiles_mapping`, after writing `rogue-rltiles-map.json`, replace generated-only behavior with:

```python
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
```

In `apply_custom_profile`, after writing `APPLIED_CUSTOM_PROFILE`, add:

```python
tilepack_writer.write_active_custom_pack(root, payload, "active")
```

and include:

```python
"tilepackPath": "tilepacks/active/tilepack.json",
```

in the response.

- [ ] **Step 2: Make run_game prefer packaged executable**

Replace `run_game` executable selection with:

```python
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
```

- [ ] **Step 3: Keep build endpoint developer-only**

Leave `/api/build-game` in place for development, but make the UI stop presenting it as a normal player action in the next step.

- [ ] **Step 4: Update picker buttons**

In `tile_picker/index.html`, replace toolbar buttons:

```html
<button id="buildBtn" type="button">Build</button>
<button id="runBtn" type="button">Run</button>
<button id="applyRunBtn" type="button">Apply + Run</button>
```

with:

```html
<button id="runBtn" type="button">Run Game</button>
<button id="applyRunBtn" type="button">Save + Run</button>
```

Replace `applyBuildRun()` with:

```javascript
async function applyAndRun() {
  const applied = await applyActive();
  if (!applied) return;
  await runGame();
}
```

Replace this listener:

```javascript
document.getElementById("buildBtn").addEventListener("click", buildGame);
document.getElementById("applyRunBtn").addEventListener("click", applyBuildRun);
```

with:

```javascript
document.getElementById("applyRunBtn").addEventListener("click", applyAndRun);
```

Keep the `buildGame()` function only if a hidden developer control still calls it. If no UI calls it, remove the function and remove the `/api/build-game` user path later.

- [ ] **Step 5: Run tests and API smoke**

Run:

```powershell
python tests\test_tilepack_writer.py
python tests\test_tile_picker_generation.py
powershell -ExecutionPolicy Bypass -File scripts\build-tile-picker.ps1
```

Expected: tests pass and picker data builds.

- [ ] **Step 6: Commit**

```powershell
git add tile_picker\serve_picker.py tile_picker\index.html
git commit -m "Save picker selections as runtime tilepacks"
```

---

### Task 6: Package A Clean Game Folder

**Files:**
- Create: `scripts/package-windows.ps1`
- Modify: none
- Test: packaged tile smoke

- [ ] **Step 1: Create package script**

Create `scripts/package-windows.ps1`:

```powershell
param(
    [string]$MsysRoot = "C:\msys64",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$distRoot = Join-Path $repoRoot "dist"
$packageDir = Join-Path $distRoot "RogueTiles"
$nativeDir = Join-Path $repoRoot "native-build"

if (-not $SkipBuild) {
    powershell -ExecutionPolicy Bypass -File (Join-Path $repoRoot "scripts\build-runtime-tilepacks.ps1")
    powershell -ExecutionPolicy Bypass -File (Join-Path $repoRoot "scripts\build-windows-native.ps1") -MsysRoot $MsysRoot -Tiles
}

if (Test-Path $packageDir) {
    Remove-Item -LiteralPath $packageDir -Recurse -Force
}
New-Item -ItemType Directory -Force $packageDir | Out-Null

Copy-Item -LiteralPath (Join-Path $nativeDir "rogue54.exe") -Destination (Join-Path $packageDir "RogueTiles.exe") -Force
Copy-Item -LiteralPath (Join-Path $repoRoot "LICENSE.TXT") -Destination $packageDir -Force
Copy-Item -LiteralPath (Join-Path $repoRoot "README.md") -Destination $packageDir -Force

foreach ($dll in Get-ChildItem -LiteralPath $nativeDir -Filter "*.dll") {
    Copy-Item -LiteralPath $dll.FullName -Destination (Join-Path $packageDir $dll.Name) -Force
}

Copy-Item -LiteralPath (Join-Path $repoRoot "assets") -Destination (Join-Path $packageDir "assets") -Recurse -Force
Copy-Item -LiteralPath (Join-Path $repoRoot "tilepacks") -Destination (Join-Path $packageDir "tilepacks") -Recurse -Force
Copy-Item -LiteralPath (Join-Path $repoRoot "tile_picker") -Destination (Join-Path $packageDir "tile_picker") -Recurse -Force

Write-Host "Packaged: $packageDir"
```

- [ ] **Step 2: Run package script**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\package-windows.ps1
```

Expected: `dist\RogueTiles\RogueTiles.exe` exists with DLLs, `assets`, `tilepacks`, and `tile_picker`.

- [ ] **Step 3: Smoke test packaged game**

Run:

```powershell
.\dist\RogueTiles\RogueTiles.exe --tiles-smoke
```

Expected: exits `0`.

- [ ] **Step 4: Commit**

```powershell
git add scripts\package-windows.ps1
git commit -m "Package RogueTiles game folder"
```

---

### Task 7: Bundle TilePicker.exe With PyInstaller

**Files:**
- Create: `scripts/build-tile-picker-exe.ps1`
- Modify: `scripts/package-windows.ps1`
- Test: launch `TilePicker.exe --help` or server smoke

- [ ] **Step 1: Add PyInstaller build script**

Create `scripts/build-tile-picker-exe.ps1`:

```powershell
param(
    [string]$Python = "python"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$distDir = Join-Path $repoRoot "dist\picker-build"

Push-Location $repoRoot
try {
    & $Python -m PyInstaller --version | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "PyInstaller is not available. Install for developer packaging with: python -m pip install pyinstaller"
    }

    & $Python -m PyInstaller `
        --noconfirm `
        --onefile `
        --name TilePicker `
        --distpath $distDir `
        --workpath (Join-Path $repoRoot "build\pyinstaller") `
        --specpath (Join-Path $repoRoot "build\pyinstaller") `
        -m tile_picker.serve_picker
    if ($LASTEXITCODE -ne 0) {
        throw "PyInstaller failed"
    }
}
finally {
    Pop-Location
}
```

- [ ] **Step 2: Update package script to include TilePicker.exe when present**

In `scripts/package-windows.ps1`, after copying `tile_picker`, add:

```powershell
$tilePickerExe = Join-Path $repoRoot "dist\picker-build\TilePicker.exe"
if (Test-Path $tilePickerExe) {
    Copy-Item -LiteralPath $tilePickerExe -Destination (Join-Path $packageDir "TilePicker.exe") -Force
}
else {
    Write-Warning "TilePicker.exe was not found. Run scripts\build-tile-picker-exe.ps1 before final packaging."
}
```

- [ ] **Step 3: Build picker executable**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-tile-picker-exe.ps1
```

Expected: `dist\picker-build\TilePicker.exe` exists.

- [ ] **Step 4: Repackage**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\package-windows.ps1 -SkipBuild
```

Expected: `dist\RogueTiles\TilePicker.exe` exists.

- [ ] **Step 5: Smoke test picker executable**

Run:

```powershell
.\dist\RogueTiles\TilePicker.exe --root . --host 127.0.0.1 --port 8788
```

Expected: prints `Rogue tile picker running at http://127.0.0.1:8788/tile_picker/index.html`. Stop it with `Ctrl+C` after confirming.

- [ ] **Step 6: Commit**

```powershell
git add scripts\build-tile-picker-exe.ps1 scripts\package-windows.ps1
git commit -m "Bundle tile picker executable"
```

---

### Task 8: Final Package Verification

**Files:**
- Modify only if verification finds defects.
- Test: package smoke checks

- [ ] **Step 1: Run automated checks**

Run:

```powershell
python tests\test_tilepack_writer.py
python tests\test_tile_picker_generation.py
powershell -ExecutionPolicy Bypass -File scripts\validate-rltiles.ps1
powershell -ExecutionPolicy Bypass -File scripts\package-windows.ps1
.\dist\RogueTiles\RogueTiles.exe --tiles-smoke
```

Expected: all pass.

- [ ] **Step 2: Verify active fallback behavior**

Run:

```powershell
Rename-Item dist\RogueTiles\tilepacks\active\tilepack.json tilepack.bad.json
.\dist\RogueTiles\RogueTiles.exe --tiles-smoke
Rename-Item dist\RogueTiles\tilepacks\active\tilepack.bad.json tilepack.json
```

Expected: smoke still exits `0` while active pack is missing because default pack loads.

- [ ] **Step 3: Verify package contents**

Run:

```powershell
Get-ChildItem dist\RogueTiles | Select-Object Name,Length
Get-ChildItem dist\RogueTiles\tilepacks -Recurse | Select-Object FullName,Length
```

Expected: package contains `RogueTiles.exe`, `TilePicker.exe`, runtime DLLs, `assets`, `tilepacks/default`, and `tilepacks/active`.

- [ ] **Step 4: Commit any verification fixes**

If Task 8 required code changes, commit them:

```powershell
git add <changed-files>
git commit -m "Fix packaged runtime tilepack verification"
```

If no code changes were required, do not create an empty commit.

---

## Self-Review

- Spec coverage: runtime tile-pack loader is covered by Tasks 1-4; picker runtime writes are covered by Task 5; packaging is covered by Tasks 6-7; fallback and smoke verification are covered by Task 8.
- Placeholder scan: the plan uses concrete file names, functions, scripts, commands, and expected outputs. No placeholder markers remain.
- Type consistency: C API names are consistently `rogue_tilepack_*`; Python writer names are consistently `write_default_pack` and `write_active_custom_pack`; package layout matches the approved design.
