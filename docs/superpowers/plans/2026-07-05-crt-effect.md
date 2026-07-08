# CRT Effect Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a visual-only CRT postprocess effect with Off, Subtle, Balanced, and Dramatic intensity modes.

**Architecture:** Extend the existing Allegro frontend settings model and Shader Settings menu, then fold CRT into the current scene postprocess path. Keep the compatibility-first CPU bitmap postprocess behavior while also documenting the intended GLSL uniforms and shader logic in the existing postprocess shader source.

**Tech Stack:** C, Allegro 5, GLSL source strings, Python unittest-style contract tests, PowerShell Windows-native build scripts.

---

## File Structure

- Modify `tests/test_combat_damage_log.py`: add string-contract tests for CRT settings persistence, menu cycling, postprocess logic, and shader-smoke enablement.
- Modify `allegro_frontend.c`: add CRT mode state, settings JSON persistence, menu cycling, postprocess enablement, CPU CRT pixel processing, GLSL source additions, and shader-smoke mode activation.
- Modify `README.md`: add CRT Effect to the Shader settings list.

No new runtime source files are needed. The current shader and postprocess code already lives in `allegro_frontend.c`; splitting it during this feature would add churn without improving the first CRT pass.

### Task 1: Add CRT Settings And Menu Contract

**Files:**
- Modify: `tests/test_combat_damage_log.py`
- Modify: `allegro_frontend.c`

- [ ] **Step 1: Write the failing settings/menu test**

Add this test after `test_pixel_sharpen_and_posterize_postprocess_settings` in `tests/test_combat_damage_log.py`:

```python
    def test_crt_effect_modes_are_persisted_and_menu_driven(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        self.assertIn("ROGUE_CRT_OFF", allegro_c)
        self.assertIn("ROGUE_CRT_SUBTLE", allegro_c)
        self.assertIn("ROGUE_CRT_BALANCED", allegro_c)
        self.assertIn("ROGUE_CRT_DRAMATIC", allegro_c)
        self.assertIn("crt_effect_mode", allegro_c)
        self.assertIn('"crtEffect"', allegro_c)
        self.assertIn("crt_effect_label", allegro_c)
        self.assertIn("cycle_crt_effect_mode", allegro_c)
        self.assertIn("CRT Effect", allegro_c)

        settings_menu = allegro_c[
            allegro_c.index("show_shader_settings_menu"):
            allegro_c.index("show_tilepack_menu")
        ]
        self.assertIn('"f) CRT Effect: %s"', settings_menu)
        self.assertIn("crt_effect_label(settings.crt_effect_mode)", settings_menu)
        self.assertIn("cycle_crt_effect_mode();", settings_menu)
        self.assertIn("save_settings();", settings_menu)

        cycle = allegro_c[
            allegro_c.index("cycle_crt_effect_mode"):
            allegro_c.index("static bool\npostprocess_enabled")
        ]
        self.assertIn("ROGUE_CRT_SUBTLE", cycle)
        self.assertIn("ROGUE_CRT_BALANCED", cycle)
        self.assertIn("ROGUE_CRT_DRAMATIC", cycle)
        self.assertIn("ROGUE_CRT_OFF", cycle)
```

- [ ] **Step 2: Run the focused test and verify it fails**

Run:

```powershell
python tests\test_combat_damage_log.py
```

Expected result: the new test fails because `ROGUE_CRT_OFF` and `crt_effect_mode` are not present.

- [ ] **Step 3: Add CRT mode state**

In `allegro_frontend.c`, add this enum after `typedef enum rogue_allegro_view`:

```c
typedef enum rogue_crt_effect_mode {
    ROGUE_CRT_OFF = 0,
    ROGUE_CRT_SUBTLE = 1,
    ROGUE_CRT_BALANCED = 2,
    ROGUE_CRT_DRAMATIC = 3
} ROGUE_CRT_EFFECT_MODE;
```

Add this field to `ROGUE_ALLEGRO_SETTINGS` after `posterize_enabled`:

