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
- **Instantaneous Native Speed**: Fast string caching, zero-overhead selection probing, and root-only icon parsing ensures sub-millisecond context menu render times.
- **Dynamic 7-Zip Auto-Discovery**: Automatically locates your 7-Zip installation from the Windows Registry or standard program files directories (`C:\Program Files\7-Zip`, `C:\Program Files (x86)\7-Zip`, etc.).
- **Safe & Clean**: Uses Windows Sparse MSIX Package identity (`AllowExternalContent`), making installation and uninstallation fast and completely reversible with no registry clutter.

---

## Requirements

- **Windows 11** (64-bit)
- **7-Zip** installed on your system (any path)
- **PowerShell 5.1+**

---

## Quick Start (Installation for Any User)

1. Clone or download this repository:
   ```powershell
   git clone https://github.com/brianodeen/7zip_helper.git
   cd 7zip_helper
   ```
2. Open PowerShell as **Administrator** and run:
   ```powershell
   .\install.ps1
   ```
   *The script will automatically trust the package certificate, register the Sparse MSIX package, and refresh Windows Explorer.*
3. Right-click any file or folder in File Explorer to verify that 7-Zip is now in the top-level menu!

---

## Rebuilding from Source (Developers)

If you modify the C++ source code in `src/` and want to recompile:
```powershell
.\build.ps1
.\install.ps1 -Rebuild
```
*(Requires Visual Studio 2019/2022/2026 with C++ Desktop Development tools and Windows 10/11 SDK).*

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
