param(
    [string]$MsysRoot = "C:\msys64"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$includeRoot = $repoRoot -replace '\\', '/'
$buildDir = Join-Path $repoRoot "native-build-tests"
$mingwBin = Join-Path $MsysRoot "mingw64\bin"
$gcc = Join-Path $MsysRoot "mingw64\bin\gcc.exe"

if (-not (Test-Path $gcc)) {
    throw "Missing dependency: $gcc"
}

New-Item -ItemType Directory -Force $buildDir | Out-Null
$env:PATH = "$mingwBin;$env:PATH"

$exe = Join-Path $buildDir "mobile_controls_test.exe"
& $gcc -std=gnu89 -Wall -Wextra "-I$includeRoot" `
    (Join-Path $repoRoot "tests\mobile_controls_test.c") `
    (Join-Path $repoRoot "mobile_controls.c") `
    -o $exe
if ($LASTEXITCODE -ne 0) {
    throw "mobile controls test build failed"
}

& $exe
if ($LASTEXITCODE -ne 0) {
    throw "mobile controls tests failed"
}
