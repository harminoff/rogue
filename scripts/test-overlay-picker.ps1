param(
    [string]$MsysRoot = "C:\msys64"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "native-build-tests"
$mingwBin = Join-Path $MsysRoot "mingw64\bin"
$gcc = Join-Path $MsysRoot "mingw64\bin\gcc.exe"

if (-not (Test-Path $gcc)) {
    throw "Missing dependency: $gcc"
}

New-Item -ItemType Directory -Force $buildDir | Out-Null
$env:PATH = "$mingwBin;$env:PATH"

$exe = Join-Path $buildDir "overlay_picker_test.exe"
& $gcc -std=gnu89 -Wall -Wextra -I$repoRoot `
    (Join-Path $repoRoot "tests\overlay_picker_test.c") `
    (Join-Path $repoRoot "overlay_picker.c") `
    -o $exe
if ($LASTEXITCODE -ne 0) {
    throw "overlay picker test build failed"
}

& $exe
if ($LASTEXITCODE -ne 0) {
    throw "overlay picker tests failed"
}
