#pragma once
#include <windows.h>
#include <shobjidl_core.h>
#include <vector>
#include <string>

enum SevenZipActionType
{
    ACTION_ROOT = 0,
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

class SevenZipSubCommand : public IExplorerCommand
{
public:
    SevenZipSubCommand(SevenZipActionType action);
    virtual ~SevenZipSubCommand();

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    IFACEMETHODIMP_(ULONG) AddRef() override;
    IFACEMETHODIMP_(ULONG) Release() override;

    // IExplorerCommand
    IFACEMETHODIMP GetTitle(IShellItemArray* psiItemArray, LPWSTR* ppszName) override;
    IFACEMETHODIMP GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon) override;
    IFACEMETHODIMP GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfo) override;
    IFACEMETHODIMP GetCanonicalName(GUID* pguidCommandName) override;
    IFACEMETHODIMP GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState) override;
    IFACEMETHODIMP Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc) override;
    IFACEMETHODIMP GetFlags(EXPCMDFLAGS* pFlags) override;
    IFACEMETHODIMP EnumSubCommands(IEnumExplorerCommand** ppEnum) override;

private:
    long m_refCount;
    SevenZipActionType m_action;
};

class SevenZipEnumCommands : public IEnumExplorerCommand
{
public:
    SevenZipEnumCommands(const std::vector<SevenZipActionType>& actions);
    virtual ~SevenZipEnumCommands();

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    IFACEMETHODIMP_(ULONG) AddRef() override;
    IFACEMETHODIMP_(ULONG) Release() override;

    // IEnumExplorerCommand
    IFACEMETHODIMP Next(ULONG celt, IExplorerCommand** pUICommand, ULONG* pceltFetched) override;
    IFACEMETHODIMP Skip(ULONG celt) override;
    IFACEMETHODIMP Reset() override;
    IFACEMETHODIMP Clone(IEnumExplorerCommand** ppEnum) override;

private:
    long m_refCount;
    std::vector<SevenZipActionType> m_actions;
    size_t m_currentIndex;
};

class SevenZipRootCommand : public IExplorerCommand
{
public:
    SevenZipRootCommand();
    virtual ~SevenZipRootCommand();

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    IFACEMETHODIMP_(ULONG) AddRef() override;
    IFACEMETHODIMP_(ULONG) Release() override;

    // IExplorerCommand
    IFACEMETHODIMP GetTitle(IShellItemArray* psiItemArray, LPWSTR* ppszName) override;
    IFACEMETHODIMP GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon) override;
    IFACEMETHODIMP GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfo) override;
    IFACEMETHODIMP GetCanonicalName(GUID* pguidCommandName) override;
    IFACEMETHODIMP GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState) override;
    IFACEMETHODIMP Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc) override;
    IFACEMETHODIMP GetFlags(EXPCMDFLAGS* pFlags) override;
    IFACEMETHODIMP EnumSubCommands(IEnumExplorerCommand** ppEnum) override;

private:
    long m_refCount;
};
