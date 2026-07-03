param(
    [string]$Python = "python",
    [int]$Port = 8788,
    [switch]$NoOpen
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$openArg = @()
if (-not $NoOpen) {
    $openArg = @("--open")
}

Push-Location $repoRoot
try {
    & $Python -m tile_picker.serve_picker --root $repoRoot --port $Port @openArg
}
finally {
    Pop-Location
}
