// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlDiagnosticsTap.h"

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
#endif

extern "C"
void __cdecl __security_init_cookie(void);

#pragma region Dll Helpers

extern "C" 
BOOL WINAPI
DllMain(_In_ HINSTANCE hinstDLL, _In_ unsigned int fdwReason, _In_opt_ void *)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, _COM_Outptr_ void** ppv)
{
    return wrl::Module<wrl::InProc>::GetModule().GetClassObject(rclsid, riid, ppv);
}

STDAPI
DllCanUnloadNow()
{
    return wrl::Module<wrl::InProc>::GetModule().Terminate() ? S_OK : S_FALSE;
}
