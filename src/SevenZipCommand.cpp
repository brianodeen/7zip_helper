#include "SevenZipCommand.h"
#include "SevenZipHelper.h"
#include "Guid.h"
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

extern long g_dllRefCount;

// Fast in-place COM string duplication
static inline HRESULT AllocString(const wchar_t* str, LPWSTR* ppsz) noexcept
{
    if (!ppsz) return E_POINTER;
    return SHStrDupW(str, ppsz);
}

static inline HRESULT AllocString(const std::wstring& str, LPWSTR* ppsz) noexcept
{
    if (!ppsz) return E_POINTER;
    return SHStrDupW(str.c_str(), ppsz);
}

// Pre-allocated immortal subcommand table (zero heap allocations on menu open)
static constexpr size_t kSubCommandCount = 16;
static SevenZipSubCommand s_subCommands[kSubCommandCount] = {
    SevenZipSubCommand(ACTION_OPEN, true),
    SevenZipSubCommand(ACTION_EXTRACT_DIALOG, true),
    SevenZipSubCommand(ACTION_EXTRACT_HERE, true),
    SevenZipSubCommand(ACTION_EXTRACT_TO_FOLDER, true),
    SevenZipSubCommand(ACTION_TEST, true),
    SevenZipSubCommand(ACTION_SEP_1, true),
    SevenZipSubCommand(ACTION_ADD_DIALOG, true),
    SevenZipSubCommand(ACTION_COMPRESS_EMAIL_DIALOG, true),
    SevenZipSubCommand(ACTION_ADD_7Z, true),
    SevenZipSubCommand(ACTION_ADD_ZIP, true),
    SevenZipSubCommand(ACTION_COMPRESS_7Z_EMAIL, true),
    SevenZipSubCommand(ACTION_COMPRESS_ZIP_EMAIL, true),
    SevenZipSubCommand(ACTION_SEP_2, true),
    SevenZipSubCommand(ACTION_CRC_SHA_ALL, true),
    SevenZipSubCommand(ACTION_CRC_SHA_256, true),
    SevenZipSubCommand(ACTION_CRC_32, true)
};

// ----------------------------------------------------------------------------
// SevenZipSubCommand Implementation
// ----------------------------------------------------------------------------

SevenZipSubCommand::SevenZipSubCommand(SevenZipActionType action, bool isStaticSingleton)
    : m_refCount(1), m_action(action), m_isStaticSingleton(isStaticSingleton), m_pUnkMarshaler(nullptr)
{
    if (!m_isStaticSingleton)
    {
        InterlockedIncrement(&g_dllRefCount);
    }
}

SevenZipSubCommand::~SevenZipSubCommand()
{
    if (m_pUnkMarshaler)
    {
        m_pUnkMarshaler->Release();
        m_pUnkMarshaler = nullptr;
    }
    if (!m_isStaticSingleton)
    {
        InterlockedDecrement(&g_dllRefCount);
    }
}

IFACEMETHODIMP SevenZipSubCommand::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_POINTER;

    if (IsEqualIID(riid, IID_IExplorerCommand) || IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, __uuidof(IAgileObject)))
    {
        *ppv = static_cast<IExplorerCommand*>(this);
        AddRef();
        return S_OK;
    }

    if (IsEqualIID(riid, IID_IMarshal))
    {
        if (!m_pUnkMarshaler)
        {
            IUnknown* pMarshaler = nullptr;
            if (SUCCEEDED(CoCreateFreeThreadedMarshaler(static_cast<IExplorerCommand*>(this), &pMarshaler)))
            {
                if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&m_pUnkMarshaler), pMarshaler, nullptr) != nullptr)
                {
                    pMarshaler->Release();
                }
            }
        }
        if (m_pUnkMarshaler)
        {
            return m_pUnkMarshaler->QueryInterface(riid, ppv);
        }
    }

    *ppv = nullptr;
    return E_NOINTERFACE;
}

