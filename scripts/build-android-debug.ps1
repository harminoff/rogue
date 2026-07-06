$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$androidRoot = Join-Path $repoRoot "android"
$gradlew = Join-Path $androidRoot "gradlew.bat"

& (Join-Path $repoRoot "scripts\check-android-allegro.ps1")

if (-not (Test-Path $gradlew)) {
    throw "Android Gradle wrapper not found. Generate it under android\ or install project wrapper before building."
}

& (Join-Path $repoRoot "scripts\sync-android-assets.ps1")

Push-Location $androidRoot
try {
    & $gradlew :app:assembleDebug
    if ($LASTEXITCODE -ne 0) {
        throw "Android debug build failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
