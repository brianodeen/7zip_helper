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
    if (!ppv) return E_POINTER;
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = static_cast<IClassFactory*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

IFACEMETHODIMP_(ULONG) ClassFactory::AddRef()
{
    return 2;
}

IFACEMETHODIMP_(ULONG) ClassFactory::Release()
{
    return 1;
}

IFACEMETHODIMP ClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv)
{
    if (!ppv) return E_POINTER;
    *ppv = nullptr;

    if (pUnkOuter != nullptr)
    {
        return CLASS_E_NOAGGREGATION;
    }

    static SevenZipRootCommand s_rootCommand;
    return s_rootCommand.QueryInterface(riid, ppv);
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
