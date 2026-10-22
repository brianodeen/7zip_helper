#include <windows.h>
#include <objbase.h>
#include "Guid.h"
#include "ClassFactory.h"
#include "SevenZipHelper.h"

extern HMODULE g_hModule;
long g_dllRefCount = 1; // Keep >= 1 so DllCanUnloadNow never unloads the cached DLL

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);

        // Pin the DLL in memory so CoFreeUnusedLibraries never unloads it
        HMODULE hPinned = nullptr;
        GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            reinterpret_cast<LPCWSTR>(hModule),
            &hPinned
        );

        SevenZipHelper::WarmupCache();
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (!ppv) return E_POINTER;
    *ppv = nullptr;

    if (IsEqualCLSID(rclsid, CLSID_SevenZipContextMenu))
    {
        static ClassFactory s_factory;
        static bool s_serverLocked = false;
        if (!s_serverLocked)
        {
            s_serverLocked = true;
            // Lock the COM surrogate process (dllhost.exe) in memory so it stays warm
            // in the background instead of shutting down after the context menu closes.
            CoAddRefServerProcess();
            CoLockObjectExternal( static_cast<IClassFactory*>(&s_factory), TRUE, FALSE );
        }
        return s_factory.QueryInterface(riid, ppv);
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow()
{
    // Always stay resident in memory (~70 KB) for instant 0ms response on subsequent right-clicks
    return S_FALSE;
}

STDAPI DllRegisterServer()
{
    return S_OK;
}

STDAPI DllUnregisterServer()
{
    return S_OK;
}
