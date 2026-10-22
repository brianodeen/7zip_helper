# install.ps1 - Packages and installs the 7-Zip Modern Context Menu Sparse Package
[CmdletBinding()]
param (
    [switch]$Rebuild
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location $ScriptDir

$packageDir = Join-Path $ScriptDir "package"
$dllPath = Join-Path $packageDir "SevenZipContextMenu.dll"
$exePath = Join-Path $packageDir "SevenZipContextMenu.exe"
$msixPath = Join-Path $ScriptDir "SevenZipContextMenu.msix"

# 1. Locate Windows SDK (optional if pre-built MSIX is present)
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

# 2. Check if rebuilding from source is requested or necessary
$needsBuild = $Rebuild -or (-not (Test-Path $dllPath)) -or (-not (Test-Path $exePath))
if ($needsBuild) {
    Write-Host "Building C++ binaries from source..." -ForegroundColor Yellow
    & (Join-Path $ScriptDir "build.ps1")
}

$needsPack = $needsBuild -or (-not (Test-Path $msixPath))
if ($needsPack) {
    if (-not $makeappx -or -not $signtool) {
        throw "Windows SDK tools (makeappx.exe / signtool.exe) not found to package MSIX. Please install the Windows SDK."
    }

    $certSubject = "CN=SevenZipContextMenu"
    $cert = Get-ChildItem "Cert:\CurrentUser\My" | Where-Object { $_.Subject -eq $certSubject } | Select-Object -First 1

    if (-not $cert) {
        Write-Host "Creating self-signed certificate..." -ForegroundColor Yellow
        $cert = New-SelfSignedCertificate `
            -Type Custom `
            -Subject $certSubject `
            -KeyUsage DigitalSignature `
            -FriendlyName "7-Zip Context Menu Test Certificate" `
            -CertStoreLocation "Cert:\CurrentUser\My" `
            -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3")
    }

    if (Test-Path $msixPath) {
        Remove-Item $msixPath -Force
    }

    Write-Host "Packing Sparse MSIX package..." -ForegroundColor Yellow
    & $makeappx pack /d "$packageDir" /p "$msixPath" /nv /o
    if ($LASTEXITCODE -ne 0) {
        throw "makeappx pack failed with exit code $LASTEXITCODE"
    }

    Write-Host "Signing Sparse MSIX package..." -ForegroundColor Yellow
    & $signtool sign /fd SHA256 /sha1 $cert.Thumbprint /v "$msixPath"
    if ($LASTEXITCODE -ne 0) {
        throw "signtool sign failed with exit code $LASTEXITCODE"
    }
}

# 3. Check if signer certificate is already trusted in LocalMachine\TrustedPeople
$sig = Get-AuthenticodeSignature $msixPath
$isTrusted = $false
if ($sig.SignerCertificate) {
    $trustedCert = Get-ChildItem "Cert:\LocalMachine\TrustedPeople" -ErrorAction SilentlyContinue |
        Where-Object { $_.Thumbprint -eq $sig.SignerCertificate.Thumbprint } |
        Select-Object -First 1
    if ($trustedCert) {
        $isTrusted = $true
    }
}

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isTrusted -and -not $isAdmin) {
    Write-Host "Administrator privileges are required once to trust the package signing certificate." -ForegroundColor Yellow
    try {
        Write-Host "Launching elevated PowerShell prompt..." -ForegroundColor Cyan
        $argList = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
        $process = Start-Process powershell.exe -ArgumentList $argList -Verb RunAs -PassThru -Wait
        exit $process.ExitCode
    } catch {
        Write-Host "Elevation was cancelled or failed. Please run .\install.ps1 as Administrator." -ForegroundColor Red
        exit 1
    }
}

if (-not $isTrusted -and $sig.SignerCertificate) {
    $store = New-Object System.Security.Cryptography.X509Certificates.X509Store("TrustedPeople", "LocalMachine")
    $store.Open("ReadWrite")
    $store.Add($sig.SignerCertificate)
    $store.Close()
    Write-Host "Signer certificate trusted in LocalMachine\TrustedPeople." -ForegroundColor Green
}

Write-Host "===============================================" -ForegroundColor Cyan
Write-Host " Installing 7-Zip Modern Context Menu Extension" -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan

# 4. Pre-fetch and store 7-Zip installation path in HKCU:\Software\7-Zip
$sevenZipCandidates = @(
    (Get-ItemProperty "HKCU:\Software\7-Zip" -ErrorAction SilentlyContinue).Path,
    (Get-ItemProperty "HKLM:\SOFTWARE\7-Zip" -ErrorAction SilentlyContinue).Path,
    (Get-ItemProperty "HKLM:\SOFTWARE\WOW6432Node\7-Zip" -ErrorAction SilentlyContinue).Path,
    "C:\Program Files\7-Zip",
    "C:\Program Files (x86)\7-Zip"
)
foreach ($candidate in $sevenZipCandidates) {
    if ($candidate) {
        $cleanPath = $candidate.TrimEnd('\', '/')
        if (Test-Path (Join-Path $cleanPath "7zG.exe")) {
            if (-not (Test-Path "HKCU:\Software\7-Zip")) {
                New-Item -Path "HKCU:\Software\7-Zip" -Force | Out-Null
            }
            Set-ItemProperty -Path "HKCU:\Software\7-Zip" -Name "Path" -Value $cleanPath
            Write-Host "Pre-fetched 7-Zip location: $cleanPath" -ForegroundColor Green
            break
        }
    }
}

# 5. Remove previous package registration if present
$existing = Get-AppxPackage -Name "SevenZipContextMenu" -ErrorAction SilentlyContinue
if ($existing) {
    Write-Host "Removing previous package registration..." -ForegroundColor Yellow
    Remove-AppxPackage -Package $existing.PackageFullName -ErrorAction SilentlyContinue
}

# 6. Install Signed Sparse MSIX with ExternalLocation
Write-Host "Installing Signed Sparse MSIX with ExternalLocation..." -ForegroundColor Yellow
Add-AppxPackage -Path "$msixPath" -ExternalLocation "$packageDir"

# 7. Register InProcServer32 in HKCU for zero-latency in-process COM activation
$clsidKey = "HKCU:\Software\Classes\CLSID\{4B5E8908-7211-460B-A512-88F9A64B7C99}"
$inprocKey = "$clsidKey\InProcServer32"
New-Item -Path $inprocKey -Force | Out-Null
Set-ItemProperty -Path $clsidKey -Name "(default)" -Value "7-Zip Modern Context Menu"
Set-ItemProperty -Path $inprocKey -Name "(default)" -Value $dllPath
Set-ItemProperty -Path $inprocKey -Name "ThreadingModel" -Value "Both"
Write-Host "Registered InProcServer32 (ThreadingModel=Both) at: $dllPath" -ForegroundColor Green

# 8. Verify installation
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
