param(
    [string]$Python = "python"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

Push-Location $repoRoot
try {
    & $Python -c "from pathlib import Path; from tile_picker import tilepack_writer; root=Path('.').resolve(); tilepack_writer.write_default_pack(root, 'default'); tilepack_writer.write_default_pack(root, 'active'); print('Wrote tilepacks/default and tilepacks/active')"
    if ($LASTEXITCODE -ne 0) {
        throw "runtime tilepack generation failed"
    }
}
finally {
    Pop-Location
}
