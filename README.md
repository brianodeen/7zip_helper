# 7-Zip Modern Windows 11 Context Menu Extension

A lightweight, native C++ Windows 11 Shell Extension that integrates **7-Zip** directly into the modern File Explorer right-click context menu (bypassing the need to click "Show more options").

---

## Features

- **Direct Top-Level Access**: Right-click any file, folder, or drive in Windows 11 to immediately see the **7-Zip** menu and its official icon.
- **Smart, Context-Aware Actions**:
  - **When clicking Archive files (`.7z`, `.zip`, `.rar`, `.tar.gz`, `.iso`, etc.)**:
    - `Open archive` (Opens in 7-Zip File Manager)
    - `Extract files...` (Opens the standard 7-Zip Extract GUI dialog)
    - `Extract Here` (Extracts directly into the current directory)
    - `Extract to "<ArchiveName>\"` (Extracts into a subfolder named after the archive)
    - `Test archive` (Tests archive integrity)
    - `Add to archive...`
    - `Compress and email...`
    - `Add to "<ArchiveName>.7z"`
    - `Add to "<ArchiveName>.zip"`
    - `CRC SHA` checksum calculations (All Hashes `*`, SHA-256, CRC-32)
  - **When clicking Regular Files or Folders**:
    - `Add to archive...` (Opens the standard 7-Zip Compression dialog)
    - `Compress and email...`
    - `Add to "<FileName>.7z"` (Instant one-click compression)
    - `Add to "<FileName>.zip"` (Instant one-click compression)
    - `Compress to "<FileName>.7z" and email`
    - `Compress to "<FileName>.zip" and email`
    - `CRC SHA` checksum calculation
- **Native COM Performance**: Implements Microsoft's modern `IExplorerCommand` and `IEnumExplorerCommand` interfaces in native C++ with negligible latency.
- **Safe & Clean**: Uses Windows Sparse MSIX Package identity (`AllowExternalContent`), making installation and uninstallation fast and reversible with no registry corruption.

---

## Requirements

- **Windows 11** (64-bit)
- **7-Zip** installed on your system (typically `C:\Program Files\7-Zip`)
- **PowerShell 5.1+**

---

## Quick Start (Installation)

1. Open PowerShell and navigate to this folder:
   ```powershell
   cd c:\Users\brian\Documents\antigravity\7zip_helper
   ```
2. Run the installation script:
   ```powershell
   .\install.ps1
   ```
   *The script will compile the DLL (if needed), create a local package signature, register the Sparse MSIX package, and restart Windows Explorer.*
3. Right-click any file or folder in File Explorer to verify that 7-Zip is now in the top-level context menu!

---

## Uninstallation

To remove the extension at any time:
```powershell
.\uninstall.ps1
```

---

## How It Works

1. **`SevenZipContextMenu.dll`**: Implements the `IExplorerCommand` COM interface and enumerates 7-Zip subcommands. It inspects selected items via `IShellItemArray`, dynamically customizes menu labels (e.g. `Extract to "MyFolder\"`), and invokes `7zG.exe` / `7zFM.exe`.
2. **`Package.appxmanifest`**: Grants the unpackaged DLL modern Windows 11 Package Identity via `desktop4:FileExplorerContextMenus` and `uap10:AllowExternalContent="true"`.
3. **`7zG.exe` & `7zFM.exe`**: All compression and extraction tasks are handed off to your local 7-Zip binaries, preserving full compatibility with 7-Zip GUI dialogs, password prompts, and progress bars.
