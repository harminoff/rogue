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
Copy-Item -LiteralPath (Join-Path $repoRoot "rogue54.6") -Destination (Join-Path $assetRoot "rogue54.6") -Force
Copy-Item -LiteralPath (Join-Path $repoRoot "rogue54.doc") -Destination (Join-Path $assetRoot "rogue54.doc") -Force
New-Item -ItemType Directory -Force (Join-Path $assetRoot "variants") | Out-Null
New-Item -ItemType Directory -Force (Join-Path $assetRoot "variants\rogue52") | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot "variants\rogue52\rogue.6") -Destination (Join-Path $assetRoot "variants\rogue52\rogue.6") -Force
New-Item -ItemType Directory -Force (Join-Path $assetRoot "variants\rogue36") | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot "variants\rogue36\rogue.6") -Destination (Join-Path $assetRoot "variants\rogue36\rogue.6") -Force
Copy-Item -LiteralPath (Join-Path $repoRoot "variants\rogue36\rogue.r") -Destination (Join-Path $assetRoot "variants\rogue36\rogue.r") -Force
New-Item -ItemType Directory -Force (Join-Path $assetRoot "variants\srogue90") | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot "variants\srogue90\rogue.nr") -Destination (Join-Path $assetRoot "variants\srogue90\rogue.nr") -Force
Copy-Item -LiteralPath (Join-Path $repoRoot "variants\srogue90\LICENSE.TXT") -Destination (Join-Path $assetRoot "variants\srogue90\LICENSE.TXT") -Force

Write-Host "Synced Android assets to $assetRoot"
