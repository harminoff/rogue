import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def strip_gradle_comment(line: str) -> str:
    return line.split("//", 1)[0].strip()


def strip_powershell_comment(line: str) -> str:
    return line.split("#", 1)[0].strip()


def test_android_project_files_exist():
    required = [
        "android/settings.gradle",
        "android/build.gradle",
        "android/app/build.gradle",
        "android/app/src/main/AndroidManifest.xml",
        "android/app/src/main/java/com/roguetiles/RogueTilesActivity.java",
        "android/app/src/main/cpp/CMakeLists.txt",
        "android/vendor/allegro/README.md",
        "scripts/check-android-allegro.ps1",
        "scripts/sync-android-assets.ps1",
        "scripts/build-android-debug.ps1",
    ]
    missing = [path for path in required if not (ROOT / path).exists()]
    assert missing == []


def test_android_build_uses_cmake_and_bundled_variant_native_sources():
    build_gradle = read("android/app/build.gradle")
    cmake = read("android/app/src/main/cpp/CMakeLists.txt")
    assert "externalNativeBuild" in build_gradle
    assert "src/main/cpp/CMakeLists.txt" in build_gradle
    assert re.search(r"\bROGUE_ANDROID\b", cmake)
    assert "ROGUE_ANDROID_DEFAULT_ONLY" not in cmake
    assert "allegro_frontend.c" in cmake
    assert "mobile_controls.c" in cmake
    assert "rogue_platform.c" in cmake
    assert "add_rogue_variant_objects(rogue52" in cmake
    assert "add_rogue_variant_objects(rogue36" in cmake
    assert "add_rogue_variant_objects(srogue90" in cmake
    assert "android_variant_bridge_stubs.c" not in cmake


def test_android_app_excludes_tile_editor_artifacts():
    build_text = read("android/app/build.gradle")
    sync_script = read("scripts/sync-android-assets.ps1")
    relevant_lines = [
        line
        for line in (strip_gradle_comment(line) for line in build_text.splitlines())
        if line
    ]
    relevant_lines.extend(
        line
        for line in (
            strip_powershell_comment(line) for line in sync_script.splitlines()
        )
        if line and "Copy-Item" in line
    )
    forbidden = ["tile_picker", "TilePicker.exe", "RogueTiles-windows-x64.zip"]
    for value in forbidden:
        assert all(value not in line for line in relevant_lines)


def test_android_activity_loads_allegro_and_roguetiles_library():
    activity = read("android/app/src/main/java/com/roguetiles/RogueTilesActivity.java")
    platform = read("rogue_platform.c")
    header = read("rogue_platform.h")
    assert "extends AllegroActivity" in activity
    assert 'System.loadLibrary("allegro")' in activity
    assert 'System.loadLibrary("allegro_image")' in activity
    assert 'System.loadLibrary("allegro_font")' in activity
    assert 'System.loadLibrary("allegro_ttf")' in activity
    assert 'System.loadLibrary("allegro_primitives")' in activity
    assert 'System.loadLibrary("roguetiles")' in activity
    assert 'super("libroguetiles.so")' in activity
    assert "nativeConfigureStorage" in activity
    assert "nativeConfigureSafeArea" in activity
    assert "WindowInsets" in activity
    assert "DisplayCutout" in activity
    assert "getSafeInsetTop()" in activity
    assert "setOnApplyWindowInsetsListener" in activity
    assert "Java_com_roguetiles_RogueTilesActivity_nativeConfigureSafeArea" in platform
    assert "rogue_platform_configure_safe_area" in platform
    assert "rogue_platform_android_safe_top_inset" in header


def test_android_debug_build_requires_project_gradle_wrapper():
    build_script = read("scripts/build-android-debug.ps1")
    assert '$gradlew = Join-Path $androidRoot "gradlew.bat"' in build_script
    assert "Android Gradle wrapper not found." in build_script
    assert "& $gradlew :app:assembleDebug" in build_script
    assert "gradle :app:assembleDebug" not in build_script.replace(
        "& $gradlew :app:assembleDebug", ""
    )