```c
    ROGUE_CRT_EFFECT_MODE crt_effect_mode;
```

Add this initializer after the existing `posterize_enabled` initializer:

```c
    ROGUE_CRT_OFF,
```

- [ ] **Step 4: Add helpers before `postprocess_enabled`**

Insert these helpers after `cycle_wall_thickness`:

```c
static const char *
crt_effect_label(ROGUE_CRT_EFFECT_MODE mode)
{
    switch (mode)
    {
        case ROGUE_CRT_SUBTLE:
            return "Subtle";
        case ROGUE_CRT_BALANCED:
            return "Balanced";
        case ROGUE_CRT_DRAMATIC:
            return "Dramatic";
        default:
            return "Off";
    }
}

static void
cycle_crt_effect_mode(void)
{
    switch (settings.crt_effect_mode)
    {
        case ROGUE_CRT_OFF:
            settings.crt_effect_mode = ROGUE_CRT_SUBTLE;
            break;
        case ROGUE_CRT_SUBTLE:
            settings.crt_effect_mode = ROGUE_CRT_BALANCED;
            break;
        case ROGUE_CRT_BALANCED:
            settings.crt_effect_mode = ROGUE_CRT_DRAMATIC;
            break;
        default:
            settings.crt_effect_mode = ROGUE_CRT_OFF;
            break;
    }
}
```

- [ ] **Step 5: Persist `crtEffect`**

In `load_settings`, add after the `posterize` load:

```c
    settings.crt_effect_mode = (ROGUE_CRT_EFFECT_MODE)json_int_field(
        text, "crtEffect", settings.crt_effect_mode,
        ROGUE_CRT_OFF, ROGUE_CRT_DRAMATIC);
```

In `save_settings`, add `"crtEffect": %d,` after `"posterize": %s,` and pass `settings.crt_effect_mode` after the posterize argument:

```c
        "  \"posterize\": %s,\n"
        "  \"crtEffect\": %d,\n"
        "  \"wallThickness\": %d\n"
```

```c
        settings.posterize_enabled ? "true" : "false",
        settings.crt_effect_mode,
        settings.wall_thickness);
```

- [ ] **Step 6: Add the Shader Settings menu row**

In `show_shader_settings_menu`, add after the Posterize row:

```c
        snprintf(line, sizeof(line), "f) CRT Effect: %s",
                 crt_effect_label(settings.crt_effect_mode));
        rogue_allegro_text_overlay_add(line);
```

Add this switch case after the Posterize case:

```c
        case 'f':
        case 'F':
            cycle_crt_effect_mode();
            save_settings();
            break;
```

- [ ] **Step 7: Run the focused test and verify it passes**

Run:

```powershell
python tests\test_combat_damage_log.py
```

Expected result: all tests in the file pass.

- [ ] **Step 8: Commit Task 1**

Run:

```powershell
git add tests\test_combat_damage_log.py allegro_frontend.c
git commit -m "feat: add crt effect setting"
```

### Task 2: Add CRT Postprocess Contract And Implementation

**Files:**
- Modify: `tests/test_combat_damage_log.py`
- Modify: `allegro_frontend.c`

- [ ] **Step 1: Write the failing postprocess test**

Add this test after `test_crt_effect_modes_are_persisted_and_menu_driven`:

```python
    def test_crt_effect_participates_in_postprocess_pipeline(self):
        allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")

        postprocess_enabled = allegro_c[
            allegro_c.index("postprocess_enabled"):
            allegro_c.index("postprocess_pixel_shader_source")
        ]
        self.assertIn("settings.crt_effect_mode != ROGUE_CRT_OFF", postprocess_enabled)

        shader_source = allegro_c[
            allegro_c.index("postprocess_pixel_shader_source"):
            allegro_c.index("gloom_pixel_shader_source")
        ]
        self.assertIn("u_crt_effect_mode", shader_source)
        self.assertIn("u_crt_scanline_strength", shader_source)
        self.assertIn("u_crt_vignette_strength", shader_source)
        self.assertIn("u_crt_rgb_offset", shader_source)
        self.assertIn("scanline", shader_source)

        postprocess_draw = allegro_c[
            allegro_c.index("draw_scene_with_postprocess_shader"):
            allegro_c.index("draw_scene_with_gloom_shader")
        ]
        self.assertIn("crt_sample_coordinates", postprocess_draw)
        self.assertIn("crt_scanline_factor", postprocess_draw)
        self.assertIn("crt_vignette_factor", postprocess_draw)
        self.assertIn("crt_rgb_offset_pixels", postprocess_draw)
        self.assertIn("settings.crt_effect_mode", postprocess_draw)
```

