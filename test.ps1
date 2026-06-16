# Kutie smoke tests
# Requires a built sample.exe (Release by default).

param(
    [string]$Config = "Release",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$RootDir = $PSScriptRoot
$BuildDir = Join-Path $RootDir "build"
$SampleExe = Join-Path $BuildDir "sample\sample.exe"
$UnitExe = Join-Path $BuildDir "tests\kutie_smoke_frameless.exe"
$BrowserWindowExe = Join-Path $BuildDir "tests\kutie_smoke_browser_window.exe"

if (-not $SkipBuild) {
    & (Join-Path $RootDir "build.ps1") -Config $Config
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Get-Process sample -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

Write-Host "Running kutie_smoke_frameless..." -ForegroundColor Cyan
if (-not (Test-Path $UnitExe)) {
    Write-Host "Missing $UnitExe — configure with -DKUTIE_BUILD_TESTS=ON" -ForegroundColor Red
    exit 1
}
& $UnitExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Running kutie_smoke_browser_window..." -ForegroundColor Cyan
if (-not (Test-Path $BrowserWindowExe)) {
    Write-Host "Missing $BrowserWindowExe — configure with -DKUTIE_BUILD_TESTS=ON" -ForegroundColor Red
    exit 1
}
& $BrowserWindowExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Running custom-titlebar smoke..." -ForegroundColor Cyan
& (Join-Path $RootDir "tools\smoke\custom-titlebar.ps1") -SampleExe $SampleExe
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Running multi-window smoke..." -ForegroundColor Cyan
& (Join-Path $RootDir "tools\smoke\multi-window.ps1") -SampleExe $SampleExe
exit $LASTEXITCODE
