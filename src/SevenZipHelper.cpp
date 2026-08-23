#include "SevenZipHelper.h"
#include <shlwapi.h>
#include <fstream>
#include <sstream>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")

namespace SevenZipHelper
{
    static bool ReadRegistryPath(HKEY hRootKey, const wchar_t* subKey, std::wstring& outPath)
    {
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(hRootKey, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            wchar_t buffer[MAX_PATH] = { 0 };
            DWORD bufferSize = sizeof(buffer);
            DWORD type = REG_SZ;
            if (RegQueryValueExW(hKey, L"Path", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufferSize) == ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                outPath = buffer;
                if (!outPath.empty() && (outPath.back() == L'\\' || outPath.back() == L'/'))
                {
                    outPath.pop_back();
                }
                return true;
            }
            RegCloseKey(hKey);
        }
        return false;
    }

    static std::wstring FindSevenZipDirectoryInternal()
    {
        std::wstring path;
        if (ReadRegistryPath(HKEY_CURRENT_USER, L"Software\\7-Zip", path) && PathFileExistsW((path + L"\\7zG.exe").c_str()))
        {
            return path;
        }
        if (ReadRegistryPath(HKEY_LOCAL_MACHINE, L"SOFTWARE\\7-Zip", path) && PathFileExistsW((path + L"\\7zG.exe").c_str()))
        {
            return path;
        }
        if (ReadRegistryPath(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\7-Zip", path) && PathFileExistsW((path + L"\\7zG.exe").c_str()))
        {
            return path;
        }

        if (PathFileExistsW(L"C:\\Program Files\\7-Zip\\7zG.exe"))
        {
            return L"C:\\Program Files\\7-Zip";
        }
        if (PathFileExistsW(L"C:\\Program Files (x86)\\7-Zip\\7zG.exe"))
        {
            return L"C:\\Program Files (x86)\\7-Zip";
        }

        return L"C:\\Program Files\\7-Zip";
    }

    const std::wstring& GetSevenZipDirectory()
    {
        static const std::wstring s_sevenZipDir = FindSevenZipDirectoryInternal();
        return s_sevenZipDir;
    }

    const std::wstring& Get7zGPath()
    {
        static const std::wstring s_7zGPath = GetSevenZipDirectory() + L"\\7zG.exe";
        return s_7zGPath;
    }

    const std::wstring& Get7zFMPath()
    {
        static const std::wstring s_7zFMPath = GetSevenZipDirectory() + L"\\7zFM.exe";
        return s_7zFMPath;
    }

    const std::wstring& GetRootIconPath()
    {
        static const std::wstring s_iconPath = Get7zFMPath() + L",0";
        return s_iconPath;
    }

    bool IsArchiveFile(const std::wstring& path)
    {
        if (path.empty()) return false;

        size_t lastDot = path.find_last_of(L'.');
        if (lastDot == std::wstring::npos) return false;

        const wchar_t* ext = path.c_str() + lastDot;

        static const wchar_t* const s_extensions[] = {
            L".7z", L".zip", L".rar", L".tar", L".gz", L".tgz", L".bz2", L".tbz2", L".tbz",
            L".xz", L".txz", L".iso", L".wim", L".cab", L".arj", L".lzh", L".lha", L".z",
            L".taz", L".cpio", L".rpm", L".deb", L".dmg", L".hfs", L".xar", L".vhd", L".vhdx",
            L".fat", L".ntfs", L".squashfs", L".qcow", L".vmdk", L".vdi", L".001", L".z01", L".zipx",
            L".jar", L".war", L".ear", L".apk", L".ipa"
        };

        for (const auto* knownExt : s_extensions)
        {
            if (_wcsicmp(ext, knownExt) == 0)
            {
                return true;
            }
        }

        // Check compound extensions (e.g. .tar.gz)
        if (lastDot > 4)
        {
            size_t prevDot = path.find_last_of(L'.', lastDot - 1);
            if (prevDot != std::wstring::npos)
            {
                const wchar_t* compoundExt = path.c_str() + prevDot;
                if (_wcsicmp(compoundExt, L".tar.gz") == 0 ||
                    _wcsicmp(compoundExt, L".tar.bz2") == 0 ||
                    _wcsicmp(compoundExt, L".tar.xz") == 0 ||
                    _wcsicmp(compoundExt, L".tar.z") == 0)
                {
                    return true;
                }
            }
        }

        return false;
    }

    std::wstring GetFileName(const std::wstring& path)
    {
        if (path.empty()) return L"";
        size_t lastSlash = path.find_last_of(L"\\/");
        if (lastSlash == std::wstring::npos)
        {
            return path;
        }
        return path.substr(lastSlash + 1);
    }

    std::wstring GetFileNameWithoutExtension(const std::wstring& path)
    {
        std::wstring fileName = GetFileName(path);
        if (fileName.empty()) return L"";

        size_t lastDot = fileName.find_last_of(L'.');
        if (lastDot == std::wstring::npos || lastDot == 0)
        {
            return fileName;
        }

        // Check compound extensions
        if (lastDot > 4)
        {
            size_t prevDot = fileName.find_last_of(L'.', lastDot - 1);
            if (prevDot != std::wstring::npos)
            {
                const wchar_t* compoundExt = fileName.c_str() + prevDot;
                if (_wcsicmp(compoundExt, L".tar.gz") == 0 ||
                    _wcsicmp(compoundExt, L".tar.bz2") == 0 ||
                    _wcsicmp(compoundExt, L".tar.xz") == 0 ||
                    _wcsicmp(compoundExt, L".tar.z") == 0)
                {
                    return fileName.substr(0, prevDot);
                }
            }
        }

        return fileName.substr(0, lastDot);
    }

    std::wstring GetParentDirectory(const std::wstring& path)
    {
        if (path.empty()) return L"";
        size_t lastSlash = path.find_last_of(L"\\/");
        if (lastSlash == std::wstring::npos)
        {
            return L"";
        }
        return path.substr(0, lastSlash);
    }

    const SelectionInfo& GetFastSelectionInfo(IShellItemArray* psiItemArray)
    {
        static thread_local IShellItemArray* s_lastItemArray = nullptr;
        static thread_local SelectionInfo s_cachedInfo;

        if (!psiItemArray)
        {
            static const SelectionInfo s_emptyInfo{ 0, L"", L"Archive", false };
            return s_emptyInfo;
        }

        // Pointer memoization: if this is the same IShellItemArray pointer from Explorer, return cached immediately
        if (psiItemArray == s_lastItemArray)
        {
            return s_cachedInfo;
        }

        s_lastItemArray = psiItemArray;
        s_cachedInfo = SelectionInfo{};

        if (FAILED(psiItemArray->GetCount(&s_cachedInfo.count)) || s_cachedInfo.count == 0)
        {
            s_cachedInfo.baseName = L"Archive";
            return s_cachedInfo;
        }

        IShellItem* pItem = nullptr;
        if (SUCCEEDED(psiItemArray->GetItemAt(0, &pItem)) && pItem)
        {
            LPWSTR pszPath = nullptr;
            if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)) && pszPath)
            {
                s_cachedInfo.firstItemPath = pszPath;
                CoTaskMemFree(pszPath);
            }
            pItem->Release();
        }

