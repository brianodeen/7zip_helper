#include "ClassFactory.h"
#include "SevenZipCommand.h"
#include <shlwapi.h>

extern long g_dllRefCount;

ClassFactory::ClassFactory()
    : m_refCount(1)
{
    InterlockedIncrement(&g_dllRefCount);
}

ClassFactory::~ClassFactory()
{
    InterlockedDecrement(&g_dllRefCount);
}

IFACEMETHODIMP ClassFactory::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = {
        QITABENT(ClassFactory, IClassFactory),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) ClassFactory::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG) ClassFactory::Release()
{
    long ref = InterlockedDecrement(&m_refCount);
    if (ref == 0)
    {
        delete this;
    }
    return ref;
}

IFACEMETHODIMP ClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv)
{
    if (!ppv) return E_POINTER;
    *ppv = nullptr;

    if (pUnkOuter != nullptr)
    {
        return CLASS_E_NOAGGREGATION;
    }

    SevenZipRootCommand* pCommand = new SevenZipRootCommand();
    if (!pCommand)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pCommand->QueryInterface(riid, ppv);
    pCommand->Release();
    return hr;
}

IFACEMETHODIMP ClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
    {
        InterlockedIncrement(&g_dllRefCount);
    }
    else
    {
        InterlockedDecrement(&g_dllRefCount);
    }
    return S_OK;
}
