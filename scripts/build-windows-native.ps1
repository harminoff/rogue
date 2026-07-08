param(
    [string]$MsysRoot = "C:\msys64",
    [switch]$Tiles
)

$ErrorActionPreference = "Stop"

function Invoke-Native {
    param(
        [Parameter(Mandatory = $true)]
        [scriptblock]$Command
    )

    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $Command"
    }
}

function Copy-OverlayDirectory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Source,
        [Parameter(Mandatory = $true)]
        [string]$Destination
    )

    if (-not (Test-Path $Source)) {
        return
    }

    New-Item -ItemType Directory -Force $Destination | Out-Null
    Get-ChildItem -LiteralPath $Source -Force | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $Destination -Recurse -Force
    }
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "native-build"
$archive = Join-Path $buildDir "rogue-src.tar"
$msysUsrBin = Join-Path $MsysRoot "usr\bin"
$mingwBin = Join-Path $MsysRoot "mingw64\bin"
$mingwLib = Join-Path $MsysRoot "mingw64\lib"
$ncursesHeaderCandidates = @(
    (Join-Path $MsysRoot "mingw64\include\ncursesw"),
    (Join-Path $MsysRoot "mingw64\include\ncurses")
)
$ncursesHeaders = $ncursesHeaderCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
$ncursesImportCandidates = @(
    (Join-Path $mingwLib "libncursesw.dll.a"),
    (Join-Path $mingwLib "libncurses.dll.a"),
    (Join-Path $mingwLib "libncursesw.a"),
    (Join-Path $mingwLib "libncurses.a")
)
$ncursesImport = $ncursesImportCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
$ncursesDllCandidates = @(
    (Join-Path $mingwBin "libncursesw6.dll"),
    (Join-Path $mingwBin "libncurses6.dll"),
    (Join-Path $mingwBin "libncursesw.dll"),
    (Join-Path $mingwBin "libncurses.dll")
)
$ncursesDll = $ncursesDllCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
$winpthreadDllCandidates = @(
    (Join-Path $mingwBin "libwinpthread-1.dll"),
    (Join-Path $MsysRoot "ucrt64\bin\libwinpthread-1.dll"),
    (Join-Path $MsysRoot "clang64\bin\libwinpthread-1.dll")
)
$winpthreadDll = $winpthreadDllCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
$pkgConfig = Join-Path $mingwBin "pkg-config.exe"
$objdump = Join-Path $mingwBin "objdump.exe"
$mingwGcc = Join-Path $mingwBin "gcc.exe"

foreach ($path in @(
    (Join-Path $msysUsrBin "tar.exe"),
    (Join-Path $msysUsrBin "sed.exe")
)) {
    if (-not (Test-Path $path)) {
        throw "Missing dependency: $path. Install with: C:\msys64\usr\bin\pacman.exe -S mingw-w64-x86_64-ncurses"
    }
}

if ([string]::IsNullOrWhiteSpace($ncursesHeaders)) {
    throw "Missing dependency: ncurses headers. Install with: C:\msys64\usr\bin\pacman.exe -S mingw-w64-x86_64-ncurses"
}

if ([string]::IsNullOrWhiteSpace($ncursesImport)) {
    throw "Missing dependency: ncurses import library. Install with: C:\msys64\usr\bin\pacman.exe -S mingw-w64-x86_64-ncurses"
}

if ($Tiles) {
    foreach ($path in @($pkgConfig, $objdump)) {
        if (-not (Test-Path $path)) {
            throw "Missing dependency: $path"
        }
    }

    Invoke-Native { & $pkgConfig --exists allegro-5 allegro_image-5 allegro_font-5 allegro_ttf-5 allegro_primitives-5 }
}

if (-not (Get-Command mingw32-make -ErrorAction SilentlyContinue)) {
    throw "mingw32-make was not found on PATH."
}

if (-not (Test-Path $mingwGcc)) {
    throw "Missing dependency: $mingwGcc"
}

if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
    throw "python was not found on PATH. It is required to generate tile mappings."
}

$resolvedRoot = (Resolve-Path $repoRoot).Path
if (Test-Path $buildDir) {
    $resolvedBuild = (Resolve-Path $buildDir).Path
    if (-not $resolvedBuild.StartsWith($resolvedRoot)) {
        throw "Refusing to remove build directory outside repo: $resolvedBuild"
    }
    Remove-Item -LiteralPath $resolvedBuild -Recurse -Force
}

New-Item -ItemType Directory -Force $buildDir | Out-Null

