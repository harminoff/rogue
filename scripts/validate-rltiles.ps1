param(
    [string]$AssetsRoot = (Join-Path $PSScriptRoot "..\assets\rltiles")
)

$ErrorActionPreference = "Stop"

$AssetsRoot = (Resolve-Path $AssetsRoot).Path
$atlasPath = Join-Path $AssetsRoot "rltiles-2d.json"
$imagePath = Join-Path $AssetsRoot "rltiles-2d.png"
$mappingPath = Join-Path $AssetsRoot "rogue-rltiles-map.json"

foreach ($path in @($atlasPath, $imagePath, $mappingPath)) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing required rltiles asset: $path"
    }
}

$atlas = Get-Content -LiteralPath $atlasPath -Raw | ConvertFrom-Json
$mapping = Get-Content -LiteralPath $mappingPath -Raw | ConvertFrom-Json

if ($atlas.tileSize -ne 32) {
    throw "Expected rltiles tileSize 32, found $($atlas.tileSize)"
}

if (-not $atlas.tiles -or $atlas.tiles.Count -le 0) {
    throw "Atlas has no tile names: $atlasPath"
}

$tileNames = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($tile in $atlas.tiles) {
    [void]$tileNames.Add([string]$tile)
}

$missing = [System.Collections.Generic.List[string]]::new()

function Test-AtlasReference {
    param(
        [AllowNull()]$Node,
        [string]$Path
    )

    if ($null -eq $Node) {
        return
    }

    if ($Node -is [System.Array]) {
        for ($i = 0; $i -lt $Node.Count; $i++) {
            Test-AtlasReference -Node $Node[$i] -Path "$Path[$i]"
        }
        return
    }

    if ($Node -isnot [pscustomobject]) {
        return
    }

    foreach ($prop in $Node.PSObject.Properties) {
        $propPath = "$Path.$($prop.Name)"
        if ($prop.Name -eq "atlas" -and $prop.Value -is [string] -and $prop.Value -ne "") {
            if (-not $tileNames.Contains([string]$prop.Value)) {
                $script:missing.Add("$propPath -> $($prop.Value)")
            }
        }

        Test-AtlasReference -Node $prop.Value -Path $propPath
    }
}

Test-AtlasReference -Node $mapping -Path "mapping"

if ($missing.Count -gt 0) {
    $message = "Mapping references atlas keys that do not exist:`n" + ($missing -join "`n")
    throw $message
}

function Assert-HasProperties {
    param(
        [pscustomobject]$Object,
        [string[]]$Names,
        [string]$Path
    )

    foreach ($name in $Names) {
        if ($Object.PSObject.Properties.Name -notcontains $name) {
            throw "Missing required mapping entry: $Path.$name"
        }
    }
}

Assert-HasProperties -Object $mapping.terrain -Path "terrain" -Names @(
    "empty",
    "floor",
    "passage",
    "door",
    "vertical_wall",
    "horizontal_wall",
    "stairs_down",
    "known_trap"
)

Assert-HasProperties -Object $mapping.objects -Path "objects" -Names @(
    "gold",
    "potion",
    "scroll",
    "magic",
    "food",
    "weapon",
    "armor",
    "amulet",
    "ring",
    "stick"
)

Assert-HasProperties -Object $mapping.actors -Path "actors" -Names @("player")

$monsterKeys = [string[]]($mapping.monsters.PSObject.Properties.Name)
foreach ($letter in [char[]]"ABCDEFGHIJKLMNOPQRSTUVWXYZ") {
    $key = [string]$letter
    if ($monsterKeys -notcontains $key) {
        throw "Missing required monster mapping: monsters.$key"
    }
}

if ($monsterKeys.Count -ne 26) {
    throw "Expected 26 monster mappings, found $($monsterKeys.Count)"
}

$variantMonsterCount = 0
if ($mapping.PSObject.Properties.Name -contains "variantMonsters" -and $null -ne $mapping.variantMonsters) {
    foreach ($variant in $mapping.variantMonsters.PSObject.Properties) {
        $variantId = $variant.Name
        $variantKeys = [string[]]($variant.Value.PSObject.Properties.Name)
        foreach ($letter in [char[]]"ABCDEFGHIJKLMNOPQRSTUVWXYZ") {
            $key = [string]$letter
            if ($variantKeys -notcontains $key) {
                throw "Missing required variant monster mapping: variantMonsters.$variantId.$key"
            }
        }
        if ($variantKeys.Count -ne 26) {
            throw "Expected 26 monster mappings for variant '$variantId', found $($variantKeys.Count)"
        }
        $variantMonsterCount += $variantKeys.Count
    }
}

$trapIds = @{}
foreach ($trap in $mapping.traps.PSObject.Properties) {
    $trapIds[[int]$trap.Value.id] = $trap.Name
}

for ($id = 0; $id -lt 8; $id++) {
    if (-not $trapIds.ContainsKey($id)) {
        throw "Missing trap mapping for Rogue trap id $id"
    }
}

$objectCount = @($mapping.objects.PSObject.Properties).Count
Write-Host "Validated rltiles atlas references: $($atlas.tiles.Count) atlas keys, 26 base monsters, $variantMonsterCount variant monsters, $objectCount object categories."
