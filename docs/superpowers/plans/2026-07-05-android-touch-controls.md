# Android Touch Controls Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the first playable Android APK for RogueTiles with the graphical Allegro frontend, bundled default tiles, and large touch direction controls.

**Architecture:** Add an `android/` Gradle/CMake app beside the Windows build, keeping existing Windows scripts unchanged. The first APK compiles the default RogueTiles ruleset only under `ROGUE_ANDROID_DEFAULT_ONLY`, forces graphical tile mode, and feeds touch controls into the existing single-character Rogue command path. Shared mobile button layout and hit testing live in small C files so behavior can be tested without launching Android.

**Tech Stack:** C89/C99-compatible Rogue core, Allegro 5 Android runtime, Android Gradle Plugin, CMake externalNativeBuild, Java `AllegroActivity`, Python contract tests, small C control tests.

---

## Context And Constraints

Current Windows builds compile the full RogueTiles collection through `Makefile.std` and variant symbol rewriting. The first Android APK should avoid that linker complexity and compile the default ruleset only. Windows packaging remains unchanged and keeps all variants, the tile picker, and runtime tilepack editing.

Current docs consulted:

- Android Developers external native build docs: Gradle app modules can link CMake through `android.externalNativeBuild.cmake.path` and build with `:app:assembleDebug`.
- Allegro 5 Android docs: an Android activity extends `org.liballeg.android.AllegroActivity`, loads Allegro shared libraries, passes the app shared library name to `super(...)`, and touch input is received through `al_install_touch_input()` plus `al_get_touch_input_event_source()`.

## File Structure

- `android/settings.gradle`: Android-only Gradle settings.
- `android/build.gradle`: Android root plugin repository and plugin version declarations.
- `android/app/build.gradle`: App module config, SDK versions, CMake config, ABI filters, assets, and Allegro AAR dependency.
- `android/app/src/main/AndroidManifest.xml`: Activity declaration.
- `android/app/src/main/java/com/roguetiles/RogueTilesActivity.java`: Allegro activity, asset extraction, and native storage bootstrap.
- `android/app/src/main/cpp/CMakeLists.txt`: Native library build for default RogueTiles ruleset.
- `android/app/src/main/assets/`: Copied game assets used by the APK.
- `android/vendor/allegro/README.md`: Exact local dependency contract for Allegro Android artifacts.
- `scripts/check-android-allegro.ps1`: Fails early with actionable missing-file messages.
- `scripts/sync-android-assets.ps1`: Copies only Android-required assets into the app module.
- `scripts/build-android-debug.ps1`: Runs dependency checks, asset sync, and `:app:assembleDebug`.
- `rogue_platform.h`, `rogue_platform.c`: Cross-platform asset and writable-path helpers.
- `mobile_controls.h`, `mobile_controls.c`: Pure C mobile button layout and hit-test mapping.
- `tests/test_android_project_contract.py`: Python contract tests for Android project shape.
- `tests/mobile_controls_test.c`: Native C hit-test tests.
- `scripts/test-mobile-controls.ps1`: Builds and runs the mobile controls test executable.
- Existing files modified: `main.c`, `frontend.c`, `allegro_frontend.c`, `tilepack.c`, `Makefile.std`, `scripts/build-windows-native.ps1`, `README.md`.

---

### Task 1: Add Android Project Contract Tests

**Files:**
- Create: `tests/test_android_project_contract.py`

- [ ] **Step 1: Write failing project shape tests**

Create `tests/test_android_project_contract.py`:

```python
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
    assert "ROGUE_ANDROID" in cmake
    assert "ROGUE_ANDROID_DEFAULT_ONLY" in cmake
    assert "allegro_frontend.c" in cmake
    assert "mobile_controls.c" in cmake
    assert "rogue_platform.c" in cmake


def test_android_app_excludes_tile_editor_artifacts():
    build_text = read("android/app/build.gradle")
    sync_script = read("scripts/sync-android-assets.ps1")
    forbidden = ["tile_picker", "TilePicker.exe", "RogueTiles-windows-x64.zip"]
    for value in forbidden:
        assert value not in build_text
        assert value not in sync_script


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
```

- [ ] **Step 2: Run tests to verify they fail**

Run:

```powershell
python -m pytest tests/test_android_project_contract.py -q
```

Expected: FAIL because the Android project files do not exist yet.

- [ ] **Step 3: Commit the failing tests**

Run:

```powershell
git add tests/test_android_project_contract.py
git commit -m "test: define android project contract"
```

---

### Task 2: Scaffold Android Project And Dependency Contract

**Files:**
- Create: `android/settings.gradle`
- Create: `android/build.gradle`
- Create: `android/app/build.gradle`
- Create: `android/app/src/main/AndroidManifest.xml`
- Create: `android/app/src/main/java/com/roguetiles/RogueTilesActivity.java`
- Create: `android/app/src/main/cpp/CMakeLists.txt`
- Create: `android/vendor/allegro/README.md`
- Create: `scripts/check-android-allegro.ps1`
- Create: `scripts/sync-android-assets.ps1`
- Create: `scripts/build-android-debug.ps1`

- [ ] **Step 1: Create Android Gradle settings**

Create `android/settings.gradle`:

```groovy
pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
        flatDir {
            dirs 'vendor/allegro'
        }
    }
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
        flatDir {
            dirs 'vendor/allegro'
        }
    }
}

rootProject.name = 'RogueTilesAndroid'
include ':app'
```

Create `android/build.gradle`:

```groovy
plugins {
    id 'com.android.application' version '8.5.2' apply false
}
```

- [ ] **Step 2: Create app Gradle module**

Create `android/app/build.gradle`:

