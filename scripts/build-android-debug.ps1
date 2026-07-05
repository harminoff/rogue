$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$androidRoot = Join-Path $repoRoot "android"

& (Join-Path $repoRoot "scripts\check-android-allegro.ps1")
& (Join-Path $repoRoot "scripts\sync-android-assets.ps1")

Push-Location $androidRoot
try {
    if (Test-Path ".\gradlew.bat") {
        .\gradlew.bat :app:assembleDebug
    }
    else {
        gradle :app:assembleDebug
    }
    if ($LASTEXITCODE -ne 0) {
        throw "Android debug build failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
