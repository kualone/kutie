# Kutie build script
param(
    [switch]$SetupDeps,
    [switch]$Clean,
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"
$RootDir = $PSScriptRoot

function Get-VcpkgRoot {
    if ($env:VCPKG_ROOT -and (Test-Path (Join-Path $env:VCPKG_ROOT "vcpkg.exe"))) {
        return $env:VCPKG_ROOT
    }
    foreach ($candidate in @("E:\vcpkg", "C:\vcpkg")) {
        if (Test-Path (Join-Path $candidate "vcpkg.exe")) {
            return $candidate
        }
    }
    throw "vcpkg not found. Set VCPKG_ROOT or install vcpkg to E:\vcpkg or C:\vcpkg."
}

$VcpkgRoot = Get-VcpkgRoot
$VcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
$ToolchainFile = Join-Path $VcpkgRoot "scripts/buildsystems/vcpkg.cmake"

if ($SetupDeps) {
    Write-Host "Installing manifest dependencies via vcpkg..." -ForegroundColor Cyan
    Push-Location $RootDir
    & $VcpkgExe install --triplet=x64-windows
    Pop-Location
    Write-Host "Dependencies installed." -ForegroundColor Green
    return
}

if ($Clean) {
    $BuildDir = Join-Path $RootDir "build"
    if (Test-Path $BuildDir) {
        Remove-Item $BuildDir -Recurse -Force
        Write-Host "Cleaned build directory." -ForegroundColor Green
    }
    return
}

$BuildDir = Join-Path $RootDir "build"
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vsWhere -latest -property installationPath 2>$null
if (-not $vsPath) {
    throw "Visual Studio installation not found."
}

$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
$buildScript = Join-Path $BuildDir "build_cmd.bat"

@"
@echo off
call "$vcvars" >nul 2>&1
if %ERRORLEVEL% neq 0 exit /b 1
"@ | Out-File -FilePath $buildScript -Encoding ascii

if (-not (Test-Path (Join-Path $BuildDir "build.ninja"))) {
    @"
cmake -B "$BuildDir" -S "$RootDir" -G Ninja -DCMAKE_BUILD_TYPE=$Config -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_TOOLCHAIN_FILE="$ToolchainFile"
if %ERRORLEVEL% neq 0 exit /b 1
"@ | Out-File -FilePath $buildScript -Encoding ascii -Append
} else {
    @"
cmake -B "$BuildDir" -S "$RootDir" -G Ninja -DCMAKE_BUILD_TYPE=$Config -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_TOOLCHAIN_FILE="$ToolchainFile"
if %ERRORLEVEL% neq 0 exit /b 1
"@ | Out-File -FilePath $buildScript -Encoding ascii -Append
}

@"
ninja -C "$BuildDir"
if %ERRORLEVEL% neq 0 exit /b 1
"@ | Out-File -FilePath $buildScript -Encoding ascii -Append

cmd /c $buildScript
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed." -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "Build complete." -ForegroundColor Green
$ExePath = Join-Path $BuildDir "sample.exe"
if (Test-Path $ExePath) {
    Write-Host "Output: $ExePath" -ForegroundColor Green
}