```groovy
plugins {
    id 'com.android.application'
}

android {
    namespace 'com.roguetiles'
    compileSdk 35

    defaultConfig {
        applicationId 'com.roguetiles'
        minSdk 26
        targetSdk 35
        versionCode 1
        versionName '0.1.0-dev'

        externalNativeBuild {
            cmake {
                cFlags '-std=gnu89', '-DROGUE_ANDROID', '-DROGUE_ANDROID_DEFAULT_ONLY', '-DROGUE_ENABLE_ALLEGRO', '-DALLSCORES', '-DSCOREFILE="rogue54.scr"', '-DLOCKFILE="rogue54.lck"'
                arguments '-DANDROID_STL=c++_shared'
            }
        }

        ndk {
            abiFilters 'arm64-v8a', 'x86_64'
        }
    }

    externalNativeBuild {
        cmake {
            path 'src/main/cpp/CMakeLists.txt'
        }
    }
}

dependencies {
    implementation(name: 'allegro-release', ext: 'aar')
}
```

- [ ] **Step 3: Create Android manifest and activity**

Create `android/app/src/main/AndroidManifest.xml`:

```xml
<manifest xmlns:android="http://schemas.android.com/apk/res/android">
    <application
        android:allowBackup="false"
        android:extractNativeLibs="true"
        android:label="RogueTiles"
        android:theme="@style/RogueTilesTheme">
        <activity
            android:name=".RogueTilesActivity"
            android:configChanges="keyboard|keyboardHidden|orientation|screenSize"
            android:exported="true"
            android:screenOrientation="landscape">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>
```

Create `android/app/src/main/res/values/styles.xml`:

```xml
<resources>
    <style name="RogueTilesTheme" parent="@android:style/Theme.NoTitleBar.Fullscreen">
        <item name="android:windowFullscreen">true</item>
        <item name="android:windowNoTitle">true</item>
        <item name="android:windowActionBar">false</item>
        <item name="android:windowBackground">#050712</item>
    </style>
</resources>
```

Create `android/app/src/main/java/com/roguetiles/RogueTilesActivity.java`:

```java
package com.roguetiles;

import android.os.Bundle;
import org.liballeg.android.AllegroActivity;

public class RogueTilesActivity extends AllegroActivity {
    static {
        System.loadLibrary("allegro");
        System.loadLibrary("allegro_image");
        System.loadLibrary("allegro_font");
        System.loadLibrary("allegro_ttf");
        System.loadLibrary("allegro_primitives");
        System.loadLibrary("roguetiles");
    }

    public RogueTilesActivity() {
        super("libroguetiles.so");
    }

    private static native void nativeConfigureStorage(String assetRoot, String userRoot);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        String assetRoot = AndroidAssetSync.syncBundledAssets(this).getAbsolutePath();
        String userRoot = getFilesDir().getAbsolutePath();
        nativeConfigureStorage(assetRoot, userRoot);
        super.onCreate(savedInstanceState);
    }
}
```

Create `android/app/src/main/java/com/roguetiles/AndroidAssetSync.java`:

```java
package com.roguetiles;

import android.content.Context;
import android.content.res.AssetManager;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

final class AndroidAssetSync {
    private AndroidAssetSync() {
    }

    static File syncBundledAssets(Context context) {
        File root = new File(context.getFilesDir(), "bundled-assets");
        copyTree(context.getAssets(), "assets", root);
        copyTree(context.getAssets(), "tilepacks", root);
        return root;
    }

    private static void copyTree(AssetManager assets, String source, File root) {
        try {
            String[] children = assets.list(source);
            if (children == null || children.length == 0) {
                copyFile(assets, source, new File(root, source));
                return;
            }
            for (String child : children) {
                copyTree(assets, source + "/" + child, root);
            }
        } catch (IOException ex) {
            throw new IllegalStateException("Could not copy bundled asset: " + source, ex);
        }
    }

    private static void copyFile(AssetManager assets, String source, File destination) throws IOException {
        File parent = destination.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs()) {
            throw new IOException("Could not create directory: " + parent);
        }
        try (InputStream input = assets.open(source);
             OutputStream output = new FileOutputStream(destination)) {
            byte[] buffer = new byte[8192];
            int read;
            while ((read = input.read(buffer)) >= 0) {
                output.write(buffer, 0, read);
            }
        }
    }
}
```

- [ ] **Step 4: Create a CMake sentinel that intentionally fails at native target until Task 7**

Create `android/app/src/main/cpp/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.22.1)
project(roguetiles_android C)

message(FATAL_ERROR "RogueTiles Android native sources are added in Task 7")
```

- [ ] **Step 5: Document and check Allegro Android dependency contract**

Create `android/vendor/allegro/README.md`:

```markdown
# Allegro Android Dependency

Place the Android Allegro artifacts here before building the APK:

- `allegro-release.aar`
- `jni/arm64-v8a/liballegro.so`
- `jni/arm64-v8a/liballegro_image.so`
- `jni/arm64-v8a/liballegro_font.so`
- `jni/arm64-v8a/liballegro_ttf.so`
- `jni/arm64-v8a/liballegro_primitives.so`
- `jni/x86_64/liballegro.so`
- `jni/x86_64/liballegro_image.so`
- `jni/x86_64/liballegro_font.so`
- `jni/x86_64/liballegro_ttf.so`
- `jni/x86_64/liballegro_primitives.so`

The app loads these libraries from `RogueTilesActivity` before Allegro starts `libroguetiles.so`.
```

Create `scripts/check-android-allegro.ps1`:

