#pragma once
#include <windows.h>
#include <shobjidl_core.h>
#include <string>
#include <vector>

namespace SevenZipHelper
{
    struct SelectionInfo
    {
        DWORD count = 0;
        bool isArchive = false;
        std::wstring firstItemPath;
        std::wstring baseName;

        // Pre-formatted dynamic menu titles built once per selection
        std::wstring titleExtractTo;
        std::wstring titleAdd7z;
        std::wstring titleAddZip;
        std::wstring titleEmail7z;
        std::wstring titleEmailZip;
    };

    // Pre-warm all static paths at DLL load time
    void WarmupCache();

    // Re-discover 7-Zip installation if it was moved/updated
    void RefreshSevenZipPaths();

    // Cached 7-Zip installation paths
    const std::wstring& GetSevenZipDirectory();
    const std::wstring& Get7zGPath();
    const std::wstring& Get7zFMPath();
    const std::wstring& GetRootIconPath();

    // O(1) compile-time packed integer archive extension check
    bool IsArchiveFile(const std::wstring& path) noexcept;

    // Fast path utilities
    std::wstring GetFileName(const std::wstring& path);
    std::wstring GetFileNameWithoutExtension(const std::wstring& path);
    std::wstring GetParentDirectory(const std::wstring& path);

    // Called once on RootCommand::GetState to populate the selection cache for all 16 subcommands
    void UpdateSelectionCache(IShellItemArray* psiItemArray);

    // Returns the cached SelectionInfo with zero COM calls during subcommand enumeration
    SelectionInfo GetCachedSelectionInfo(IShellItemArray* psiItemArray);

    // Full path retrieval (only used when user actually clicks/invokes an action)
    std::vector<std::wstring> GetPathsFromShellItemArray(IShellItemArray* psiItemArray);

    // Process launcher with automatic path self-healing if 7-Zip moved
    bool LaunchProcess(const std::wstring& exePath, const std::wstring& arguments, const std::wstring& workingDirectory = L"");

    // List file helper (for passing multiple files safely on invoke)
    std::wstring CreateTempListFile(const std::vector<std::wstring>& paths);
    std::wstring BuildFileArgumentList(const std::vector<std::wstring>& paths);
}
