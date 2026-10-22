#include "SevenZipHelper.h"
#include "resource.h"
#include <shlwapi.h>
#include <fstream>
#include <cstdint>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")

HMODULE g_hModule = nullptr;

namespace SevenZipHelper
{
    static SRWLOCK s_pathLock = SRWLOCK_INIT;
    static std::wstring s_sevenZipDir;
    static std::wstring s_7zGPath;
    static std::wstring s_7zFMPath;
    static std::wstring s_iconPath;
    static bool s_pathsInitialized = false;

    static SRWLOCK s_selLock = SRWLOCK_INIT;
    static SelectionInfo s_cachedSelection;
    static ULONGLONG s_lastSelectionTick = 0;
    static IShellItemArray* s_lastSelectionPtr = nullptr;

    // Fast file existence check using GetFileAttributesW
    static inline bool FastFileExists(const wchar_t* path) noexcept
    {
        DWORD attr = GetFileAttributesW(path);
        return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
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
                while (!outPath.empty() && (outPath.back() == L'\\' || outPath.back() == L'/'))
                {
                    outPath.pop_back();
                }
                return !outPath.empty();
            }
            RegCloseKey(hKey);
        }
        return false;
    }

    static void SaveRegistryPath(const std::wstring& path)
    {
        HKEY hKey = nullptr;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\7-Zip", 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS)
        {
            DWORD bytes = static_cast<DWORD>((path.size() + 1) * sizeof(wchar_t));
            RegSetValueExW(hKey, L"Path", 0, REG_SZ, reinterpret_cast<const BYTE*>(path.c_str()), bytes);
            RegCloseKey(hKey);
        }
    }

    static std::wstring DiscoverSevenZipDirectory(bool forceRescan)
    {
        std::wstring path;
        if (!forceRescan && ReadRegistryPath(HKEY_CURRENT_USER, L"Software\\7-Zip", path))
        {
            if (FastFileExists((path + L"\\7zG.exe").c_str()))
            {
                return path;
            }
        }

        // Fast check default 64-bit path
        if (FastFileExists(L"C:\\Program Files\\7-Zip\\7zG.exe"))
        {
            return L"C:\\Program Files\\7-Zip";
        }

        if (ReadRegistryPath(HKEY_LOCAL_MACHINE, L"SOFTWARE\\7-Zip", path) && FastFileExists((path + L"\\7zG.exe").c_str()))
        {
            SaveRegistryPath(path);
            return path;
        }
        if (ReadRegistryPath(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\7-Zip", path) && FastFileExists((path + L"\\7zG.exe").c_str()))
        {
            SaveRegistryPath(path);
            return path;
        }
        if (FastFileExists(L"C:\\Program Files (x86)\\7-Zip\\7zG.exe"))
        {
            SaveRegistryPath(L"C:\\Program Files (x86)\\7-Zip");
            return L"C:\\Program Files (x86)\\7-Zip";
        }

        return L"C:\\Program Files\\7-Zip";
    }

    static void EnsurePathsInitialized(bool forceRescan = false)
    {
        AcquireSRWLockExclusive(&s_pathLock);
        if (!s_pathsInitialized || forceRescan)
        {
            s_sevenZipDir = DiscoverSevenZipDirectory(forceRescan);
            s_7zGPath = s_sevenZipDir + L"\\7zG.exe";
            s_7zFMPath = s_sevenZipDir + L"\\7zFM.exe";

            wchar_t dllPath[MAX_PATH] = { 0 };
            if (g_hModule && GetModuleFileNameW(g_hModule, dllPath, MAX_PATH) > 0)
            {
                // Reference the icon resource embedded directly inside SevenZipContextMenu.dll (already in RAM)
                s_iconPath = std::wstring(dllPath) + L",-" + std::to_wstring(IDI_SEVENZIP);
            }
            else
            {
                s_iconPath = s_7zFMPath + L",0";
            }
            s_pathsInitialized = true;
        }
        ReleaseSRWLockExclusive(&s_pathLock);
    }

    void WarmupCache()
    {
        EnsurePathsInitialized(false);

        AcquireSRWLockExclusive(&s_selLock);
        if (s_cachedSelection.baseName.empty())
        {
            s_cachedSelection.baseName = L"Archive";
            s_cachedSelection.titleExtractTo = L"Extract to \"Archive\\\"";
            s_cachedSelection.titleAdd7z = L"Add to \"Archive.7z\"";
            s_cachedSelection.titleAddZip = L"Add to \"Archive.zip\"";
            s_cachedSelection.titleEmail7z = L"Compress to \"Archive.7z\" and email";
            s_cachedSelection.titleEmailZip = L"Compress to \"Archive.zip\" and email";
        }
        ReleaseSRWLockExclusive(&s_selLock);
    }

    void RefreshSevenZipPaths()
    {
        EnsurePathsInitialized(true);
    }

    const std::wstring& GetSevenZipDirectory()
    {
        if (!s_pathsInitialized) EnsurePathsInitialized(false);
        return s_sevenZipDir;
    }

    const std::wstring& Get7zGPath()
    {
        if (!s_pathsInitialized) EnsurePathsInitialized(false);
        return s_7zGPath;
    }

    const std::wstring& Get7zFMPath()
    {
        if (!s_pathsInitialized) EnsurePathsInitialized(false);
        return s_7zFMPath;
    }

    const std::wstring& GetRootIconPath()
    {
        if (!s_pathsInitialized) EnsurePathsInitialized(false);
        return s_iconPath;
    }

    // Compile-time 64-bit extension packer (up to 8 ASCII chars after the dot)
    static constexpr uint64_t PackExtConst(const char* str) noexcept
    {
        uint64_t val = 0;
        for (int i = 0; str[i] != '\0' && i < 8; ++i)
        {
            unsigned char c = static_cast<unsigned char>(str[i]);
            if (c >= 'A' && c <= 'Z') c |= 0x20;
            val = (val << 8) | c;
        }
        return val;
    }

    static inline uint64_t PackExtRuntime(const wchar_t* extWithoutDot) noexcept
    {
        uint64_t val = 0;
        for (int i = 0; extWithoutDot[i] != L'\0'; ++i)
        {
            if (i >= 8) return 0;
            wchar_t wc = extWithoutDot[i];
            if (wc >= 0x80) return 0;
            unsigned char c = static_cast<unsigned char>(wc);
            if (c >= 'A' && c <= 'Z') c |= 0x20;
            val = (val << 8) | c;
        }
        return val;
    }

    bool IsArchiveFile(const std::wstring& path) noexcept
    {
        if (path.empty()) return false;

        size_t lastDot = path.find_last_of(L'.');
        if (lastDot == std::wstring::npos || lastDot + 1 >= path.size()) return false;

        // O(1) compiled jump-table check on the packed 64-bit extension integer
        switch (PackExtRuntime(path.c_str() + lastDot + 1))
        {
        case PackExtConst("7z"):
        case PackExtConst("zip"):
        case PackExtConst("rar"):
        case PackExtConst("tar"):
        case PackExtConst("gz"):
        case PackExtConst("tgz"):
        case PackExtConst("bz2"):
        case PackExtConst("tbz2"):
        case PackExtConst("tbz"):
        case PackExtConst("xz"):
        case PackExtConst("txz"):
        case PackExtConst("iso"):
        case PackExtConst("wim"):
        case PackExtConst("cab"):
        case PackExtConst("arj"):
        case PackExtConst("lzh"):
        case PackExtConst("lha"):
        case PackExtConst("z"):
        case PackExtConst("taz"):
        case PackExtConst("cpio"):
        case PackExtConst("rpm"):
        case PackExtConst("deb"):
        case PackExtConst("dmg"):
        case PackExtConst("hfs"):
        case PackExtConst("xar"):
        case PackExtConst("vhd"):
        case PackExtConst("vhdx"):
        case PackExtConst("fat"):
        case PackExtConst("ntfs"):
        case PackExtConst("squashfs"):
        case PackExtConst("qcow"):
        case PackExtConst("vmdk"):
        case PackExtConst("vdi"):
        case PackExtConst("001"):
        case PackExtConst("z01"):
        case PackExtConst("zipx"):
        case PackExtConst("jar"):
        case PackExtConst("war"):
        case PackExtConst("ear"):
        case PackExtConst("apk"):
        case PackExtConst("ipa"):
            return true;
        default:
            return false;
        }
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

        // Check compound extensions (.tar.gz, .tar.bz2, .tar.xz, .tar.z)
        if (lastDot > 4)
        {
            size_t prevDot = fileName.find_last_of(L'.', lastDot - 1);
            if (prevDot != std::wstring::npos && prevDot > 0 && (lastDot - prevDot) == 4)
            {
                const wchar_t* p = fileName.c_str() + prevDot + 1;
                if ((p[0] == L't' || p[0] == L'T') &&
                    (p[1] == L'a' || p[1] == L'A') &&
                    (p[2] == L'r' || p[2] == L'R'))
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

    static void PopulateSelectionInfo(IShellItemArray* psiItemArray, SelectionInfo& info)
    {
        info = SelectionInfo{};

        if (psiItemArray && SUCCEEDED(psiItemArray->GetCount(&info.count)) && info.count > 0)
        {
            IShellItem* pItem = nullptr;
            if (SUCCEEDED(psiItemArray->GetItemAt(0, &pItem)) && pItem)
            {
                LPWSTR pszPath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)) && pszPath)
                {
                    info.firstItemPath = pszPath;
                    CoTaskMemFree(pszPath);
                }
                pItem->Release();
            }
        }

        if (!info.firstItemPath.empty())
        {
            info.isArchive = IsArchiveFile(info.firstItemPath);
            if (info.count == 1)
            {
                info.baseName = GetFileNameWithoutExtension(info.firstItemPath);
            }
            else
            {
                std::wstring parentDir = GetParentDirectory(info.firstItemPath);
                info.baseName = GetFileName(parentDir);
            }
        }

        if (info.baseName.empty())
        {
            info.baseName = L"Archive";
        }

        info.titleExtractTo = L"Extract to \"" + info.baseName + L"\\\"";
        info.titleAdd7z     = L"Add to \"" + info.baseName + L".7z\"";
        info.titleAddZip    = L"Add to \"" + info.baseName + L".zip\"";
        info.titleEmail7z   = L"Compress to \"" + info.baseName + L".7z\" and email";
        info.titleEmailZip   = L"Compress to \"" + info.baseName + L".zip\" and email";
    }

    void UpdateSelectionCache(IShellItemArray* psiItemArray)
    {
        if (!psiItemArray) return;

        SelectionInfo freshInfo;
        PopulateSelectionInfo(psiItemArray, freshInfo);

        AcquireSRWLockExclusive(&s_selLock);
        s_cachedSelection = std::move(freshInfo);
        s_lastSelectionPtr = psiItemArray;
        s_lastSelectionTick = GetTickCount64();
        ReleaseSRWLockExclusive(&s_selLock);
    }

    SelectionInfo GetCachedSelectionInfo(IShellItemArray* psiItemArray)
    {
        ULONGLONG now = GetTickCount64();

        AcquireSRWLockShared(&s_selLock);
        // Fast path: if updated by RootCommand::GetState within the last 500ms or matching pointer, return immediately with ZERO COM calls
        if (s_lastSelectionTick != 0 &&
            (psiItemArray == nullptr || psiItemArray == s_lastSelectionPtr || (now - s_lastSelectionTick) < 500))
        {
            SelectionInfo copy = s_cachedSelection;
            ReleaseSRWLockShared(&s_selLock);
            return copy;
        }
        ReleaseSRWLockShared(&s_selLock);

        if (psiItemArray)
        {
            UpdateSelectionCache(psiItemArray);
        }

        AcquireSRWLockShared(&s_selLock);
        SelectionInfo copy = s_cachedSelection;
        ReleaseSRWLockShared(&s_selLock);
        return copy;
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

    static bool TrySpawn(const std::wstring& exePath, const std::wstring& arguments, const std::wstring& workingDirectory)
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

    bool LaunchProcess(const std::wstring& exePath, const std::wstring& arguments, const std::wstring& workingDirectory)
    {
        if (TrySpawn(exePath, arguments, workingDirectory))
        {
            return true;
        }

        // Auto-heal: if 7-Zip was reinstalled to a different drive/folder, re-discover and retry once
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
        {
            bool isFM = (exePath.find(L"7zFM.exe") != std::wstring::npos);
            RefreshSevenZipPaths();
            const std::wstring& updatedExe = isFM ? Get7zFMPath() : Get7zGPath();
            return TrySpawn(updatedExe, arguments, workingDirectory);
        }
        return false;
    }
}