```powershell
$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$vendor = Join-Path $repoRoot "android\vendor\allegro"
$required = @(
    "allegro-release.aar",
    "jni\arm64-v8a\liballegro.so",
    "jni\arm64-v8a\liballegro_image.so",
    "jni\arm64-v8a\liballegro_font.so",
    "jni\arm64-v8a\liballegro_ttf.so",
    "jni\arm64-v8a\liballegro_primitives.so",
    "jni\x86_64\liballegro.so",
    "jni\x86_64\liballegro_image.so",
    "jni\x86_64\liballegro_font.so",
    "jni\x86_64\liballegro_ttf.so",
    "jni\x86_64\liballegro_primitives.so"
)

$missing = @()
foreach ($path in $required) {
    $full = Join-Path $vendor $path
    if (-not (Test-Path $full)) {
        $missing += $path
    }
}

if ($missing.Count -gt 0) {
    Write-Error "Missing Allegro Android artifacts under android\vendor\allegro: $($missing -join ', ')"
}

Write-Host "Android Allegro dependency check passed."
```

- [ ] **Step 6: Create Android asset sync script**

Create `scripts/sync-android-assets.ps1`:

```powershell
$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$assetRoot = Join-Path $repoRoot "android\app\src\main\assets"

New-Item -ItemType Directory -Force $assetRoot | Out-Null

foreach ($child in Get-ChildItem -LiteralPath $assetRoot -Force) {
    Remove-Item -LiteralPath $child.FullName -Recurse -Force
}

Copy-Item -LiteralPath (Join-Path $repoRoot "assets") -Destination (Join-Path $assetRoot "assets") -Recurse -Force
New-Item -ItemType Directory -Force (Join-Path $assetRoot "tilepacks") | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot "tilepacks\default") -Destination (Join-Path $assetRoot "tilepacks\default") -Recurse -Force

Write-Host "Synced Android assets to $assetRoot"
```

Create `scripts/build-android-debug.ps1`:

```powershell
$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$androidRoot = Join-Path $repoRoot "android"

& (Join-Path $repoRoot "scripts\check-android-allegro.ps1")
& (Join-Path $repoRoot "scripts\sync-android-assets.ps1")

Push-Location $androidRoot
try {
    if (Test-Path ".\gradlew.bat") {
        .\gradlew.bat :app:assembleDebug
    }
    else {
        gradle :app:assembleDebug
    }
    if ($LASTEXITCODE -ne 0) {
        throw "Android debug build failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
```

- [ ] **Step 7: Run contract tests**

Run:

```powershell
python -m pytest tests/test_android_project_contract.py -q
```

Expected: PASS.

- [ ] **Step 8: Commit scaffold**

Run:

```powershell
git add android scripts/check-android-allegro.ps1 scripts/sync-android-assets.ps1 scripts/build-android-debug.ps1
git commit -m "build: scaffold android app project"
```

---

### Task 3: Add Platform Path Helpers

**Files:**
- Create: `rogue_platform.h`
- Create: `rogue_platform.c`
- Modify: `tilepack.c`
- Modify: `allegro_frontend.c`
- Modify: `Makefile.std`
- Modify: `scripts/build-windows-native.ps1`
- Create: `tests/test_android_platform_paths.py`

- [ ] **Step 1: Write platform path tests**

Create `tests/test_android_platform_paths.py`:

```python
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def test_platform_path_api_exists():
    header = (ROOT / "rogue_platform.h").read_text(encoding="utf-8")
    assert "rogue_platform_configure_storage" in header
    assert "rogue_platform_asset_path" in header
    assert "rogue_platform_user_path" in header


def test_tilepack_uses_platform_asset_paths():
    text = (ROOT / "tilepack.c").read_text(encoding="utf-8")
    assert '#include "rogue_platform.h"' in text
    assert "rogue_platform_read_text_file" in text
    assert "rogue_platform_asset_path" in text
    assert 'fopen(path, "rb")' not in text


def test_allegro_frontend_uses_platform_paths_for_settings_and_atlas():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert '#include "rogue_platform.h"' in text
    assert "rogue_platform_user_path" in text
    assert "rogue_platform_asset_path" in text
```

- [ ] **Step 2: Run tests to verify they fail**

Run:

```powershell
python -m pytest tests/test_android_platform_paths.py -q
```

Expected: FAIL because `rogue_platform.h` does not exist yet.

- [ ] **Step 3: Add platform API**

Create `rogue_platform.h`:

```c
#ifndef ROGUE_PLATFORM_H
#define ROGUE_PLATFORM_H

#include <stddef.h>
#include <stdbool.h>

void rogue_platform_configure_storage(const char *asset_root, const char *user_root);
const char *rogue_platform_asset_path(const char *relative, char *out, size_t out_size);
const char *rogue_platform_user_path(const char *relative, char *out, size_t out_size);
char *rogue_platform_read_text_file(const char *relative_path);

#endif
```

Create `rogue_platform.c`:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rogue_platform.h"

#ifdef ROGUE_ANDROID
#include <jni.h>
#endif

#define ROGUE_PLATFORM_PATH_MAX 1024
#define ROGUE_PLATFORM_MAX_TEXT 262144

static char platform_asset_root[ROGUE_PLATFORM_PATH_MAX] = "";
static char platform_user_root[ROGUE_PLATFORM_PATH_MAX] = "";

static void copy_root(char *target, size_t target_size, const char *value)
{
    if (value == NULL)
        value = "";
    snprintf(target, target_size, "%s", value);
    target[target_size - 1] = '\0';
}

void rogue_platform_configure_storage(const char *asset_root, const char *user_root)
{
    copy_root(platform_asset_root, sizeof(platform_asset_root), asset_root);
    copy_root(platform_user_root, sizeof(platform_user_root), user_root);
}

static const char *join_root(const char *root, const char *relative, char *out, size_t out_size)
{
    if (relative == NULL)
        relative = "";
    if (root != NULL && root[0] != '\0')
        snprintf(out, out_size, "%s/%s", root, relative);
    else
        snprintf(out, out_size, "%s", relative);
    out[out_size - 1] = '\0';
    return out;
}

const char *rogue_platform_asset_path(const char *relative, char *out, size_t out_size)
{
    return join_root(platform_asset_root, relative, out, out_size);
}

