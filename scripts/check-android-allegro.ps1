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
