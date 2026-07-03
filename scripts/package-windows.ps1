param(
    [string]$MsysRoot = "C:\msys64",
    [switch]$SkipBuild,
    [switch]$Zip,
    [string]$ZipPath
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$distRoot = Join-Path $repoRoot "dist"
$packageDir = Join-Path $distRoot "RogueTiles"
$nativeDir = Join-Path $repoRoot "native-build"

if ([string]::IsNullOrWhiteSpace($ZipPath)) {
    $ZipPath = Join-Path $distRoot "RogueTiles-windows-x64.zip"
}

function Invoke-RepoScript {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ScriptPath,
        [string[]]$Arguments = @()
    )

    & powershell -ExecutionPolicy Bypass -File $ScriptPath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Script failed with exit code ${LASTEXITCODE}: $ScriptPath $($Arguments -join ' ')"
    }
}

if (-not $SkipBuild) {
    Invoke-RepoScript -ScriptPath (Join-Path $repoRoot "scripts\build-runtime-tilepacks.ps1")
    Invoke-RepoScript -ScriptPath (Join-Path $repoRoot "scripts\build-windows-native.ps1") -Arguments @("-MsysRoot", $MsysRoot, "-Tiles")
}

New-Item -ItemType Directory -Force $packageDir | Out-Null

if (Test-Path $packageDir) {
    $resolvedRepo = (Resolve-Path $repoRoot).Path
    $resolvedPackage = (Resolve-Path $packageDir).Path
    if (-not $resolvedPackage.StartsWith($resolvedRepo)) {
        throw "Refusing to update package directory outside repo: $resolvedPackage"
    }

    foreach ($child in Get-ChildItem -LiteralPath $packageDir -Force) {
        Remove-Item -LiteralPath $child.FullName -Recurse -Force
    }
}

Copy-Item -LiteralPath (Join-Path $nativeDir "rogue54.exe") -Destination (Join-Path $packageDir "RogueTiles.exe") -Force
Copy-Item -LiteralPath (Join-Path $repoRoot "LICENSE.TXT") -Destination $packageDir -Force
Copy-Item -LiteralPath (Join-Path $repoRoot "README.md") -Destination $packageDir -Force

foreach ($dll in Get-ChildItem -LiteralPath $nativeDir -Filter "*.dll") {
    Copy-Item -LiteralPath $dll.FullName -Destination (Join-Path $packageDir $dll.Name) -Force
}

Copy-Item -LiteralPath (Join-Path $repoRoot "assets") -Destination (Join-Path $packageDir "assets") -Recurse -Force
Copy-Item -LiteralPath (Join-Path $repoRoot "tilepacks") -Destination (Join-Path $packageDir "tilepacks") -Recurse -Force
Copy-Item -LiteralPath (Join-Path $repoRoot "tile_picker") -Destination (Join-Path $packageDir "tile_picker") -Recurse -Force

$tilePickerExe = Join-Path $repoRoot "dist\picker-build\TilePicker.exe"
if (Test-Path $tilePickerExe) {
    Copy-Item -LiteralPath $tilePickerExe -Destination (Join-Path $packageDir "TilePicker.exe") -Force
}
else {
    Write-Warning "TilePicker.exe was not found. Run scripts\build-tile-picker-exe.ps1 before final packaging."
}

Write-Host "Packaged: $packageDir"

if ($Zip) {
    $resolvedDist = (Resolve-Path $distRoot).Path
    $zipParent = Split-Path -Parent $ZipPath
    if (-not (Test-Path $zipParent)) {
        New-Item -ItemType Directory -Force $zipParent | Out-Null
    }
    $resolvedZipParent = (Resolve-Path $zipParent).Path
    if (-not $resolvedZipParent.StartsWith($resolvedDist)) {
        throw "Refusing to write release zip outside dist: $ZipPath"
    }
    if (Test-Path $ZipPath) {
        Remove-Item -LiteralPath $ZipPath -Force
    }
    Compress-Archive -LiteralPath $packageDir -DestinationPath $ZipPath -Force
    Write-Host "Release zip: $ZipPath"
}
