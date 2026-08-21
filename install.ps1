# install.ps1 - Packages and installs the 7-Zip Modern Context Menu Sparse Package
[CmdletBinding()]
param ()

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $ScriptDir

# Check for Administrator privileges
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "Administrator privileges are required to trust the local test certificate and register the shell extension." -ForegroundColor Yellow
    try {
        Write-Host "Attempting to launch elevated PowerShell prompt..." -ForegroundColor Cyan
        $process = Start-Process powershell.exe -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs -PassThru -Wait
        exit $process.ExitCode
    } catch {
        Write-Host ""
        Write-Host "==========================================================================" -ForegroundColor Red
        Write-Host " ELEVATION REQUIRED: Please open PowerShell as Administrator and run:     " -ForegroundColor Yellow
        Write-Host "   Set-Location `"$ScriptDir`"" -ForegroundColor Cyan
        Write-Host "   .\install.ps1" -ForegroundColor Cyan
        Write-Host "==========================================================================" -ForegroundColor Red
        Write-Host ""
        exit 1
    }
}

Write-Host "===============================================" -ForegroundColor Cyan
Write-Host " Installing 7-Zip Modern Context Menu Extension" -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan

# 1. Check if binaries exist; if not, build them
$packageDir = Join-Path $ScriptDir "package"
$dllPath = Join-Path $packageDir "SevenZipContextMenu.dll"
$exePath = Join-Path $packageDir "SevenZipContextMenu.exe"
$manifestPath = Join-Path $packageDir "AppxManifest.xml"

if ((-not (Test-Path $dllPath)) -or (-not (Test-Path $exePath))) {
    Write-Host "Binaries not found in package folder. Running build.ps1..." -ForegroundColor Yellow
    & (Join-Path $ScriptDir "build.ps1")
}

if (-not (Test-Path $dllPath)) {
    throw "Failed to locate SevenZipContextMenu.dll at: $dllPath"
}

# 2. Check 7-Zip installation
$sevenZipExe = "C:\Program Files\7-Zip\7zG.exe"
if (-not (Test-Path $sevenZipExe)) {
    Write-Warning "7-Zip not found at default location ($sevenZipExe). Ensure 7-Zip is installed."
} else {
    Write-Host "Found 7-Zip at: $sevenZipExe" -ForegroundColor Green
}

# 3. Locate makeappx.exe and signtool.exe
$sdkRoot = "${env:ProgramFiles(x86)}\Windows Kits\10\bin"
$sdkVersions = Get-ChildItem $sdkRoot -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending
$makeappx = $null
$signtool = $null

foreach ($ver in $sdkVersions) {
    $mPath = Join-Path $ver.FullName "x64\makeappx.exe"
    $sPath = Join-Path $ver.FullName "x64\signtool.exe"
    if ((Test-Path $mPath) -and (Test-Path $sPath)) {
        $makeappx = $mPath
        $signtool = $sPath
        break
    }
}

if (-not $makeappx -or -not $signtool) {
    throw "Windows SDK tools (makeappx.exe / signtool.exe) not found."
}

# 4. Remove previous registration if present
$existing = Get-AppxPackage -Name "SevenZipContextMenu" -AllUsers -ErrorAction SilentlyContinue
if ($existing) {
    Write-Host "Removing existing package registration..." -ForegroundColor Yellow
    Remove-AppxPackage -Package $existing.PackageFullName -AllUsers -ErrorAction SilentlyContinue
}

# 5. Create / Retrieve Self-Signed Certificate
$certSubject = "CN=SevenZipContextMenu"
$cert = Get-ChildItem "Cert:\CurrentUser\My" | Where-Object { $_.Subject -eq $certSubject } | Select-Object -First 1

if (-not $cert) {
    Write-Host "Creating self-signed certificate for Sparse Package signing..." -ForegroundColor Yellow
    $cert = New-SelfSignedCertificate `
        -Type Custom `
        -Subject $certSubject `
        -KeyUsage DigitalSignature `
        -FriendlyName "7-Zip Context Menu Test Certificate" `
        -CertStoreLocation "Cert:\CurrentUser\My" `
        -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3")
}

# Export and import certificate into LocalMachine\TrustedPeople
$cerPath = Join-Path $env:TEMP "SevenZipContextMenu.cer"
Export-Certificate -Cert $cert -FilePath $cerPath -Force | Out-Null
Import-Certificate -FilePath $cerPath -CertStoreLocation "Cert:\LocalMachine\TrustedPeople" | Out-Null
Write-Host "Certificate trusted in LocalMachine\TrustedPeople." -ForegroundColor Green

# 6. Package MSIX
$msixPath = Join-Path $ScriptDir "SevenZipContextMenu.msix"
if (Test-Path $msixPath) {
    Remove-Item $msixPath -Force
}

Write-Host "Packing Sparse MSIX package..." -ForegroundColor Yellow
& $makeappx pack /d "$packageDir" /p "$msixPath" /nv /o
if ($LASTEXITCODE -ne 0) {
    throw "makeappx pack failed with exit code $LASTEXITCODE"
}

# 7. Sign MSIX
Write-Host "Signing Sparse MSIX package..." -ForegroundColor Yellow
& $signtool sign /fd SHA256 /sha1 $cert.Thumbprint /v "$msixPath"
if ($LASTEXITCODE -ne 0) {
    throw "signtool sign failed with exit code $LASTEXITCODE"
}

# 8. Install Signed Sparse MSIX with ExternalLocation
Write-Host "Installing Signed Sparse MSIX with ExternalLocation..." -ForegroundColor Yellow
Add-AppxPackage -Path "$msixPath" -ExternalLocation "$packageDir"

# 9. Verify installation
$check = Get-AppxPackage -Name "SevenZipContextMenu" -ErrorAction SilentlyContinue
if ($check) {
    Write-Host "===============================================" -ForegroundColor Green
    Write-Host " 7-Zip Modern Context Menu successfully installed!" -ForegroundColor Green
    Write-Host " Package: $($check.PackageFullName)" -ForegroundColor Green
    Write-Host "===============================================" -ForegroundColor Green

    # Restart Explorer to refresh context menus
    Write-Host "Restarting Windows Explorer to refresh context menus..." -ForegroundColor Cyan
    Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1
    if (-not (Get-Process -Name explorer -ErrorAction SilentlyContinue)) {
        Start-Process explorer.exe
    }
    Write-Host "Explorer restarted! Right-click any file or folder to see 7-Zip in the top-level menu." -ForegroundColor Green
} else {
    throw "Package installation verification failed."
}
