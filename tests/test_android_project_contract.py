import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


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
        for line in build_text.splitlines()
        if line.strip() and not line.strip().startswith("//")
    ]
    relevant_lines.extend(
        line for line in sync_script.splitlines() if "Copy-Item" in line
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
