# Rogue 5.2.1 Variant Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first distributable Rogue variant, Rogue 5.2.1, as a selectable option from the RogueTiles executable.

**Architecture:** Keep one user-facing `RogueTiles.exe` with a startup variant picker. Vendor Rogue 5.2.1 under `variants/rogue52/` with its license preserved, compile it into the same executable through prefixed symbols, and route startup into the selected engine before normal gameplay begins.

**Tech Stack:** C89/gnu89 Rogue sources, MSYS2 MinGW64, ncurses, Allegro 5 frontend, PowerShell build scripts, Python tests.

---

### Task 1: License-Safe Source Import

**Files:**
- Create: `variants/rogue52/`
- Create: `variants/rogue52/README-RogueTiles.txt`
- Preserve: `variants/rogue52/LICENSE.TXT`

- [ ] **Step 1: Download and extract Rogue 5.2.1**

Run:

```powershell
$url = "https://britzl.github.io/roguearchive/files/rogue5.2-1-src.tar.gz"
$archive = "variants\rogue52-src.tar.gz"
Invoke-WebRequest -Uri $url -OutFile $archive -UseBasicParsing
New-Item -ItemType Directory -Force variants\rogue52 | Out-Null
tar -xzf $archive -C variants\rogue52
Remove-Item $archive
```

Expected: `variants/rogue52/LICENSE.TXT` exists and contains BSD-style redistribution terms.

- [ ] **Step 2: Add provenance note**

Write `variants/rogue52/README-RogueTiles.txt` with the upstream URL, archive hash, and redistribution summary.

- [ ] **Step 3: Verify license text**

Run:

```powershell
Select-String -Path variants\rogue52\LICENSE.TXT -Pattern "Redistribution and use in source and binary forms"
```

Expected: at least one match.

### Task 2: Variant Metadata And Startup Picker

**Files:**
- Create: `variant.h`
- Create: `variant.c`
- Modify: `main.c`
- Modify: `Makefile.std`
- Test: `tests/test_variant_metadata.py`

- [ ] **Step 1: Add metadata test**

Create a Python test that reads `variant.c` and asserts `rogue54` and `rogue52` are present, that Rogue 5.2.1 has a bundled/BSD-style distribution note, and that the default variant is `rogue54`.

- [ ] **Step 2: Add variant registry**

Create `variant.h` / `variant.c` with:

```c
typedef struct rogue_variant_info {
    const char *id;
    const char *name;
    const char *era;
    const char *lineage;
    const char *license;
    const char *tile_support;
    const char *summary;
} ROGUE_VARIANT_INFO;
```

Expose lookup and selection helpers.

- [ ] **Step 3: Add startup menu**

In tile mode, show a simple Allegro text overlay before gameplay starts. In terminal mode, default to `rogue54` unless `--variant rogue52` is passed.

### Task 3: Rogue 5.2.1 Engine Entry

**Files:**
- Create: `variants/rogue52/rogue52_prefix.h`
- Modify: `Makefile.std`
- Modify: `main.c`

- [ ] **Step 1: Prefix variant symbols**

Compile Rogue 5.2.1 source with a prefix header so its globals and functions do not collide with the current Rogue 5.4.4 engine.

- [ ] **Step 2: Expose entrypoint**

Rename Rogue 5.2.1 `main` to:

```c
int rogue52_main(int argc, char **argv);
```

- [ ] **Step 3: Route selected variant**

After the startup menu or `--variant rogue52`, call `rogue52_main(argc, argv)`.

### Task 4: Build And Smoke Tests

**Files:**
- Modify: `scripts/build-windows-native.ps1`
- Test: `tests/test_variant_metadata.py`

- [ ] **Step 1: Run metadata test**

Run:

```powershell
python tests\test_variant_metadata.py
```

Expected: pass.

- [ ] **Step 2: Run existing tests**

Run:

```powershell
python tests\test_combat_damage_log.py
python tests\test_tilepack_writer.py
python tests\test_tile_picker_packaged.py
python tests\test_tile_picker_generation.py
```

Expected: all pass.

- [ ] **Step 3: Build tiles executable**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-windows-native.ps1 -Tiles
```

Expected: `native-build\rogue54.exe` builds successfully.

- [ ] **Step 4: Smoke both variants**

Run:

```powershell
native-build\rogue54.exe --tiles-smoke --variant rogue54
native-build\rogue54.exe --tiles-smoke --variant rogue52
```

Expected: both commands exit 0.
