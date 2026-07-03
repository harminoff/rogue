param(
    [string]$Python = "python"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

Push-Location $repoRoot
try {
    & $Python -m tile_picker.build_picker --root $repoRoot
    if ($LASTEXITCODE -ne 0) {
        throw "tile picker data generation failed"
    }
}
finally {
    Pop-Location
}
