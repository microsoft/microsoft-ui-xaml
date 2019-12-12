// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TraceLogging.h"

HINSTANCE g_hInstance = nullptr;

STDAPI_(void) SendTelemetryOnSuspend();

STDAPI_(BOOL) DllMain(_In_ HINSTANCE hInstance, _In_ DWORD reason, _In_opt_ void *)
{
    if (DLL_PROCESS_ATTACH == reason)
    {
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

HRESULT WINAPI DllGetActivationFactory(_In_ HSTRING activatableClassId, _Out_ ::IActivationFactory** factory)
{
    return WINRT_GetActivationFactory(activatableClassId, reinterpret_cast<void**>(factory));
}

__control_entrypoint(DllExport)
HRESULT __stdcall DllCanUnloadNow()
{
    return WINRT_CanUnloadNow();
}
