# build.ps1 - Builds SevenZipContextMenu.dll and SevenZipContextMenu.exe, and prepares the package folder
[CmdletBinding()]
param (
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $ScriptDir

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " Building 7-Zip Modern Context Menu      " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# Locate VS vcvars64.bat or MSBuild
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "Visual Studio Installer (vswhere.exe) not found."
}

$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath -or -not (Test-Path $vsPath)) {
    throw "Visual Studio with C++ Build Tools not found."
}

$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
$buildDir = Join-Path $ScriptDir "build"

if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
}

Write-Host "Configuring CMake (x64)..." -ForegroundColor Yellow
& cmd.exe /c "call `"$vcvars`" && cmake -B `"$buildDir`" -S `"$ScriptDir`" -A x64"
if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed."
}

Write-Host "Building $Configuration..." -ForegroundColor Yellow
& cmd.exe /c "call `"$vcvars`" && cmake --build `"$buildDir`" --config $Configuration"
if ($LASTEXITCODE -ne 0) {
    throw "CMake build failed."
}

$dllSource = Join-Path $buildDir "$Configuration\SevenZipContextMenu.dll"
if (-not (Test-Path $dllSource)) {
    $dllSource = Join-Path $buildDir "SevenZipContextMenu.dll"
}

$exeSource = Join-Path $buildDir "$Configuration\SevenZipContextMenu.exe"
if (-not (Test-Path $exeSource)) {
    $exeSource = Join-Path $buildDir "SevenZipContextMenu.exe"
}

if (-not (Test-Path $dllSource)) {
    throw "Compiled DLL not found at: $dllSource"
}

if (-not (Test-Path $exeSource)) {
    throw "Compiled EXE not found at: $exeSource"
}

# Copy output to package folder
$packageDir = Join-Path $ScriptDir "package"
Copy-Item -Path $dllSource -Destination (Join-Path $packageDir "SevenZipContextMenu.dll") -Force
Copy-Item -Path $exeSource -Destination (Join-Path $packageDir "SevenZipContextMenu.exe") -Force

Write-Host "=========================================" -ForegroundColor Green
Write-Host " Build succeeded! Output binaries copied to: " -ForegroundColor Green
Write-Host " $(Join-Path $packageDir 'SevenZipContextMenu.dll')" -ForegroundColor Green
Write-Host " $(Join-Path $packageDir 'SevenZipContextMenu.exe')" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green
