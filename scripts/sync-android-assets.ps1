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
