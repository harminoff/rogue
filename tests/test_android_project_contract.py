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


def test_android_build_uses_cmake_and_default_only_native_flag():
    build_gradle = read("android/app/build.gradle")
    cmake = read("android/app/src/main/cpp/CMakeLists.txt")
    assert "externalNativeBuild" in build_gradle
    assert "src/main/cpp/CMakeLists.txt" in build_gradle
    assert re.search(r"\bROGUE_ANDROID\b", cmake)
    assert "ROGUE_ANDROID_DEFAULT_ONLY" in cmake
    assert "allegro_frontend.c" in cmake
    assert "mobile_controls.c" in cmake
    assert "rogue_platform.c" in cmake


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
    assert "extends AllegroActivity" in activity
    assert 'System.loadLibrary("allegro")' in activity
    assert 'System.loadLibrary("allegro_image")' in activity
    assert 'System.loadLibrary("allegro_font")' in activity
    assert 'System.loadLibrary("allegro_ttf")' in activity
    assert 'System.loadLibrary("allegro_primitives")' in activity
    assert 'System.loadLibrary("roguetiles")' in activity
    assert 'super("libroguetiles.so")' in activity
    assert "nativeConfigureStorage" in activity


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


def test_android_cmake_uses_default_ruleset_only():
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
    assert "variants/rogue52" not in cmake
    assert "variants/rogue36" not in cmake
    assert "variants/srogue90" not in cmake
