param(
    [string]$MsysRoot = "C:\msys64",
    [string]$Python = "python"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

Push-Location $repoRoot
try {
    & $Python -m tile_picker.generate_tile_mapping --root $repoRoot
    if ($LASTEXITCODE -ne 0) {
        throw "tile mapping generation failed"
    }

    powershell -ExecutionPolicy Bypass -File scripts\build-windows-native.ps1 -MsysRoot $MsysRoot -Tiles
    if ($LASTEXITCODE -ne 0) {
        throw "native tile build failed"
    }

    Start-Process -FilePath (Join-Path $repoRoot "native-build\rogue54.exe") `
        -ArgumentList @("--tiles") `
        -WorkingDirectory (Join-Path $repoRoot "native-build") `
        -WindowStyle Hidden
}
finally {
    Pop-Location
}