Push-Location $repoRoot
try {
    Invoke-Native { python -m tile_picker.generate_tile_mapping --root $repoRoot }
    Invoke-Native { git archive --format=tar -o "native-build\rogue-src.tar" HEAD }
    Invoke-Native { & (Join-Path $msysUsrBin "tar.exe") -xf "native-build/rogue-src.tar" -C "native-build" }

    # Overlay working-tree build inputs so local compatibility and tile
    # presentation patches are included before they are committed.
    foreach ($path in @(
        "main.c",
        "command.c",
        "extern.h",
        "fight.c",
        "io.c",
        "mach_dep.c",
        "options.c",
        "pack.c",
        "rip.c",
        "save.c",
        "things.c",
        "Makefile.std",
        "tiles.c",
        "tiles.h",
        "tilepack.c",
        "tilepack.h",
        "rogue_platform.c",
        "rogue_platform.h",
        "frontend.c",
        "frontend.h",
        "overlay_picker.c",
        "overlay_picker.h",
        "mobile_controls.c",
        "mobile_controls.h",
        "variant.c",
        "variant.h",
        "allegro_frontend.c"
    )) {
        $source = Join-Path $repoRoot $path
        if (Test-Path $source) {
            Copy-Item -LiteralPath $source -Destination (Join-Path $buildDir $path) -Force
        }
    }

    $assetSource = Join-Path $repoRoot "assets"
    Copy-OverlayDirectory -Source $assetSource -Destination (Join-Path $buildDir "assets")

    $generatedSource = Join-Path $repoRoot "generated"
    Copy-OverlayDirectory -Source $generatedSource -Destination (Join-Path $buildDir "generated")

    $tilepackSource = Join-Path $repoRoot "tilepacks"
    Copy-OverlayDirectory -Source $tilepackSource -Destination (Join-Path $buildDir "tilepacks")

    $variantSource = Join-Path $repoRoot "variants"
    Copy-OverlayDirectory -Source $variantSource -Destination (Join-Path $buildDir "variants")
}
finally {
    Pop-Location
}

$includeDir = Join-Path $buildDir "include"
New-Item -ItemType Directory -Force $includeDir | Out-Null

@"
#ifndef ROGUE_NATIVE_BUILD_CURSES_H
#define ROGUE_NATIVE_BUILD_CURSES_H

#include "$(($ncursesHeaders -replace '\\', '/'))/curses.h"

#endif
"@ | Set-Content -LiteralPath (Join-Path $includeDir "curses.h") -NoNewline

@"
#ifndef ROGUE_NATIVE_BUILD_TERM_H
#define ROGUE_NATIVE_BUILD_TERM_H

#include "$(($ncursesHeaders -replace '\\', '/'))/term.h"

#endif
"@ | Set-Content -LiteralPath (Join-Path $includeDir "term.h") -NoNewline

Copy-Item -LiteralPath $ncursesHeaders -Destination (Join-Path $includeDir "ncursesw") -Recurse -Force

$env:PATH = "$mingwBin;$msysUsrBin;$env:PATH"
$cppflags = "-Iinclude"
$libs = "$ncursesImport -lws2_32"
$allegroObjs = ""

if ($Tiles) {
    $allegroCflags = (& $pkgConfig --cflags allegro-5 allegro_image-5 allegro_font-5 allegro_ttf-5 allegro_primitives-5) -join " "
    $allegroLibs = (& $pkgConfig --libs allegro-5 allegro_image-5 allegro_font-5 allegro_ttf-5 allegro_primitives-5) -join " "
    $cppflags = "$cppflags -DROGUE_ENABLE_ALLEGRO $allegroCflags"
    $libs = "$libs $allegroLibs -luser32"
    $allegroObjs = "allegro_frontend.o"
}

function Copy-DependentDlls {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Binary
    )

    $seen = @{}
    $queue = New-Object System.Collections.Queue
    $queue.Enqueue($Binary)

    while ($queue.Count -gt 0) {
        $current = [string]$queue.Dequeue()
        if (-not (Test-Path $current)) {
            continue
        }

        $dllNames = & $objdump -p $current |
            Select-String -Pattern "DLL Name: (.+)$" |
            ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() }

        foreach ($dll in $dllNames) {
            $key = $dll.ToLowerInvariant()
            if ($seen.ContainsKey($key)) {
                continue
            }
            $seen[$key] = $true

            $source = Join-Path $mingwBin $dll
            if (Test-Path $source) {
                $dest = Join-Path $buildDir $dll
                Copy-Item -LiteralPath $source -Destination $dest -Force
                $queue.Enqueue($source)
            }
        }
    }
}

Push-Location $buildDir
try {
    Invoke-Native { & mingw32-make -f Makefile.std `
        CC="$mingwGcc" `
        CFLAGS="-std=gnu89 -O2" `
        CPPFLAGS="$cppflags" `
        LDFLAGS="" `
        LIBS="$libs" `
        ALLEGRO_OBJS="$allegroObjs" }

    if (-not [string]::IsNullOrWhiteSpace($ncursesDll)) {
        Copy-Item -LiteralPath $ncursesDll -Destination (Join-Path $buildDir (Split-Path -Leaf $ncursesDll)) -Force
    }
    if (-not [string]::IsNullOrWhiteSpace($winpthreadDll)) {
        Copy-Item -LiteralPath $winpthreadDll -Destination (Join-Path $buildDir "libwinpthread-1.dll") -Force
    }
    if ($Tiles) {
        Copy-DependentDlls -Binary (Join-Path $buildDir "rogue54.exe")
    }
}
finally {
    Pop-Location
}

Write-Host "Built: $(Join-Path $buildDir 'rogue54.exe')"
if (-not [string]::IsNullOrWhiteSpace($ncursesDll)) {
    Write-Host "Runtime DLL: $(Join-Path $buildDir (Split-Path -Leaf $ncursesDll))"
}
if (-not [string]::IsNullOrWhiteSpace($winpthreadDll)) {
    Write-Host "Runtime DLL: $(Join-Path $buildDir 'libwinpthread-1.dll')"
}
