#include "SevenZipHelper.h"
#include <shlwapi.h>
#include <fstream>
#include <algorithm>
#include <sstream>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")

namespace SevenZipHelper
{
    static std::wstring ToLower(std::wstring str)
    {
        std::transform(str.begin(), str.end(), str.begin(), ::towlower);
        return str;
    }

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

    std::wstring GetSevenZipDirectory()
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

        // Standard default paths
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

    std::wstring Get7zGPath()
    {
        return GetSevenZipDirectory() + L"\\7zG.exe";
    }

    std::wstring Get7zFMPath()
    {
        return GetSevenZipDirectory() + L"\\7zFM.exe";
    }

    bool IsArchiveFile(const std::wstring& path)
    {
        if (path.empty() || PathIsDirectoryW(path.c_str()))
        {
            return false;
        }

        std::wstring lower = ToLower(path);
        const wchar_t* extensions[] = {
            L".7z", L".zip", L".rar", L".tar", L".gz", L".tgz", L".bz2", L".tbz2", L".tbz",
            L".xz", L".txz", L".iso", L".wim", L".cab", L".arj", L".lzh", L".lha", L".z",
            L".taz", L".cpio", L".rpm", L".deb", L".dmg", L".hfs", L".xar", L".vhd", L".vhdx",
            L".fat", L".ntfs", L".squashfs", L".qcow", L".vmdk", L".vdi", L".001", L".z01", L".zipx",
            L".jar", L".war", L".ear", L".apk", L".ipa"
        };

        for (const auto& ext : extensions)
        {
            size_t extLen = wcslen(ext);
            if (lower.length() >= extLen && lower.compare(lower.length() - extLen, extLen, ext) == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool AnyIsArchive(const std::vector<std::wstring>& paths)
    {
        for (const auto& p : paths)
        {
            if (IsArchiveFile(p))
            {
                return true;
            }
        }
        return false;
    }

    bool AllAreArchives(const std::vector<std::wstring>& paths)
    {
        if (paths.empty()) return false;
        for (const auto& p : paths)
        {
            if (!IsArchiveFile(p))
            {
                return false;
            }
        }
        return true;
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

        // Remove compound extensions like .tar.gz, .tar.bz2
        std::wstring lower = ToLower(fileName);
        const wchar_t* compoundExts[] = { L".tar.gz", L".tar.bz2", L".tar.xz", L".tar.z" };
        for (const auto& cExt : compoundExts)
        {
            size_t cLen = wcslen(cExt);
            if (lower.length() >= cLen && lower.compare(lower.length() - cLen, cLen, cExt) == 0)
            {
                return fileName.substr(0, fileName.length() - cLen);
            }
        }

        size_t dotPos = fileName.find_last_of(L'.');
        if (dotPos != std::wstring::npos && dotPos > 0)
        {
            return fileName.substr(0, dotPos);
        }
        return fileName;
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

    std::wstring GetDefaultArchiveBaseName(const std::vector<std::wstring>& paths)
    {
        if (paths.empty())
        {
            return L"Archive";
        }
        if (paths.size() == 1)
        {
            return GetFileNameWithoutExtension(paths[0]);
        }
        // Multiple items: use parent folder name
        std::wstring parentDir = GetParentDirectory(paths[0]);
        std::wstring parentName = GetFileName(parentDir);
        if (!parentName.empty())
        {
            return parentName;
        }
        return L"Archive";
    }

    std::wstring GetDefaultDestinationFolder(const std::vector<std::wstring>& paths)
    {
        if (paths.empty()) return L"";
        std::wstring parentDir = GetParentDirectory(paths[0]);
        std::wstring baseName = GetFileNameWithoutExtension(paths[0]);
        if (!parentDir.empty())
        {
            return parentDir + L"\\" + baseName;
        }
        return baseName;
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
                    // Write string without trailing null, then newline
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