const char *rogue_platform_user_path(const char *relative, char *out, size_t out_size)
{
    return join_root(platform_user_root, relative, out, out_size);
}

char *rogue_platform_read_text_file(const char *relative_path)
{
    char path[ROGUE_PLATFORM_PATH_MAX];
    FILE *file;
    long size;
    char *text;

    rogue_platform_asset_path(relative_path, path, sizeof(path));
    file = fopen(path, "rb");
    if (file == NULL)
        return NULL;
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    if (size < 0 || size > ROGUE_PLATFORM_MAX_TEXT) {
        fclose(file);
        return NULL;
    }
    fseek(file, 0, SEEK_SET);
    text = (char *)calloc((size_t)size + 1, 1);
    if (text == NULL) {
        fclose(file);
        return NULL;
    }
    fread(text, 1, (size_t)size, file);
    fclose(file);
    return text;
}

#ifdef ROGUE_ANDROID
JNIEXPORT void JNICALL
Java_com_roguetiles_RogueTilesActivity_nativeConfigureStorage(
    JNIEnv *env, jclass clazz, jstring asset_root, jstring user_root)
{
    const char *asset_chars;
    const char *user_chars;
    (void)clazz;

    asset_chars = (*env)->GetStringUTFChars(env, asset_root, NULL);
    user_chars = (*env)->GetStringUTFChars(env, user_root, NULL);
    rogue_platform_configure_storage(asset_chars, user_chars);
    (*env)->ReleaseStringUTFChars(env, asset_root, asset_chars);
    (*env)->ReleaseStringUTFChars(env, user_root, user_chars);
}
#endif
```

- [ ] **Step 4: Use platform API from tilepack and Allegro frontend**

In `tilepack.c`:

- Add `#include "rogue_platform.h"`.
- Remove the local `read_text_file` function.
- Replace `read_text_file(tilepack_path)` with `rogue_platform_read_text_file(tilepack_path)`.
- Before `load_tilepack_file("tilepacks/default/tilepack.json", "default")`, keep the relative path string; `rogue_platform_read_text_file()` adds the Android asset root.

In `allegro_frontend.c`:

- Add `#include "rogue_platform.h"`.
- In `init_settings_path()`, use `rogue_platform_user_path("settings.json", settings_path, sizeof(settings_path))` before the existing `_WIN32` executable-path branch.
- In `load_current_atlas()`, replace the first `al_load_bitmap(atlas_path)` attempt with:

```c
char resolved_atlas_path[512];
rogue_platform_asset_path(atlas_path, resolved_atlas_path, sizeof(resolved_atlas_path));
loaded = al_load_bitmap(resolved_atlas_path);
```

- Keep the parent path fallback for Windows developer runs.

- [ ] **Step 5: Add platform object to Windows build**

In `Makefile.std`, add `rogue_platform.h` to `HDRS`, add `rogue_platform.$(O)` to `OBJS2`, and add `rogue_platform.c` to `CFILES`.

In `scripts/build-windows-native.ps1`, add `"rogue_platform.c"` and `"rogue_platform.h"` to the overlay file list.

- [ ] **Step 6: Run tests**

Run:

```powershell
python -m pytest tests/test_android_platform_paths.py -q
```

Expected: PASS.

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-windows-native.ps1 -Tiles
```

Expected: PASS and `native-build\rogue54.exe` exists.

- [ ] **Step 7: Commit platform helpers**

Run:

```powershell
git add rogue_platform.h rogue_platform.c tilepack.c allegro_frontend.c Makefile.std scripts/build-windows-native.ps1 tests/test_android_platform_paths.py
git commit -m "feat: add platform asset paths"
```

---

### Task 4: Add Pure C Mobile Control Layout

**Files:**
- Create: `mobile_controls.h`
- Create: `mobile_controls.c`
- Create: `tests/mobile_controls_test.c`
- Create: `scripts/test-mobile-controls.ps1`
- Modify: `Makefile.std`
- Modify: `scripts/build-windows-native.ps1`

- [ ] **Step 1: Add failing mobile control test**

Create `tests/mobile_controls_test.c`:

```c
#include <stdio.h>
#include "mobile_controls.h"

static int expect_command(const char *name, char got, char expected)
{
    if (got != expected) {
        fprintf(stderr, "%s: got %d expected %d\n", name, (int)got, (int)expected);
        return 1;
    }
    return 0;
}

int main(void)
{
    ROGUE_MOBILE_LAYOUT layout;
    int failures = 0;

    rogue_mobile_layout_build(&layout, 0, 0, 900, 360);

    failures += expect_command("northwest", rogue_mobile_command_at(&layout, 10, 10), 'y');
    failures += expect_command("north", rogue_mobile_command_at(&layout, 450, 10), 'k');
    failures += expect_command("northeast", rogue_mobile_command_at(&layout, 890, 10), 'u');
    failures += expect_command("west", rogue_mobile_command_at(&layout, 10, 120), 'h');
    failures += expect_command("center", rogue_mobile_command_at(&layout, 450, 120), '.');
    failures += expect_command("east", rogue_mobile_command_at(&layout, 890, 120), 'l');
    failures += expect_command("southwest", rogue_mobile_command_at(&layout, 10, 230), 'b');
    failures += expect_command("south", rogue_mobile_command_at(&layout, 450, 230), 'j');
    failures += expect_command("southeast", rogue_mobile_command_at(&layout, 890, 230), 'n');
    failures += expect_command("outside", rogue_mobile_command_at(&layout, -1, -1), '\0');

    if (failures != 0)
        return 1;
    printf("mobile controls tests passed\n");
    return 0;
}
```

Create `scripts/test-mobile-controls.ps1`:

```powershell
$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "native-build-tests"
$exe = Join-Path $buildDir "mobile_controls_test.exe"
$gcc = "C:\msys64\mingw64\bin\gcc.exe"