IFACEMETHODIMP_(ULONG) SevenZipSubCommand::AddRef()
{
    if (m_isStaticSingleton) return 2;
    return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG) SevenZipSubCommand::Release()
{
    if (m_isStaticSingleton) return 1;
    long ref = InterlockedDecrement(&m_refCount);
    if (ref == 0)
    {
        delete this;
    }
    return ref;
}

IFACEMETHODIMP SevenZipSubCommand::GetTitle(IShellItemArray* psiItemArray, LPWSTR* ppszName)
{
    switch (m_action)
    {
    case ACTION_OPEN:
        return AllocString(L"Open archive", ppszName);
    case ACTION_EXTRACT_DIALOG:
        return AllocString(L"Extract files...", ppszName);
    case ACTION_EXTRACT_HERE:
        return AllocString(L"Extract Here", ppszName);
    case ACTION_EXTRACT_TO_FOLDER:
        return AllocString(SevenZipHelper::GetCachedSelectionInfo(psiItemArray).titleExtractTo, ppszName);
    case ACTION_TEST:
        return AllocString(L"Test archive", ppszName);
    case ACTION_ADD_DIALOG:
        return AllocString(L"Add to archive...", ppszName);
    case ACTION_COMPRESS_EMAIL_DIALOG:
        return AllocString(L"Compress and email...", ppszName);
    case ACTION_ADD_7Z:
        return AllocString(SevenZipHelper::GetCachedSelectionInfo(psiItemArray).titleAdd7z, ppszName);
    case ACTION_ADD_ZIP:
        return AllocString(SevenZipHelper::GetCachedSelectionInfo(psiItemArray).titleAddZip, ppszName);
    case ACTION_COMPRESS_7Z_EMAIL:
        return AllocString(SevenZipHelper::GetCachedSelectionInfo(psiItemArray).titleEmail7z, ppszName);
    case ACTION_COMPRESS_ZIP_EMAIL:
        return AllocString(SevenZipHelper::GetCachedSelectionInfo(psiItemArray).titleEmailZip, ppszName);
    case ACTION_CRC_SHA_ALL:
        return AllocString(L"CRC SHA (*)", ppszName);
    case ACTION_CRC_SHA_256:
        return AllocString(L"CRC SHA (SHA-256)", ppszName);
    case ACTION_CRC_32:
        return AllocString(L"CRC SHA (CRC-32)", ppszName);
    default:
        return AllocString(L"", ppszName);
    }
}

IFACEMETHODIMP SevenZipSubCommand::GetIcon(IShellItemArray* /*psiItemArray*/, LPWSTR* ppszIcon)
{
    if (ppszIcon) *ppszIcon = nullptr;
    return E_NOTIMPL;
}

IFACEMETHODIMP SevenZipSubCommand::GetToolTip(IShellItemArray* /*psiItemArray*/, LPWSTR* ppszInfo)
{
    if (ppszInfo) *ppszInfo = nullptr;
    return E_NOTIMPL;
}

IFACEMETHODIMP SevenZipSubCommand::GetCanonicalName(GUID* pguidCommandName)
{
    if (!pguidCommandName) return E_POINTER;
    *pguidCommandName = CLSID_SevenZipContextMenu;
    pguidCommandName->Data1 += static_cast<unsigned long>(m_action);
    return S_OK;
}

IFACEMETHODIMP SevenZipSubCommand::GetState(IShellItemArray* psiItemArray, BOOL /*fOkToBeSlow*/, EXPCMDSTATE* pCmdState)
{
    if (!pCmdState) return E_POINTER;

    switch (m_action)
    {
    case ACTION_OPEN:
    case ACTION_EXTRACT_DIALOG:
    case ACTION_EXTRACT_HERE:
    case ACTION_EXTRACT_TO_FOLDER:
    case ACTION_TEST:
    case ACTION_SEP_1:
    {
        bool isArchive = SevenZipHelper::GetCachedSelectionInfo(psiItemArray).isArchive;
        *pCmdState = isArchive ? ECS_ENABLED : ECS_HIDDEN;
        return S_OK;
    }
    default:
        *pCmdState = ECS_ENABLED;
        return S_OK;
    }
}

