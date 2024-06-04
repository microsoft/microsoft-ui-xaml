// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <DependencyLocator.h>
#include <XamlTypeInfo.h>
#include "XamlExtTraceLogging.h"

#ifndef QUOTE__
#define TO_STRING__(s) L#s
#define QUOTE__(s) TO_STRING__(s)
#endif

extern XHANDLE ghHeap;

namespace Private
{
    PROVIDE_DEPENDENCY(XamlRuntimeType);

    const wchar_t * c_moduleName = QUOTE__(WUX_EXT_FULLNAME__);
}

BOOL InitializeExtDll();
void DeinitializeExtDll();

// Force the linker to work with WRL module activation
#include "CreatorMap.inl"

HINSTANCE g_hInstance;

#ifndef QUOTEA__
#define TO_STRINGA__(s) #s
#define QUOTEA__(s) TO_STRINGA__(s)
#endif

// Trace logging provider GUID {A9813670-EED8-4E92-A2FF-6583C6734C93}
TRACELOGGING_DEFINE_PROVIDER(g_hExtTraceProvider,
    QUOTEA__(WUX_EXT_NAME__),
    (0xa9813670, 0xeed8, 0x4e92, 0xa2, 0xff, 0x65, 0x83, 0xc6, 0x73, 0x4c, 0x93),
    TraceLoggingOptionMicrosoftTelemetry());

extern "C"
HRESULT WINAPI
DllGetActivationFactory(
    _In_ HSTRING activatibleClassId,
    _Outptr_ IActivationFactory** factory)
{
    auto &module = wrl::Module<wrl::InProc>::GetModule();
    return module.GetActivationFactory(activatibleClassId, factory);
}

_Check_return_ STDAPI
DllGetClassObject(
    _In_ REFCLSID rclsid,
    _In_ REFIID riid,
    _Outptr_ LPVOID *ppv)
{
    auto &module = wrl::Module<wrl::InProc>::GetModule();
    return module.GetClassObject(rclsid, riid, ppv);
}

__control_entrypoint(DllExport)
STDAPI
DllCanUnloadNow()
{
    const auto &module = wrl::Module<wrl::InProc>::GetModule();
    return module.GetObjectCount() == 0 ? S_OK : S_FALSE;
}

extern "C"
BOOL WINAPI
DllMain(
    _In_opt_ HINSTANCE hInstance,
    DWORD dwReason,
    _In_opt_ LPVOID /* lpReserved */)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            {
                DisableThreadLibraryCalls(hInstance);
                g_hInstance = hInstance;

                // We'll initialize the error context object at this point in order to make sure
                // that all failures in this module are properly captured and logged.
                // We need to retrieve the error context TLS index from Microsoft.UI.Xaml.dll
                // in order to make sure we're storing errors in the same context,
                // which ensures that we retain the full callstack for errors.
                const DWORD TLS_UNINITIALIZED = static_cast<DWORD>(-1);

                HMODULE xamlModule = GetModuleHandle(L"Microsoft.UI.Xaml.dll");
                if (xamlModule)
                {
                    typedef DWORD (__cdecl* GetErrorContextIndexFuncPtr)();
                    auto pfnGetErrorContextIndex = reinterpret_cast<GetErrorContextIndexFuncPtr>(GetProcAddress(xamlModule, "GetErrorContextIndex"));

                    if (pfnGetErrorContextIndex)
                    {
                        DWORD errorContextIndex = pfnGetErrorContextIndex();
                        if (errorContextIndex != TLS_UNINITIALIZED)
                        {
                            VERIFYHR(ErrorContextGlobalInit(errorContextIndex));
                        }
                    }

                    typedef DWORD(__cdecl* GetWarningContextIndexFuncPtr)();
                    auto pfnGetWarningContextIndex = reinterpret_cast<GetWarningContextIndexFuncPtr>(GetProcAddress(xamlModule, "GetWarningContextIndex"));

                    if (pfnGetWarningContextIndex)
                    {
                        DWORD warningContextIndex = pfnGetWarningContextIndex();
                        if (warningContextIndex != TLS_UNINITIALIZED)
                        {
                            VERIFYHR(WarningContextGlobalInit(warningContextIndex));
                        }
                    }

                    // Ensure that the extension DLL is using the same heap as the framework.
                    typedef XHANDLE(__cdecl* GetHeapHandlePtr)();
                    auto pfnGetHeapHandle = reinterpret_cast<GetHeapHandlePtr>(GetProcAddress(xamlModule, "GetHeapHandle"));

                    if (pfnGetHeapHandle)
                    {
                        ghHeap = pfnGetHeapHandle();
                        ASSERT(ghHeap);
                    }
                }

                // Register trace logging provider
                TraceLoggingRegister(g_hExtTraceProvider);

                // May be different for each extension
                InitializeExtDll();
            }
            break;

        case DLL_PROCESS_DETACH:
            DeinitializeExtDll();
            TraceLoggingUnregister(g_hExtTraceProvider);

            VERIFYHR(ErrorContextGlobalDeinit());
            VERIFYHR(WarningContextGlobalDeinit());
            break;

        case DLL_THREAD_DETACH:
            VERIFYHR(ErrorContextThreadDeinit());
            VERIFYHR(WarningContextThreadDeinit());
            break;
    }

    return TRUE;
}

// Need it to export services
extern "C" __declspec(dllexport) DependencyLocator::Internal::ILocalDependencyStorage* __cdecl GetDependencyLocatorStorage()
{
    return &(DependencyLocator::Internal::GetDependencyLocatorStorage().Get());
}