New-Item -ItemType Directory -Force $buildDir | Out-Null
& $gcc -std=gnu89 -I$repoRoot (Join-Path $repoRoot "tests\mobile_controls_test.c") (Join-Path $repoRoot "mobile_controls.c") -o $exe
if ($LASTEXITCODE -ne 0) {
    throw "mobile_controls_test compile failed"
}
& $exe
if ($LASTEXITCODE -ne 0) {
    throw "mobile_controls_test failed"
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\test-mobile-controls.ps1
```

Expected: FAIL because `mobile_controls.c` and `mobile_controls.h` do not exist.

- [ ] **Step 3: Implement mobile controls**

Create `mobile_controls.h`:

```c
#ifndef ROGUE_MOBILE_CONTROLS_H
#define ROGUE_MOBILE_CONTROLS_H

#define ROGUE_MOBILE_BUTTON_COUNT 9

typedef struct rogue_mobile_rect {
    int x;
    int y;
    int w;
    int h;
} ROGUE_MOBILE_RECT;

typedef struct rogue_mobile_button {
    ROGUE_MOBILE_RECT rect;
    char command;
    const char *label;
} ROGUE_MOBILE_BUTTON;

typedef struct rogue_mobile_layout {
    ROGUE_MOBILE_BUTTON buttons[ROGUE_MOBILE_BUTTON_COUNT];
    int button_count;
} ROGUE_MOBILE_LAYOUT;

void rogue_mobile_layout_build(ROGUE_MOBILE_LAYOUT *layout, int x, int y, int w, int h);
char rogue_mobile_command_at(const ROGUE_MOBILE_LAYOUT *layout, int x, int y);

#endif
```

Create `mobile_controls.c`:

```c
#include <string.h>
#include "mobile_controls.h"

static void set_button(ROGUE_MOBILE_BUTTON *button, int x, int y, int w, int h,
		       char command, const char *label)
{
    button->rect.x = x;
    button->rect.y = y;
    button->rect.w = w;
    button->rect.h = h;
    button->command = command;
    button->label = label;
}

void rogue_mobile_layout_build(ROGUE_MOBILE_LAYOUT *layout, int x, int y, int w, int h)
{
    int col_w;
    int row_h;
    int i;
    static const char commands[9] = { 'y', 'k', 'u', 'h', '.', 'l', 'b', 'j', 'n' };
    static const char *labels[9] = { "↖", "↑", "↗", "←", "USE", "→", "↙", "↓", "↘" };

    memset(layout, 0, sizeof(*layout));
    layout->button_count = ROGUE_MOBILE_BUTTON_COUNT;
    col_w = w / 3;
    row_h = h / 3;

    for (i = 0; i < 9; i++) {
        int col = i % 3;
        int row = i / 3;
        int bx = x + col * col_w;
        int by = y + row * row_h;
        int bw = (col == 2) ? (x + w - bx) : col_w;
        int bh = (row == 2) ? (y + h - by) : row_h;
        set_button(&layout->buttons[i], bx, by, bw, bh, commands[i], labels[i]);
    }
}

char rogue_mobile_command_at(const ROGUE_MOBILE_LAYOUT *layout, int x, int y)
{
    int i;
    for (i = 0; i < layout->button_count; i++) {
        const ROGUE_MOBILE_RECT *rect = &layout->buttons[i].rect;
        if (x >= rect->x && y >= rect->y
            && x < rect->x + rect->w && y < rect->y + rect->h)
            return layout->buttons[i].command;
    }
    return '\0';
}
```

- [ ] **Step 4: Add mobile controls to native build lists**

In `Makefile.std`, add `mobile_controls.h` to `HDRS`, add `mobile_controls.$(O)` to `OBJS2`, and add `mobile_controls.c` to `CFILES`.

In `scripts/build-windows-native.ps1`, add `"mobile_controls.c"` and `"mobile_controls.h"` to the overlay file list.

- [ ] **Step 5: Run tests and Windows build**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\test-mobile-controls.ps1
powershell -ExecutionPolicy Bypass -File scripts\build-windows-native.ps1 -Tiles
```

Expected: both PASS.

- [ ] **Step 6: Commit controls module**

Run:

```powershell
git add mobile_controls.h mobile_controls.c tests/mobile_controls_test.c scripts/test-mobile-controls.ps1 Makefile.std scripts/build-windows-native.ps1
git commit -m "feat: add mobile touch control mapping"
```

---

### Task 5: Add Android-Only Startup And Frontend Guards

**Files:**
- Modify: `main.c`
- Modify: `frontend.c`
- Modify: `allegro_frontend.c`
- Create: `tests/test_android_startup_contract.py`

- [ ] **Step 1: Write startup contract tests**

Create `tests/test_android_startup_contract.py`:

```python
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def test_android_default_only_bypasses_variant_entrypoints():
    main_c = (ROOT / "main.c").read_text(encoding="utf-8")
    assert "ROGUE_ANDROID_DEFAULT_ONLY" in main_c
    assert "#ifndef ROGUE_ANDROID_DEFAULT_ONLY" in main_c


def test_android_forces_tiles_mode():
    frontend_c = (ROOT / "frontend.c").read_text(encoding="utf-8")
    assert "ROGUE_ANDROID" in frontend_c
    assert "tiles_requested = TRUE" in frontend_c


def test_android_hides_desktop_tilepack_menu_hotkey():
    allegro_c = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert "ROGUE_ANDROID" in allegro_c
    assert "show_tilepack_menu();" in allegro_c
    assert "#ifndef ROGUE_ANDROID" in allegro_c
```

- [ ] **Step 2: Run tests to verify they fail**

Run:

```powershell
python -m pytest tests/test_android_startup_contract.py -q
```

Expected: FAIL because the Android guards do not exist yet.

- [ ] **Step 3: Force tile mode on Android**

In `frontend.c`, inside `rogue_frontend_init()` after local variable declarations, add:

```c
#ifdef ROGUE_ANDROID
    tiles_requested = TRUE;
#endif
```

- [ ] **Step 4: Compile default ruleset only on Android**

In `main.c`, wrap the variant entrypoint dispatch block:

```c
#ifndef ROGUE_ANDROID_DEFAULT_ONLY
    if (rogue_variant_is_current("rogue52"))
    {
        return rogue52_main(argc, argv, envp);
    }
    if (rogue_variant_is_current("rogue36"))
    {
        prepare_rogue36_shared_state();
        return rogue36_main(argc, argv, envp);
    }
    if (rogue_variant_is_current("srogue90"))
    {
        return srogue90_main(argc, argv, envp);
    }
#endif
```

In `allegro_frontend.c`, guard the `F10` tilepack menu block:

```c
#ifndef ROGUE_ANDROID
        if (event.keyboard.keycode == ALLEGRO_KEY_F10)
        {
            show_tilepack_menu();
            suppress_key_char_keycode = event.keyboard.keycode;
            rogue_allegro_render();
            continue;
        }
#endif
```

- [ ] **Step 5: Run tests and Windows build**

Run:

```powershell
python -m pytest tests/test_android_startup_contract.py -q
powershell -ExecutionPolicy Bypass -File scripts\build-windows-native.ps1 -Tiles
```

Expected: tests PASS and Windows tile build PASS.

- [ ] **Step 6: Commit Android startup guards**

Run:

```powershell
git add main.c frontend.c allegro_frontend.c tests/test_android_startup_contract.py
git commit -m "feat: add android startup guards"
```

---

### Task 6: Wire Touch Events And Draw Mobile Controls

**Files:**
- Modify: `allegro_frontend.c`
- Create: `tests/test_android_touch_frontend.py`

- [ ] **Step 1: Write frontend touch tests**

Create `tests/test_android_touch_frontend.py`:

```python
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def test_allegro_frontend_installs_touch_input():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert "al_install_touch_input()" in text
    assert "al_get_touch_input_event_source()" in text
    assert "ALLEGRO_EVENT_TOUCH_BEGIN" in text


def test_allegro_frontend_uses_mobile_controls_module():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert '#include "mobile_controls.h"' in text
    assert "ROGUE_MOBILE_LAYOUT" in text
    assert "rogue_mobile_layout_build" in text
    assert "rogue_mobile_command_at" in text
    assert "pending_touch_command" in text


def test_mobile_attack_is_visible_but_disabled():
    text = (ROOT / "allegro_frontend.c").read_text(encoding="utf-8")
    assert '"ATK"' in text
    assert '"WAIT"' in text
    assert '"LOOK"' in text
    assert '"DOWN"' in text
    assert "mobile_attack_disabled" in text
```

- [ ] **Step 2: Run tests to verify they fail**

Run:

```powershell
python -m pytest tests/test_android_touch_frontend.py -q
```

Expected: FAIL because touch input is not wired.

- [ ] **Step 3: Add mobile state and touch registration**

In `allegro_frontend.c`:

- Include `mobile_controls.h`.
- Add static state near existing input state:

```c
static ROGUE_MOBILE_LAYOUT mobile_layout;
static char pending_touch_command = '\0';
static bool mobile_attack_disabled = TRUE;
```

- In `rogue_allegro_start()`, after keyboard install, add:

```c
#ifdef ROGUE_ANDROID
    if (!al_install_touch_input())
    {
        allegro_start_error("Allegro touch initialization failed.");
        return FALSE;
    }
#endif
```

- After registering the keyboard event source, add:

```c
#ifdef ROGUE_ANDROID
    al_register_event_source(queue, al_get_touch_input_event_source());
#endif
```

- [ ] **Step 4: Draw mobile control panel**

Add helper functions near the existing drawing helpers:

```c
#ifdef ROGUE_ANDROID
static int mobile_controls_height(void)
{
    return display_height() / 3;
}

static void build_mobile_layout(void)
{
    int h = mobile_controls_height();
    rogue_mobile_layout_build(&mobile_layout, 16, display_height() - h + 8,
                              display_width() - 32, h - 16);
}

static void draw_mobile_controls(void)
{
    int i;
    ALLEGRO_COLOR border = al_map_rgb(64, 88, 154);
    ALLEGRO_COLOR fill = al_map_rgb(7, 11, 28);
    ALLEGRO_COLOR text = al_map_rgb(210, 226, 255);
    build_mobile_layout();
    for (i = 0; i < mobile_layout.button_count; i++) {
        ROGUE_MOBILE_BUTTON *button = &mobile_layout.buttons[i];
        al_draw_filled_rectangle(button->rect.x, button->rect.y,
                                 button->rect.x + button->rect.w,
                                 button->rect.y + button->rect.h,
                                 fill);
        al_draw_rectangle(button->rect.x, button->rect.y,
                          button->rect.x + button->rect.w,
                          button->rect.y + button->rect.h,
                          border, 1);
        al_draw_text(font, text,
                     button->rect.x + button->rect.w / 2,
                     button->rect.y + button->rect.h / 2 - 16,
                     ALLEGRO_ALIGN_CENTRE,
                     button->label);
    }
    al_draw_text(small_font, al_map_rgb(228, 154, 83), 32,
                 display_height() - 30, 0, "ATK");
    al_draw_text(small_font, text, display_width() / 3,
                 display_height() - 30, ALLEGRO_ALIGN_CENTRE, "WAIT");
    al_draw_text(small_font, text, display_width() * 2 / 3,
                 display_height() - 30, ALLEGRO_ALIGN_CENTRE, "LOOK");
    al_draw_text(small_font, text, display_width() - 32,
                 display_height() - 30, ALLEGRO_ALIGN_RIGHT, "DOWN");
    (void)mobile_attack_disabled;
}
#endif
```

In `rogue_allegro_render()`, before `al_flip_display()`, call:

```c
#ifdef ROGUE_ANDROID
    draw_mobile_controls();
#endif
```

- [ ] **Step 5: Feed touch commands into readchar**

In `rogue_allegro_readchar()`, near the top of the loop after rendering timeout handling, add:

```c
        if (pending_touch_command != '\0')
        {
            mapped = pending_touch_command;
            pending_touch_command = '\0';
            return mapped;
        }
```

In the event loop before keyboard handling, add:

```c
#ifdef ROGUE_ANDROID
        if (event.type == ALLEGRO_EVENT_TOUCH_BEGIN)
        {
            build_mobile_layout();
            pending_touch_command = rogue_mobile_command_at(
                &mobile_layout, (int)event.touch.x, (int)event.touch.y);
            if (pending_touch_command != '\0')
                return pending_touch_command;
            continue;
        }
#endif
```

- [ ] **Step 6: Run tests and Windows build**

Run:

```powershell
python -m pytest tests/test_android_touch_frontend.py -q
powershell -ExecutionPolicy Bypass -File scripts\test-mobile-controls.ps1
powershell -ExecutionPolicy Bypass -File scripts\build-windows-native.ps1 -Tiles
```

Expected: tests PASS and Windows tile build PASS.

- [ ] **Step 7: Commit touch frontend**

Run:

```powershell
git add allegro_frontend.c tests/test_android_touch_frontend.py
git commit -m "feat: wire android touch controls"
```

---

### Task 7: Add Android Native CMake Build

**Files:**
- Modify: `android/app/src/main/cpp/CMakeLists.txt`
- Modify: `tests/test_android_project_contract.py`

- [ ] **Step 1: Expand contract test for default-only native source list**

Append to `tests/test_android_project_contract.py`:

```python
def test_android_cmake_uses_default_ruleset_only():
    cmake = read("android/app/src/main/cpp/CMakeLists.txt")
    base_sources = [
        "vers.c", "extern.c", "armor.c", "chase.c", "command.c",
        "daemon.c", "daemons.c", "fight.c", "init.c", "io.c",
        "list.c", "mach_dep.c", "main.c", "mdport.c", "misc.c",
        "monsters.c", "move.c", "new_level.c", "options.c", "pack.c",
        "passages.c", "potions.c", "rings.c", "rip.c", "rooms.c",
        "save.c", "scrolls.c", "state.c", "sticks.c", "things.c",
        "tiles.c", "tilepack.c", "frontend.c", "overlay_picker.c",
        "variant.c", "rogue_platform.c", "mobile_controls.c",
        "generated/rogue_tile_mapping.c", "allegro_frontend.c",
        "weapons.c", "wizard.c", "xcrypt.c",
    ]
    for source in base_sources:
        assert source in cmake
    assert "variants/rogue52" not in cmake
    assert "variants/rogue36" not in cmake
    assert "variants/srogue90" not in cmake
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
python -m pytest tests/test_android_project_contract.py::test_android_cmake_uses_default_ruleset_only -q
```

Expected: FAIL because CMake still has the intentional fatal message.

- [ ] **Step 3: Implement CMake native target**

Replace `android/app/src/main/cpp/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.22.1)
project(roguetiles_android C)

set(REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../..)

add_library(roguetiles SHARED
    ${REPO_ROOT}/vers.c
    ${REPO_ROOT}/extern.c
    ${REPO_ROOT}/armor.c
    ${REPO_ROOT}/chase.c
    ${REPO_ROOT}/command.c
    ${REPO_ROOT}/daemon.c
    ${REPO_ROOT}/daemons.c
    ${REPO_ROOT}/fight.c
    ${REPO_ROOT}/init.c
    ${REPO_ROOT}/io.c
    ${REPO_ROOT}/list.c
    ${REPO_ROOT}/mach_dep.c
    ${REPO_ROOT}/main.c
    ${REPO_ROOT}/mdport.c
    ${REPO_ROOT}/misc.c
    ${REPO_ROOT}/monsters.c
    ${REPO_ROOT}/move.c
    ${REPO_ROOT}/new_level.c
    ${REPO_ROOT}/options.c
    ${REPO_ROOT}/pack.c
    ${REPO_ROOT}/passages.c
    ${REPO_ROOT}/potions.c
    ${REPO_ROOT}/rings.c
    ${REPO_ROOT}/rip.c
    ${REPO_ROOT}/rooms.c
    ${REPO_ROOT}/save.c
    ${REPO_ROOT}/scrolls.c
    ${REPO_ROOT}/state.c
    ${REPO_ROOT}/sticks.c
    ${REPO_ROOT}/things.c
    ${REPO_ROOT}/tiles.c
    ${REPO_ROOT}/tilepack.c
    ${REPO_ROOT}/frontend.c
    ${REPO_ROOT}/overlay_picker.c
    ${REPO_ROOT}/variant.c
    ${REPO_ROOT}/rogue_platform.c
    ${REPO_ROOT}/mobile_controls.c
    ${REPO_ROOT}/generated/rogue_tile_mapping.c
    ${REPO_ROOT}/allegro_frontend.c
    ${REPO_ROOT}/weapons.c
    ${REPO_ROOT}/wizard.c
    ${REPO_ROOT}/xcrypt.c
)

target_include_directories(roguetiles PRIVATE
    ${REPO_ROOT}
    ${REPO_ROOT}/generated
    ${CMAKE_CURRENT_LIST_DIR}/../../../../android/vendor/allegro/include
)

target_compile_definitions(roguetiles PRIVATE
    ROGUE_ANDROID
    ROGUE_ANDROID_DEFAULT_ONLY
    ROGUE_ENABLE_ALLEGRO
    ALLSCORES
    SCOREFILE="rogue54.scr"
    LOCKFILE="rogue54.lck"
)

target_compile_options(roguetiles PRIVATE -std=gnu89 -fcommon)

find_library(log-lib log)
find_library(android-lib android)
find_library(gles-lib GLESv2)

add_library(allegro SHARED IMPORTED)
set_target_properties(allegro PROPERTIES IMPORTED_LOCATION
    ${REPO_ROOT}/android/vendor/allegro/jni/${ANDROID_ABI}/liballegro.so)
add_library(allegro_image SHARED IMPORTED)
set_target_properties(allegro_image PROPERTIES IMPORTED_LOCATION
    ${REPO_ROOT}/android/vendor/allegro/jni/${ANDROID_ABI}/liballegro_image.so)
add_library(allegro_font SHARED IMPORTED)
set_target_properties(allegro_font PROPERTIES IMPORTED_LOCATION
    ${REPO_ROOT}/android/vendor/allegro/jni/${ANDROID_ABI}/liballegro_font.so)
add_library(allegro_ttf SHARED IMPORTED)
set_target_properties(allegro_ttf PROPERTIES IMPORTED_LOCATION
    ${REPO_ROOT}/android/vendor/allegro/jni/${ANDROID_ABI}/liballegro_ttf.so)
add_library(allegro_primitives SHARED IMPORTED)
set_target_properties(allegro_primitives PROPERTIES IMPORTED_LOCATION
    ${REPO_ROOT}/android/vendor/allegro/jni/${ANDROID_ABI}/liballegro_primitives.so)

target_link_libraries(roguetiles
    allegro_image
    allegro_ttf
    allegro_font
    allegro_primitives
    allegro
    ${log-lib}
    ${android-lib}
    ${gles-lib}
)
```

- [ ] **Step 4: Run contract tests**

Run:

```powershell
python -m pytest tests/test_android_project_contract.py -q
```

Expected: PASS.

- [ ] **Step 5: Commit Android CMake**

Run:

```powershell
git add android/app/src/main/cpp/CMakeLists.txt tests/test_android_project_contract.py
git commit -m "build: add android native target"
```

---

### Task 8: Build APK And Verify Windows Still Works

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Sync Android assets**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\sync-android-assets.ps1
```

Expected: `android/app/src/main/assets/assets/rltiles/rltiles-2d.png` and `android/app/src/main/assets/tilepacks/default/tilepack.json` exist.

- [ ] **Step 2: Run full local verification**

Run:

```powershell
python -m pytest tests/test_android_project_contract.py tests/test_android_platform_paths.py tests/test_android_startup_contract.py tests/test_android_touch_frontend.py -q
powershell -ExecutionPolicy Bypass -File scripts\test-mobile-controls.ps1
powershell -ExecutionPolicy Bypass -File scripts\validate-rltiles.ps1
powershell -ExecutionPolicy Bypass -File scripts\build-windows-native.ps1 -Tiles
```

Expected:

- Python tests PASS.
- Mobile controls test prints `mobile controls tests passed`.
- RL Tiles validation PASS.
- Windows tile build produces `native-build\rogue54.exe`.

- [ ] **Step 3: Build Android debug APK**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-android-debug.ps1
```

Expected if Allegro Android artifacts are present: `android/app/build/outputs/apk/debug/app-debug.apk`.

Expected if Allegro Android artifacts are missing: script fails before Gradle with a missing-artifacts list from `scripts/check-android-allegro.ps1`.

- [ ] **Step 4: Install and smoke test on device when available**

Run:

```powershell
adb install -r android\app\build\outputs\apk\debug\app-debug.apk
adb shell monkey -p com.roguetiles 1
adb shell dumpsys window | Select-String -Pattern "mCurrentFocus|mFocusedApp"
```

Expected:

- Install succeeds.
- App launches.
- Focus contains `com.roguetiles`.
- Manual device check confirms dungeon tiles render and the 8 direction buttons move the player.

- [ ] **Step 5: Document Android developer build**

Add this section to `README.md` after the Windows build section:

```markdown
## Building Locally For Android

The Android build is a graphics-only developer APK. It lives beside the Windows build and does not include `TilePicker.exe` or the desktop tile editor.

Before building, place the Allegro Android artifacts listed in `android/vendor/allegro/README.md`.

Build the debug APK:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-android-debug.ps1
```

The debug APK is written to:

```text
android\app\build\outputs\apk\debug\app-debug.apk
```

The first Android slice uses the default RogueTiles ruleset, bundled default tiles, and touch controls for movement, wait, look, and descend.
```

- [ ] **Step 6: Commit verification docs and synced assets**

Run:

```powershell
git add README.md android/app/src/main/assets
git commit -m "docs: add android debug build instructions"
```

---

## Self-Review Notes

- Spec coverage: The plan covers Android alongside Windows, graphics-only startup, no tile editor packaging, bundled assets/default tilepack, screenshot-style touch controls, touch-to-command mapping, storage paths, startup failure checks, Windows build verification, and debug APK build.
- Scope decision: The first Android APK compiles the default ruleset only through `ROGUE_ANDROID_DEFAULT_ONLY`. Multi-variant Android packaging needs a separate plan because the Windows build relies on objcopy symbol rewriting for embedded variants.
- Completion scan: No task contains open-ended implementation gaps. Missing Allegro Android artifacts are handled by an explicit dependency check with expected failure text.
- Type consistency: The plan consistently uses `ROGUE_MOBILE_LAYOUT`, `rogue_mobile_layout_build`, `rogue_mobile_command_at`, `rogue_platform_asset_path`, `rogue_platform_user_path`, and `rogue_platform_read_text_file`.