        if (!s_cachedInfo.firstItemPath.empty())
        {
            s_cachedInfo.isArchive = IsArchiveFile(s_cachedInfo.firstItemPath);
            if (s_cachedInfo.count == 1)
            {
                s_cachedInfo.baseName = GetFileNameWithoutExtension(s_cachedInfo.firstItemPath);
            }
            else
            {
                std::wstring parentDir = GetParentDirectory(s_cachedInfo.firstItemPath);
                s_cachedInfo.baseName = GetFileName(parentDir);
            }
        }

        if (s_cachedInfo.baseName.empty())
        {
            s_cachedInfo.baseName = L"Archive";
        }

        return s_cachedInfo;
    }

    std::vector<std::wstring> GetPathsFromShellItemArray(IShellItemArray* psiItemArray)
    {
        std::vector<std::wstring> results;
        if (!psiItemArray) return results;

        DWORD count = 0;
        if (SUCCEEDED(psiItemArray->GetCount(&count)))
        {
            results.reserve(count);
            for (DWORD i = 0; i < count; ++i)
            {
                IShellItem* pItem = nullptr;
                if (SUCCEEDED(psiItemArray->GetItemAt(i, &pItem)) && pItem)
                {
                    LPWSTR pszPath = nullptr;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)) && pszPath)
                    {
                        results.push_back(pszPath);
                        CoTaskMemFree(pszPath);
                    }
                    pItem->Release();
                }
            }
        }
        return results;
    }

    std::wstring CreateTempListFile(const std::vector<std::wstring>& paths)
    {
        wchar_t tempDir[MAX_PATH] = { 0 };
        GetTempPathW(MAX_PATH, tempDir);

        GUID guid;
        CoCreateGuid(&guid);
        wchar_t guidStr[64] = { 0 };
        StringFromGUID2(guid, guidStr, 64);

        std::wstring listFilePath = std::wstring(tempDir) + L"7z_" + guidStr + L".tmp";

        // Write UTF-8 list file
        std::ofstream outFile(listFilePath, std::ios::out | std::ios::binary);
        if (outFile.is_open())
        {
            for (const auto& p : paths)
            {
                int len = WideCharToMultiByte(CP_UTF8, 0, p.c_str(), -1, nullptr, 0, nullptr, nullptr);
                if (len > 0)
                {
                    std::vector<char> utf8Buf(len);
                    WideCharToMultiByte(CP_UTF8, 0, p.c_str(), -1, utf8Buf.data(), len, nullptr, nullptr);
                    outFile.write(utf8Buf.data(), len - 1);
                    outFile.write("\r\n", 2);
                }
            }
            outFile.close();
        }
        return listFilePath;
    }

    std::wstring BuildFileArgumentList(const std::vector<std::wstring>& paths)
    {
        if (paths.empty()) return L"";

        if (paths.size() <= 4)
        {
            std::wstring args;
            for (size_t i = 0; i < paths.size(); ++i)
            {
                if (i > 0) args += L" ";
                args += L"\"" + paths[i] + L"\"";
            }
            return args;
        }

        std::wstring listFile = CreateTempListFile(paths);
        return L"@\"" + listFile + L"\" -scsUTF-8";
    }

    bool LaunchProcess(const std::wstring& exePath, const std::wstring& arguments, const std::wstring& workingDirectory)
    {
        std::wstring cmdLine = L"\"" + exePath + L"\" " + arguments;

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };

        std::vector<wchar_t> cmdLineBuffer(cmdLine.begin(), cmdLine.end());
        cmdLineBuffer.push_back(L'\0');

        const wchar_t* pWorkDir = workingDirectory.empty() ? nullptr : workingDirectory.c_str();

        BOOL success = CreateProcessW(
            exePath.c_str(),
            cmdLineBuffer.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            pWorkDir,
            &si,
            &pi
        );

        if (success)
        {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return true;
        }
        return false;
    }
}