IFACEMETHODIMP SevenZipSubCommand::Invoke(IShellItemArray* psiItemArray, IBindCtx* /*pbc*/)
{
    std::vector<std::wstring> paths = SevenZipHelper::GetPathsFromShellItemArray(psiItemArray);
    if (paths.empty())
    {
        return S_OK;
    }

    const std::wstring& z7G = SevenZipHelper::Get7zGPath();
    const std::wstring& z7FM = SevenZipHelper::Get7zFMPath();
    std::wstring parentDir = SevenZipHelper::GetParentDirectory(paths[0]);
    std::wstring baseName = (paths.size() == 1) ?
        SevenZipHelper::GetFileNameWithoutExtension(paths[0]) :
        SevenZipHelper::GetFileName(parentDir);
    if (baseName.empty()) baseName = L"Archive";

    std::wstring fileArgs = SevenZipHelper::BuildFileArgumentList(paths);

    switch (m_action)
    {
    case ACTION_OPEN:
        SevenZipHelper::LaunchProcess(z7FM, L"\"" + paths[0] + L"\"", parentDir);
        break;

    case ACTION_EXTRACT_DIALOG:
        SevenZipHelper::LaunchProcess(z7G, L"x -ad \"" + paths[0] + L"\"", parentDir);
        break;

    case ACTION_EXTRACT_HERE:
        SevenZipHelper::LaunchProcess(z7G, L"x -spe \"" + paths[0] + L"\" -o\"" + parentDir + L"\"", parentDir);
        break;

    case ACTION_EXTRACT_TO_FOLDER:
    {
        std::wstring destFolder = parentDir + L"\\" + baseName;
        SevenZipHelper::LaunchProcess(z7G, L"x -spe \"" + paths[0] + L"\" -o\"" + destFolder + L"\"", parentDir);
        break;
    }

    case ACTION_TEST:
        SevenZipHelper::LaunchProcess(z7G, L"t \"" + paths[0] + L"\"", parentDir);
        break;

    case ACTION_ADD_DIALOG:
    {
        std::wstring targetArchive = parentDir + L"\\" + baseName + L".7z";
        SevenZipHelper::LaunchProcess(z7G, L"a -ad \"" + targetArchive + L"\" " + fileArgs, parentDir);
        break;
    }

    case ACTION_COMPRESS_EMAIL_DIALOG:
    {
        std::wstring targetArchive = parentDir + L"\\" + baseName + L".7z";
        SevenZipHelper::LaunchProcess(z7G, L"a -ad -seml. \"" + targetArchive + L"\" " + fileArgs, parentDir);
        break;
    }

    case ACTION_ADD_7Z:
    {
        std::wstring targetArchive = parentDir + L"\\" + baseName + L".7z";
        SevenZipHelper::LaunchProcess(z7G, L"a \"" + targetArchive + L"\" " + fileArgs, parentDir);
        break;
    }

    case ACTION_ADD_ZIP:
    {
        std::wstring targetArchive = parentDir + L"\\" + baseName + L".zip";
        SevenZipHelper::LaunchProcess(z7G, L"a -tzip \"" + targetArchive + L"\" " + fileArgs, parentDir);
        break;
    }

    case ACTION_COMPRESS_7Z_EMAIL:
    {
        std::wstring targetArchive = parentDir + L"\\" + baseName + L".7z";
        SevenZipHelper::LaunchProcess(z7G, L"a -seml. \"" + targetArchive + L"\" " + fileArgs, parentDir);
        break;
    }

    case ACTION_COMPRESS_ZIP_EMAIL:
    {
        std::wstring targetArchive = parentDir + L"\\" + baseName + L".zip";
        SevenZipHelper::LaunchProcess(z7G, L"a -tzip -seml. \"" + targetArchive + L"\" " + fileArgs, parentDir);
        break;
    }

    case ACTION_CRC_SHA_ALL:
        SevenZipHelper::LaunchProcess(z7G, L"h -scrc* " + fileArgs, parentDir);
        break;

    case ACTION_CRC_SHA_256:
        SevenZipHelper::LaunchProcess(z7G, L"h -scrcSHA256 " + fileArgs, parentDir);
        break;

    case ACTION_CRC_32:
        SevenZipHelper::LaunchProcess(z7G, L"h -scrcCRC32 " + fileArgs, parentDir);
        break;

    default:
        break;
    }

    return S_OK;
}

