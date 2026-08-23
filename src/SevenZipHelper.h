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
        std::wstring firstItemPath;
        std::wstring baseName;
        bool isArchive = false;
    };

    // Cached 7-Zip installation paths
    const std::wstring& GetSevenZipDirectory();
    const std::wstring& Get7zGPath();
    const std::wstring& Get7zFMPath();
    const std::wstring& GetRootIconPath();

    // Zero-allocation case-insensitive archive check
    bool IsArchiveFile(const std::wstring& path);

    // Fast path utilities
    std::wstring GetFileName(const std::wstring& path);
    std::wstring GetFileNameWithoutExtension(const std::wstring& path);
    std::wstring GetParentDirectory(const std::wstring& path);

    // Thread-local memoized selection info (queries item 0 once per right-click event)
    const SelectionInfo& GetFastSelectionInfo(IShellItemArray* psiItemArray);

    // Full path retrieval (only used when user actually clicks/invokes an action)
    std::vector<std::wstring> GetPathsFromShellItemArray(IShellItemArray* psiItemArray);

    // Process launcher
    bool LaunchProcess(const std::wstring& exePath, const std::wstring& arguments, const std::wstring& workingDirectory = L"");

    // List file helper (for passing multiple files safely on invoke)
    std::wstring CreateTempListFile(const std::vector<std::wstring>& paths);
    std::wstring BuildFileArgumentList(const std::vector<std::wstring>& paths);
}