- [ ] **Step 2: Run the focused test and verify it fails**

Run:

```powershell
python tests\test_combat_damage_log.py
```

Expected result: the new test fails because CRT is not in `postprocess_enabled` and no CRT helper names exist.

- [ ] **Step 3: Add CRT constants and helper declarations**

Add these constants near the existing shader constants:

```c
#define ROGUE_CRT_SUBTLE_SCANLINE_STRENGTH 0.10f
#define ROGUE_CRT_BALANCED_SCANLINE_STRENGTH 0.18f
#define ROGUE_CRT_DRAMATIC_SCANLINE_STRENGTH 0.30f
#define ROGUE_CRT_SUBTLE_VIGNETTE_STRENGTH 0.08f
#define ROGUE_CRT_BALANCED_VIGNETTE_STRENGTH 0.16f
#define ROGUE_CRT_DRAMATIC_VIGNETTE_STRENGTH 0.28f
#define ROGUE_CRT_BALANCED_CURVATURE 0.035f
#define ROGUE_CRT_DRAMATIC_CURVATURE 0.070f
#define ROGUE_CRT_BALANCED_RGB_OFFSET 1
#define ROGUE_CRT_DRAMATIC_RGB_OFFSET 2
```

Add declarations near the existing `read_locked_rgba` declaration:

```c
static void crt_sample_coordinates(ROGUE_CRT_EFFECT_MODE mode, int x, int y,
                                  int *sample_x, int *sample_y);
static float crt_scanline_factor(ROGUE_CRT_EFFECT_MODE mode, int y);
static float crt_vignette_factor(ROGUE_CRT_EFFECT_MODE mode, int x, int y);
static int crt_rgb_offset_pixels(ROGUE_CRT_EFFECT_MODE mode);
```

- [ ] **Step 4: Include CRT in postprocess enablement**

Update `postprocess_enabled`:

```c
static bool
postprocess_enabled(void)
{
    return (bool)(settings.pixel_sharpen_enabled
                  || settings.posterize_enabled
                  || settings.crt_effect_mode != ROGUE_CRT_OFF);
}
```

- [ ] **Step 5: Update the GLSL source string**

Add these uniforms to `postprocess_pixel_shader_source` after `u_posterize_enabled`:

```c
    "uniform int u_crt_effect_mode;\n"
    "uniform float u_crt_scanline_strength;\n"
    "uniform float u_crt_vignette_strength;\n"
    "uniform float u_crt_rgb_offset;\n"
```

Add this block before the Posterize block:

```c
    "    if (u_crt_effect_mode > 0)\n"
    "    {\n"
    "        float scanline = 1.0 - u_crt_scanline_strength * (0.5 + 0.5 * sin(gl_FragCoord.y * 3.14159));\n"
    "        vec2 centered = uv - vec2(0.5, 0.5);\n"
    "        float vignette = 1.0 - u_crt_vignette_strength * smoothstep(0.28, 0.76, length(centered));\n"
    "        vec2 red_uv = clamp(uv + vec2(u_crt_rgb_offset, 0.0) * u_scene_texel_size, vec2(0.0, 0.0), vec2(1.0, 1.0));\n"
    "        vec2 blue_uv = clamp(uv - vec2(u_crt_rgb_offset, 0.0) * u_scene_texel_size, vec2(0.0, 0.0), vec2(1.0, 1.0));\n"
    "        color.r = texture2D(u_scene_texture, red_uv).r;\n"
    "        color.b = texture2D(u_scene_texture, blue_uv).b;\n"
    "        color.rgb *= scanline * vignette;\n"
    "    }\n"
```

