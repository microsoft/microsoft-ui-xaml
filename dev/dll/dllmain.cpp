// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TraceLogging.h"
#include <initguid.h>
#include <wrl\module.h>

using namespace Microsoft::WRL;

HINSTANCE g_hInstance = NULL;

#if defined(DBG) && defined(BUILD_WINDOWS)
// defined in:
// xcp\components\allocation\
// which we link to with:
// Windows.UI.Xaml.Allocation.lib
void InitCheckedMemoryChainLock();
#endif

STDAPI_(void) SendTelemetryOnSuspend();

STDAPI_(BOOL) DllMain(_In_ HINSTANCE hInstance, _In_ DWORD reason, _In_opt_ void *)
{
    if (DLL_PROCESS_ATTACH == reason)
    {
    #if defined(DBG) && defined(BUILD_WINDOWS)
        // Initialize the debug allocator.
        InitCheckedMemoryChainLock();
    #endif
        
        g_hInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);
        RegisterTraceLogging();
    }
    else if (DLL_PROCESS_DETACH == reason)
    {
        SendTelemetryOnSuspend();
        UnRegisterTraceLogging();
    }

    return TRUE;
}

STDAPI DllGetActivationFactory(_In_ HSTRING activatibleClassId, _COM_Outptr_ IActivationFactory **factory)
{
    return Module<InProc>::GetModule().GetActivationFactory(activatibleClassId, factory);
}

__control_entrypoint(DllExport)
STDAPI DllCanUnloadNow()
{
    if (winrt::get_module_lock())
    {
        return S_FALSE;
    }

    if (!Module<InProc>::GetModule().Terminate())
    {
        return S_FALSE;
    }

    winrt::clear_factory_cache();

    return S_OK;
}

_Check_return_
STDAPI  DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv) 
{
    return Module<InProc>::GetModule().GetClassObject(rclsid, riid, ppv);
}

// Microsoft.UI.Xaml.def includes this as an export, but it only applies to WUXC.
// We'll stub it out for MUX to avoid the build error we get otherwise.
#ifndef BUILD_WINDOWS
extern "C" void XamlTestHookFreeControlsResourceLibrary() { }
#endif
