#include "SevenZipCommand.h"
#include "SevenZipHelper.h"
#include "Guid.h"
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

// Helper to duplicate wide string to COM task memory
static inline HRESULT AllocString(const wchar_t* str, LPWSTR* ppsz)
{
    if (!ppsz) return E_POINTER;
    return SHStrDupW(str, ppsz);
}

static inline HRESULT AllocString(const std::wstring& str, LPWSTR* ppsz)
{
    if (!ppsz) return E_POINTER;
    return SHStrDupW(str.c_str(), ppsz);
}

// ----------------------------------------------------------------------------
// SevenZipSubCommand Implementation
// ----------------------------------------------------------------------------

SevenZipSubCommand::SevenZipSubCommand(SevenZipActionType action)
    : m_refCount(1), m_action(action)
{
}

SevenZipSubCommand::~SevenZipSubCommand()
{
}

IFACEMETHODIMP SevenZipSubCommand::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = {
        QITABENT(SevenZipSubCommand, IExplorerCommand),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) SevenZipSubCommand::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG) SevenZipSubCommand::Release()
{
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
    {
        SevenZipHelper::SelectionInfo info = SevenZipHelper::GetFastSelectionInfo(psiItemArray);
        return AllocString(L"Extract to \"" + info.baseName + L"\\\"", ppszName);
    }
    case ACTION_TEST:
        return AllocString(L"Test archive", ppszName);
    case ACTION_ADD_DIALOG:
        return AllocString(L"Add to archive...", ppszName);
    case ACTION_COMPRESS_EMAIL_DIALOG:
        return AllocString(L"Compress and email...", ppszName);
    case ACTION_ADD_7Z:
    {
        SevenZipHelper::SelectionInfo info = SevenZipHelper::GetFastSelectionInfo(psiItemArray);
        return AllocString(L"Add to \"" + info.baseName + L".7z\"", ppszName);
    }
    case ACTION_ADD_ZIP:
    {
        SevenZipHelper::SelectionInfo info = SevenZipHelper::GetFastSelectionInfo(psiItemArray);
        return AllocString(L"Add to \"" + info.baseName + L".zip\"", ppszName);
    }
    case ACTION_COMPRESS_7Z_EMAIL:
    {
        SevenZipHelper::SelectionInfo info = SevenZipHelper::GetFastSelectionInfo(psiItemArray);
        return AllocString(L"Compress to \"" + info.baseName + L".7z\" and email", ppszName);
    }
    case ACTION_COMPRESS_ZIP_EMAIL:
    {
        SevenZipHelper::SelectionInfo info = SevenZipHelper::GetFastSelectionInfo(psiItemArray);
        return AllocString(L"Compress to \"" + info.baseName + L".zip\" and email", ppszName);
    }
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

IFACEMETHODIMP SevenZipSubCommand::GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon)
{
    if (ppszIcon) *ppszIcon = nullptr;
    return E_NOTIMPL;
}

IFACEMETHODIMP SevenZipSubCommand::GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfo)
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

IFACEMETHODIMP SevenZipSubCommand::GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState)
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
        SevenZipHelper::SelectionInfo info = SevenZipHelper::GetFastSelectionInfo(psiItemArray);
        *pCmdState = info.isArchive ? ECS_ENABLED : ECS_HIDDEN;
        return S_OK;
    }
    case ACTION_SEP_2:
    case ACTION_ADD_DIALOG:
    case ACTION_COMPRESS_EMAIL_DIALOG:
    case ACTION_ADD_7Z:
    case ACTION_ADD_ZIP:
    case ACTION_COMPRESS_7Z_EMAIL:
    case ACTION_COMPRESS_ZIP_EMAIL:
    case ACTION_CRC_SHA_ALL:
    case ACTION_CRC_SHA_256:
    case ACTION_CRC_32:
    default:
        *pCmdState = ECS_ENABLED;
        return S_OK;
    }
}

IFACEMETHODIMP SevenZipSubCommand::Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc)
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
    if (m_action == ACTION_SEP_1 || m_action == ACTION_SEP_2)
    {
        *pFlags = ECF_ISSEPARATOR;
    }
    else
    {
        *pFlags = ECF_DEFAULT;
    }
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

SevenZipEnumCommands::SevenZipEnumCommands(const std::vector<SevenZipActionType>& actions)
    : m_refCount(1), m_actions(actions), m_currentIndex(0)
{
}

SevenZipEnumCommands::~SevenZipEnumCommands()
{
}

IFACEMETHODIMP SevenZipEnumCommands::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = {
        QITABENT(SevenZipEnumCommands, IEnumExplorerCommand),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
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
    while (m_currentIndex < m_actions.size() && fetched < celt)
    {
        SevenZipSubCommand* pCmd = new SevenZipSubCommand(m_actions[m_currentIndex]);
        pUICommand[fetched] = pCmd;
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
    if (m_currentIndex > m_actions.size())
    {
        m_currentIndex = m_actions.size();
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
    SevenZipEnumCommands* pClone = new SevenZipEnumCommands(m_actions);
    pClone->m_currentIndex = m_currentIndex;
    *ppEnum = pClone;
    return S_OK;
}

// ----------------------------------------------------------------------------
// SevenZipRootCommand Implementation
// ----------------------------------------------------------------------------

SevenZipRootCommand::SevenZipRootCommand()
    : m_refCount(1)
{
}

SevenZipRootCommand::~SevenZipRootCommand()
{
}

IFACEMETHODIMP SevenZipRootCommand::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = {
        QITABENT(SevenZipRootCommand, IExplorerCommand),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) SevenZipRootCommand::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG) SevenZipRootCommand::Release()
{
    long ref = InterlockedDecrement(&m_refCount);
    if (ref == 0)
    {
        delete this;
    }
    return ref;
}

IFACEMETHODIMP SevenZipRootCommand::GetTitle(IShellItemArray* psiItemArray, LPWSTR* ppszName)
{
    return AllocString(L"7-Zip", ppszName);
}

IFACEMETHODIMP SevenZipRootCommand::GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon)
{
    return AllocString(SevenZipHelper::GetRootIconPath(), ppszIcon);
}

IFACEMETHODIMP SevenZipRootCommand::GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfo)
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

IFACEMETHODIMP SevenZipRootCommand::GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState)
{
    if (!pCmdState) return E_POINTER;
    *pCmdState = ECS_ENABLED;
    return S_OK;
}

IFACEMETHODIMP SevenZipRootCommand::Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc)
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

    static const std::vector<SevenZipActionType> s_actions = {
        ACTION_OPEN,
        ACTION_EXTRACT_DIALOG,
        ACTION_EXTRACT_HERE,
        ACTION_EXTRACT_TO_FOLDER,
        ACTION_TEST,
        ACTION_SEP_1,
        ACTION_ADD_DIALOG,
        ACTION_COMPRESS_EMAIL_DIALOG,
        ACTION_ADD_7Z,
        ACTION_ADD_ZIP,
        ACTION_COMPRESS_7Z_EMAIL,
        ACTION_COMPRESS_ZIP_EMAIL,
        ACTION_SEP_2,
        ACTION_CRC_SHA_ALL,
        ACTION_CRC_SHA_256,
        ACTION_CRC_32
    };

    *ppEnum = new SevenZipEnumCommands(s_actions);
    return S_OK;
}