- [ ] **Step 6: Implement CPU CRT helpers**

Add these helpers before `draw_scene_with_postprocess_shader`:

```c
static float
crt_mode_scanline_strength(ROGUE_CRT_EFFECT_MODE mode)
{
    switch (mode)
    {
        case ROGUE_CRT_SUBTLE:
            return ROGUE_CRT_SUBTLE_SCANLINE_STRENGTH;
        case ROGUE_CRT_BALANCED:
            return ROGUE_CRT_BALANCED_SCANLINE_STRENGTH;
        case ROGUE_CRT_DRAMATIC:
            return ROGUE_CRT_DRAMATIC_SCANLINE_STRENGTH;
        default:
            return 0.0f;
    }
}

static float
crt_mode_vignette_strength(ROGUE_CRT_EFFECT_MODE mode)
{
    switch (mode)
    {
        case ROGUE_CRT_SUBTLE:
            return ROGUE_CRT_SUBTLE_VIGNETTE_STRENGTH;
        case ROGUE_CRT_BALANCED:
            return ROGUE_CRT_BALANCED_VIGNETTE_STRENGTH;
        case ROGUE_CRT_DRAMATIC:
            return ROGUE_CRT_DRAMATIC_VIGNETTE_STRENGTH;
        default:
            return 0.0f;
    }
}

static float
crt_mode_curvature(ROGUE_CRT_EFFECT_MODE mode)
{
    switch (mode)
    {
        case ROGUE_CRT_BALANCED:
            return ROGUE_CRT_BALANCED_CURVATURE;
        case ROGUE_CRT_DRAMATIC:
            return ROGUE_CRT_DRAMATIC_CURVATURE;
        default:
            return 0.0f;
    }
}

static int
crt_rgb_offset_pixels(ROGUE_CRT_EFFECT_MODE mode)
{
    switch (mode)
    {
        case ROGUE_CRT_BALANCED:
            return ROGUE_CRT_BALANCED_RGB_OFFSET;
        case ROGUE_CRT_DRAMATIC:
            return ROGUE_CRT_DRAMATIC_RGB_OFFSET;
        default:
            return 0;
    }
}

static void
crt_sample_coordinates(ROGUE_CRT_EFFECT_MODE mode, int x, int y,
                       int *sample_x, int *sample_y)
{
    float curvature;
    float nx;
    float ny;
    float dx;
    float dy;
    float warp;
    int sx;
    int sy;

    curvature = crt_mode_curvature(mode);
    if (curvature <= 0.0f || scene_bitmap_width <= 1
        || scene_bitmap_height <= 1)
    {
        *sample_x = x;
        *sample_y = y;
        return;
    }

    nx = ((float)x / (float)(scene_bitmap_width - 1)) * 2.0f - 1.0f;
    ny = ((float)y / (float)(scene_bitmap_height - 1)) * 2.0f - 1.0f;
    dx = nx;
    dy = ny;
    warp = 1.0f + curvature * (dx * dx + dy * dy);
    sx = (int)(((nx * warp + 1.0f) * 0.5f
                * (float)(scene_bitmap_width - 1)) + 0.5f);
    sy = (int)(((ny * warp + 1.0f) * 0.5f
                * (float)(scene_bitmap_height - 1)) + 0.5f);

    if (sx < 0)
        sx = 0;
    if (sx >= scene_bitmap_width)
        sx = scene_bitmap_width - 1;
    if (sy < 0)
        sy = 0;
    if (sy >= scene_bitmap_height)
        sy = scene_bitmap_height - 1;
    *sample_x = sx;
    *sample_y = sy;
}

static float
crt_scanline_factor(ROGUE_CRT_EFFECT_MODE mode, int y)
{
    float strength;

    strength = crt_mode_scanline_strength(mode);
    if (strength <= 0.0f)
        return 1.0f;
    return (y % 2) == 0 ? 1.0f : 1.0f - strength;
}

static float
crt_vignette_factor(ROGUE_CRT_EFFECT_MODE mode, int x, int y)
{
    float strength;
    float nx;
    float ny;
    float dist;
    float edge;

    strength = crt_mode_vignette_strength(mode);
    if (strength <= 0.0f || scene_bitmap_width <= 1
        || scene_bitmap_height <= 1)
        return 1.0f;

    nx = ((float)x / (float)(scene_bitmap_width - 1)) * 2.0f - 1.0f;
    ny = ((float)y / (float)(scene_bitmap_height - 1)) * 2.0f - 1.0f;
    dist = nx * nx + ny * ny;
    edge = dist > 1.0f ? 1.0f : dist;
    return 1.0f - strength * edge;
}
```