def test_android_asset_sync_clears_stale_bundled_assets_before_copying():
    asset_sync = read("android/app/src/main/java/com/roguetiles/AndroidAssetSync.java")
    assert 'new File(context.getFilesDir(), "bundled-assets")' in asset_sync
    assert "deleteTree(root);" in asset_sync
    assert asset_sync.index("deleteTree(root);") < asset_sync.index(
        'copyTree(context.getAssets(), "assets", root);'
    )
    assert "private static void deleteTree(File target)" in asset_sync
    assert "target.delete()" in asset_sync


def test_android_asset_sync_includes_readable_manuals():
    sync_script = read("scripts/sync-android-assets.ps1")
    asset_sync = read("android/app/src/main/java/com/roguetiles/AndroidAssetSync.java")

    assert 'New-Item -ItemType Directory -Force (Join-Path $assetRoot "variants")' in sync_script
    assert 'Copy-Item -LiteralPath (Join-Path $repoRoot "variants\\rogue52\\rogue.6")' in sync_script
    assert 'Copy-Item -LiteralPath (Join-Path $repoRoot "variants\\rogue36\\rogue.r")' in sync_script
    assert 'Copy-Item -LiteralPath (Join-Path $repoRoot "variants\\srogue90\\rogue.nr")' in sync_script
    assert 'Copy-Item -LiteralPath (Join-Path $repoRoot "rogue54.doc")' in sync_script
    assert 'copyTree(context.getAssets(), "variants", root);' in asset_sync


def test_android_curses_new_windows_have_separate_backing_storage():
    curses = read("android/app/src/main/cpp/android_curses.c")
    cell_at_body = curses[curses.index("cell_at(WINDOW *win"):
                          curses.index("static void\nwindow_fill")]
    newwin_body = curses[curses.index("newwin(int rows, int cols"):
                         curses.index("WINDOW *\nsubwin")]
    delwin_body = curses[curses.index("delwin(WINDOW *win)"):
                         curses.index("int\nmvwin")]

    assert "win->owns_cells" in cell_at_body
    assert "return &win->cells[y * win->cols + x];" in cell_at_body
    assert "win->owns_cells = 1;" in newwin_body
    assert "calloc((size_t) rows * (size_t) cols" in newwin_body
    assert "win->cells == NULL" in newwin_body
    assert "free(win->cells);" in delwin_body


def test_android_cmake_uses_default_ruleset_and_playable_variants():
    cmake = read("android/app/src/main/cpp/CMakeLists.txt")
    base_sources = [
        "vers.c",
        "extern.c",
        "armor.c",
        "chase.c",
        "command.c",
        "daemon.c",
        "daemons.c",
        "fight.c",
        "init.c",
        "io.c",
        "list.c",
        "mach_dep.c",
        "main.c",
        "mdport.c",
        "misc.c",
        "monsters.c",
        "move.c",
        "new_level.c",
        "options.c",
        "pack.c",
        "passages.c",
        "potions.c",
        "rings.c",
        "rip.c",
        "rooms.c",
        "save.c",
        "scrolls.c",
        "state.c",
        "sticks.c",
        "things.c",
        "tiles.c",
        "tilepack.c",
        "frontend.c",
        "overlay_picker.c",
        "variant.c",
        "rogue_platform.c",
        "mobile_controls.c",
        "generated/rogue_tile_mapping.c",
        "allegro_frontend.c",
        "weapons.c",
        "wizard.c",
        "xcrypt.c",
    ]
    for source in base_sources:
        assert source in cmake
    variant_sources = [
        "rogue52_port.c",
        "rogue36_port.c",
        "srogue90_port.c",
        "_symbols.redef",
    ]
    for source in variant_sources:
        assert source in cmake
