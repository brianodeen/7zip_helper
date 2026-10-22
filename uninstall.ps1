# uninstall.ps1 - Unregisters and removes the 7-Zip Modern Context Menu Sparse Package
[CmdletBinding()]
param ()

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $ScriptDir

# Check for Administrator privileges
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "Administrator privileges are required to unregister the shell extension." -ForegroundColor Yellow
    try {
        Write-Host "Attempting to launch elevated PowerShell prompt..." -ForegroundColor Cyan
        $process = Start-Process powershell.exe -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs -PassThru -Wait
        exit $process.ExitCode
    } catch {
        Write-Host ""
        Write-Host "==========================================================================" -ForegroundColor Red
        Write-Host " ELEVATION REQUIRED: Please open PowerShell as Administrator and run:     " -ForegroundColor Yellow
        Write-Host "   Set-Location `"$ScriptDir`"" -ForegroundColor Cyan
        Write-Host "   .\uninstall.ps1" -ForegroundColor Cyan
        Write-Host "==========================================================================" -ForegroundColor Red
        Write-Host ""
        exit 1
    }
}

Write-Host "=================================================" -ForegroundColor Cyan
Write-Host " Uninstalling 7-Zip Modern Context Menu Extension" -ForegroundColor Cyan
Write-Host "=================================================" -ForegroundColor Cyan

# Terminate any warm dllhost holding SevenZipContextMenu.dll
Get-Process dllhost -ErrorAction SilentlyContinue | ForEach-Object {
    try {
        if ($_.Modules.FileName -match 'SevenZipContextMenu\.dll') {
            Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
        }
    } catch {}
}

$pkg = Get-AppxPackage -Name "SevenZipContextMenu" -AllUsers -ErrorAction SilentlyContinue

if ($pkg) {
    Write-Host "Found package: $($pkg.PackageFullName)" -ForegroundColor Yellow
    Write-Host "Removing package registration..." -ForegroundColor Yellow
    Remove-AppxPackage -Package $pkg.PackageFullName -AllUsers -ErrorAction SilentlyContinue
    Write-Host "Package unregistered successfully." -ForegroundColor Green
} else {
    Write-Host "No active SevenZipContextMenu package was found." -ForegroundColor DarkYellow
}

# Clean up InProcServer32 registry registration
$clsidKey = "HKCU:\Software\Classes\CLSID\{4B5E8908-7211-460B-A512-88F9A64B7C99}"
if (Test-Path $clsidKey) {
    Remove-Item -Path $clsidKey -Recurse -Force -ErrorAction SilentlyContinue
}

# Clean up certificate from LocalMachine\TrustedPeople
$certSubject = "CN=SevenZipContextMenu"
$cert = Get-ChildItem "Cert:\LocalMachine\TrustedPeople" -ErrorAction SilentlyContinue | Where-Object { $_.Subject -eq $certSubject }
if ($cert) {
    Write-Host "Removing test certificate from LocalMachine\TrustedPeople..." -ForegroundColor Yellow
    $store = New-Object System.Security.Cryptography.X509Certificates.X509Store("TrustedPeople", "LocalMachine")
    $store.Open("ReadWrite")
    foreach ($c in $cert) {
        $store.Remove($c)
    }
    $store.Close()
}

$msixPath = Join-Path $ScriptDir "SevenZipContextMenu.msix"
if (Test-Path $msixPath) {
    Remove-Item $msixPath -Force -ErrorAction SilentlyContinue
}

Write-Host "Restarting Windows Explorer..." -ForegroundColor Cyan
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1
if (-not (Get-Process -Name explorer -ErrorAction SilentlyContinue)) {
    Start-Process explorer.exe
}

Write-Host "=================================================" -ForegroundColor Green
Write-Host " Uninstallation completed." -ForegroundColor Green
Write-Host "=================================================" -ForegroundColor Green