- [ ] **Step 7: Apply CRT inside the CPU postprocess loop**

In `draw_scene_with_postprocess_shader`, add local variables after the existing channel locals:

```c
    int sample_x, sample_y;
    int rgb_offset;
    int red_x, blue_x;
    float crt_factor;
```

Replace the first pixel read inside the loop:

```c
            read_locked_rgba(source_lock, x, y, &r, &g, &b, &a);
```

with:

```c
            crt_sample_coordinates(settings.crt_effect_mode, x, y,
                                   &sample_x, &sample_y);
            read_locked_rgba(source_lock, sample_x, sample_y,
                             &r, &g, &b, &a);
            if (settings.crt_effect_mode != ROGUE_CRT_OFF)
            {
                rgb_offset = crt_rgb_offset_pixels(settings.crt_effect_mode);
                red_x = sample_x + rgb_offset;
                blue_x = sample_x - rgb_offset;
                if (red_x >= scene_bitmap_width)
                    red_x = scene_bitmap_width - 1;
                if (blue_x < 0)
                    blue_x = 0;
                read_locked_rgba(source_lock, red_x, sample_y,
                                 &r, &north_g, &north_b, &ignored_a);
                read_locked_rgba(source_lock, blue_x, sample_y,
                                 &north_r, &south_g, &b, &ignored_a);
                crt_factor =
                    crt_scanline_factor(settings.crt_effect_mode, y)
                    * crt_vignette_factor(settings.crt_effect_mode, x, y);
                r = (unsigned char)channel_clamp((float)r * crt_factor);
                g = (unsigned char)channel_clamp((float)g * crt_factor);
                b = (unsigned char)channel_clamp((float)b * crt_factor);
            }
```

In the pixel-sharpen neighbor reads, use `sample_x` and `sample_y` instead of `x` and `y` for the center-relative source sample coordinates, clamping to `scene_bitmap_width` and `scene_bitmap_height`:

```c
                read_locked_rgba(source_lock, sample_x,
                                 sample_y > 0 ? sample_y - 1 : sample_y,
                                 &north_r, &north_g, &north_b, &ignored_a);
                read_locked_rgba(source_lock, sample_x,
                                 sample_y + 1 < scene_bitmap_height
                                     ? sample_y + 1 : sample_y,
                                 &south_r, &south_g, &south_b, &ignored_a);
                read_locked_rgba(source_lock,
                                 sample_x + 1 < scene_bitmap_width
                                     ? sample_x + 1 : sample_x,
                                 sample_y, &east_r, &east_g, &east_b,
                                 &ignored_a);
                read_locked_rgba(source_lock,
                                 sample_x > 0 ? sample_x - 1 : sample_x,
                                 sample_y, &west_r, &west_g, &west_b,
                                 &ignored_a);
```

- [ ] **Step 8: Run the focused test and verify it passes**

Run:

```powershell
python tests\test_combat_damage_log.py
```

Expected result: all tests in the file pass.

- [ ] **Step 9: Commit Task 2**

Run:

```powershell
git add tests\test_combat_damage_log.py allegro_frontend.c
git commit -m "feat: add crt postprocess effect"
```

