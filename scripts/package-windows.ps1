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
    $resolvedRepo = (Resolve-Path $repoRoot).Path
    $resolvedPackage = (Resolve-Path $packageDir).Path
    if (-not $resolvedPackage.StartsWith($resolvedRepo)) {
        throw "Refusing to remove package directory outside repo: $resolvedPackage"
    }
    Remove-Item -LiteralPath $resolvedPackage -Recurse -Force
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