IFACEMETHODIMP SevenZipSubCommand::GetFlags(EXPCMDFLAGS* pFlags)
{
    if (!pFlags) return E_POINTER;
    *pFlags = (m_action == ACTION_SEP_1 || m_action == ACTION_SEP_2) ? ECF_ISSEPARATOR : ECF_DEFAULT;
    return S_OK;
}

IFACEMETHODIMP SevenZipSubCommand::EnumSubCommands(IEnumExplorerCommand** ppEnum)
{
    if (!ppEnum) return E_POINTER;
    *ppEnum = nullptr;
    return E_NOTIMPL;
}

// ----------------------------------------------------------------------------
// SevenZipEnumCommands Implementation
// ----------------------------------------------------------------------------

SevenZipEnumCommands::SevenZipEnumCommands(size_t startIndex)
    : m_refCount(1), m_currentIndex(startIndex), m_pUnkMarshaler(nullptr)
{
    InterlockedIncrement(&g_dllRefCount);
    CoCreateFreeThreadedMarshaler(static_cast<IEnumExplorerCommand*>(this), &m_pUnkMarshaler);
}

SevenZipEnumCommands::~SevenZipEnumCommands()
{
    if (m_pUnkMarshaler)
    {
        m_pUnkMarshaler->Release();
        m_pUnkMarshaler = nullptr;
    }
    InterlockedDecrement(&g_dllRefCount);
}

IFACEMETHODIMP SevenZipEnumCommands::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_POINTER;

    if (IsEqualIID(riid, IID_IEnumExplorerCommand) || IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, __uuidof(IAgileObject)))
    {
        *ppv = static_cast<IEnumExplorerCommand*>(this);
        AddRef();
        return S_OK;
    }

    if (IsEqualIID(riid, IID_IMarshal) && m_pUnkMarshaler)
    {
        return m_pUnkMarshaler->QueryInterface(riid, ppv);
    }

    *ppv = nullptr;
    return E_NOINTERFACE;
}

IFACEMETHODIMP_(ULONG) SevenZipEnumCommands::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG) SevenZipEnumCommands::Release()
{
    long ref = InterlockedDecrement(&m_refCount);
    if (ref == 0)
    {
        delete this;
    }
    return ref;
}

IFACEMETHODIMP SevenZipEnumCommands::Next(ULONG celt, IExplorerCommand** pUICommand, ULONG* pceltFetched)
{
    if (pceltFetched)
    {
        *pceltFetched = 0;
    }

    if (!pUICommand || celt == 0)
    {
        return E_POINTER;
    }

    ULONG fetched = 0;
    while (m_currentIndex < kSubCommandCount && fetched < celt)
    {
        pUICommand[fetched] = &s_subCommands[m_currentIndex];
        fetched++;
        m_currentIndex++;
    }

    if (pceltFetched)
    {
        *pceltFetched = fetched;
    }

    return (fetched == celt) ? S_OK : S_FALSE;
}

IFACEMETHODIMP SevenZipEnumCommands::Skip(ULONG celt)
{
    m_currentIndex += celt;
    if (m_currentIndex > kSubCommandCount)
    {
        m_currentIndex = kSubCommandCount;
        return S_FALSE;
    }
    return S_OK;
}

IFACEMETHODIMP SevenZipEnumCommands::Reset()
{
    m_currentIndex = 0;
    return S_OK;
}

IFACEMETHODIMP SevenZipEnumCommands::Clone(IEnumExplorerCommand** ppEnum)
{
    if (!ppEnum) return E_POINTER;
    *ppEnum = new SevenZipEnumCommands(m_currentIndex);
    return S_OK;
}

// ----------------------------------------------------------------------------
// SevenZipRootCommand Implementation
// ----------------------------------------------------------------------------

SevenZipRootCommand::SevenZipRootCommand()
    : m_refCount(1), m_pUnkMarshaler(nullptr)
{
}