### Task 3: Add Smoke Enablement And README Coverage

**Files:**
- Modify: `tests/test_combat_damage_log.py`
- Modify: `allegro_frontend.c`
- Modify: `README.md`

- [ ] **Step 1: Write the failing smoke/docs test**

Extend `test_shader_diagnostic_smoke_saves_render_targets` with:

```python
        self.assertIn("settings.crt_effect_mode = ROGUE_CRT_DRAMATIC", allegro_c)
```

Add this README assertion near the shader settings assertions:

```python
    def test_readme_lists_crt_effect_shader_setting(self):
        readme = (ROOT / "README.md").read_text(encoding="utf-8")

        self.assertIn("CRT Effect", readme)
        self.assertIn("Off/Subtle/Balanced/Dramatic", readme)
```

- [ ] **Step 2: Run the focused test and verify it fails**

Run:

```powershell
python tests\test_combat_damage_log.py
```

Expected result: the new assertions fail because shader smoke does not force CRT and README does not mention CRT.

- [ ] **Step 3: Enable CRT in shader smoke mode**

In `rogue_allegro_start`, update the `shader_smoke_mode` block:

```c
    if (shader_smoke_mode)
    {
        settings.dungeon_gloom_enabled = TRUE;
        settings.pixel_sharpen_enabled = TRUE;
        settings.posterize_enabled = TRUE;
        settings.crt_effect_mode = ROGUE_CRT_DRAMATIC;
    }
```

- [ ] **Step 4: Document the new setting**

In `README.md`, add this bullet after Posterize in the Shader settings list:

```markdown
- CRT Effect: cycles Off/Subtle/Balanced/Dramatic CRT-style scanlines, vignette, curvature, and color separation.
```

- [ ] **Step 5: Run the focused test and verify it passes**

Run:

```powershell
python tests\test_combat_damage_log.py
```

Expected result: all tests in the file pass.

- [ ] **Step 6: Commit Task 3**

Run:

```powershell
git add tests\test_combat_damage_log.py allegro_frontend.c README.md
git commit -m "docs: document crt shader setting"
```

### Task 4: Build And Runtime Verification

**Files:**
- No code edits.

- [ ] **Step 1: Run whitespace check**

Run:

```powershell
git diff --check
```

Expected result: no output and exit code 0.

- [ ] **Step 2: Run focused shader tests**

Run:

```powershell
python tests\test_combat_damage_log.py
```

Expected result: all tests pass.

- [ ] **Step 3: Build native Windows tile executable**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-windows-native.ps1 -Tiles
```

Expected result: build exits 0 and reports `Built: C:\Programming\Repos\RogueTiles\native-build\rogue54.exe`.

- [ ] **Step 4: Run shader smoke**

Run:

```powershell
native-build\rogue54.exe --tiles-shader-smoke --variant rogue54
```

Expected result: command exits 0. Diagnostic bitmaps are written beside `settings.json`, including `rogue_scene_before_shader.png`, `rogue_scene_source_shader.png`, `rogue_scene_after_postprocess.png`, and `rogue_scene_after_shader.png`.

- [ ] **Step 5: Commit verification-only changes if any generated diagnostics are intentionally tracked**

Run:

```powershell
git status --short
```

Expected result: shader diagnostic PNGs are untracked or ignored. Do not commit diagnostic images unless the repo already tracks them.

### Task 5: Final Review

**Files:**
- Review: `allegro_frontend.c`
- Review: `tests/test_combat_damage_log.py`
- Review: `README.md`

- [ ] **Step 1: Inspect final diff**

Run:

```powershell
git diff HEAD~3..HEAD -- allegro_frontend.c tests\test_combat_damage_log.py README.md
```

Expected result: the diff contains only CRT effect setting, postprocess, smoke, tests, and README changes.

- [ ] **Step 2: Confirm working tree state**

Run:

```powershell
git status --short
```

Expected result: no tracked source files are modified. Untracked shader smoke images may exist and should remain uncommitted.
