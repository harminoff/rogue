param(
    [string]$Python = "python"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$distDir = Join-Path $repoRoot "dist\picker-build"

Push-Location $repoRoot
try {
    & $Python -m PyInstaller --version | Out-Host
    if ($LASTEXITCODE -ne 0) {
        throw "PyInstaller is not available. Install for developer packaging with: python -m pip install pyinstaller"
    }

    & $Python -m PyInstaller `
        --noconfirm `
        --onefile `
        --name TilePicker `
        --distpath $distDir `
        --workpath (Join-Path $repoRoot "build\pyinstaller") `
        --specpath (Join-Path $repoRoot "build\pyinstaller") `
        tile_picker_launcher.py
    if ($LASTEXITCODE -ne 0) {
        throw "PyInstaller failed"
    }
}
finally {
    Pop-Location
}