SevenZipRootCommand::~SevenZipRootCommand()
{
    if (m_pUnkMarshaler)
    {
        m_pUnkMarshaler->Release();
        m_pUnkMarshaler = nullptr;
    }
}

IFACEMETHODIMP SevenZipRootCommand::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_POINTER;

    if (IsEqualIID(riid, IID_IExplorerCommand) || IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, __uuidof(IAgileObject)))
    {
        *ppv = static_cast<IExplorerCommand*>(this);
        AddRef();
        return S_OK;
    }

    if (IsEqualIID(riid, IID_IMarshal))
    {
        if (!m_pUnkMarshaler)
        {
            IUnknown* pMarshaler = nullptr;
            if (SUCCEEDED(CoCreateFreeThreadedMarshaler(static_cast<IExplorerCommand*>(this), &pMarshaler)))
            {
                if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&m_pUnkMarshaler), pMarshaler, nullptr) != nullptr)
                {
                    pMarshaler->Release();
                }
            }
        }
        if (m_pUnkMarshaler)
        {
            return m_pUnkMarshaler->QueryInterface(riid, ppv);
        }
    }

    *ppv = nullptr;
    return E_NOINTERFACE;
}

IFACEMETHODIMP_(ULONG) SevenZipRootCommand::AddRef()
{
    return 2;
}

IFACEMETHODIMP_(ULONG) SevenZipRootCommand::Release()
{
    return 1;
}

IFACEMETHODIMP SevenZipRootCommand::GetTitle(IShellItemArray* /*psiItemArray*/, LPWSTR* ppszName)
{
    return AllocString(L"7-Zip", ppszName);
}

IFACEMETHODIMP SevenZipRootCommand::GetIcon(IShellItemArray* /*psiItemArray*/, LPWSTR* ppszIcon)
{
    return AllocString(SevenZipHelper::GetRootIconPath(), ppszIcon);
}

IFACEMETHODIMP SevenZipRootCommand::GetToolTip(IShellItemArray* /*psiItemArray*/, LPWSTR* ppszInfo)
{
    if (ppszInfo) *ppszInfo = nullptr;
    return E_NOTIMPL;
}

IFACEMETHODIMP SevenZipRootCommand::GetCanonicalName(GUID* pguidCommandName)
{
    if (!pguidCommandName) return E_POINTER;
    *pguidCommandName = CLSID_SevenZipContextMenu;
    return S_OK;
}

IFACEMETHODIMP SevenZipRootCommand::GetState(IShellItemArray* psiItemArray, BOOL /*fOkToBeSlow*/, EXPCMDSTATE* pCmdState)
{
    if (!pCmdState) return E_POINTER;

    // Single-pass selection probe: inspect item 0 once here so all 16 subcommands do 0 COM calls
    if (psiItemArray)
    {
        SevenZipHelper::UpdateSelectionCache(psiItemArray);
    }

    *pCmdState = ECS_ENABLED;
    return S_OK;
}

IFACEMETHODIMP SevenZipRootCommand::Invoke(IShellItemArray* psiItemArray, IBindCtx* /*pbc*/)
{
    std::vector<std::wstring> paths = SevenZipHelper::GetPathsFromShellItemArray(psiItemArray);
    const std::wstring& z7FM = SevenZipHelper::Get7zFMPath();
    std::wstring workDir = paths.empty() ? L"" : SevenZipHelper::GetParentDirectory(paths[0]);
    std::wstring arg = paths.empty() ? L"" : L"\"" + paths[0] + L"\"";
    SevenZipHelper::LaunchProcess(z7FM, arg, workDir);
    return S_OK;
}

IFACEMETHODIMP SevenZipRootCommand::GetFlags(EXPCMDFLAGS* pFlags)
{
    if (!pFlags) return E_POINTER;
    *pFlags = ECF_HASSUBCOMMANDS;
    return S_OK;
}

IFACEMETHODIMP SevenZipRootCommand::EnumSubCommands(IEnumExplorerCommand** ppEnum)
{
    if (!ppEnum) return E_POINTER;
    *ppEnum = new SevenZipEnumCommands(0);
    return S_OK;
}
