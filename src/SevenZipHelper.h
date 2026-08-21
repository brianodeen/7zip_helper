#pragma once
#include <windows.h>
#include <shobjidl_core.h>
#include <string>
#include <vector>

namespace SevenZipHelper
{
    // Find 7-Zip installation directory
    std::wstring GetSevenZipDirectory();
    std::wstring Get7zGPath();
    std::wstring Get7zFMPath();

    // Archive check
    bool IsArchiveFile(const std::wstring& path);
    bool AnyIsArchive(const std::vector<std::wstring>& paths);
    bool AllAreArchives(const std::vector<std::wstring>& paths);

    // Extraction & path utilities
    std::wstring GetFileName(const std::wstring& path);
    std::wstring GetFileNameWithoutExtension(const std::wstring& path);
    std::wstring GetParentDirectory(const std::wstring& path);
    std::wstring GetDefaultArchiveBaseName(const std::vector<std::wstring>& paths);
    std::wstring GetDefaultDestinationFolder(const std::vector<std::wstring>& paths);

    // ShellItemArray helper
    std::vector<std::wstring> GetPathsFromShellItemArray(IShellItemArray* psiItemArray);

    // Process launcher
    bool LaunchProcess(const std::wstring& exePath, const std::wstring& arguments, const std::wstring& workingDirectory = L"");

    // List file helper (for passing multiple files safely)
    std::wstring CreateTempListFile(const std::vector<std::wstring>& paths);
    std::wstring BuildFileArgumentList(const std::vector<std::wstring>& paths);
}
