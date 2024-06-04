// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DllInitializedHelpers.h"
#include "LocalizationHelpers.h"
#include <XamlTraceLogging.h>

#include <WinUIrc.ver>           //  To define VER_FILEVERSION_STR

// version of binary, used in telemetry payload so that we may distinguish between versions.
const char *gFileVersion = (const char *)(VER_FILEVERSION_STR);

//  from RuntimeProfiler.cpp
STDAPI_(void) SendTelemetryOnSuspend();

extern "C"
void
XamlTestHookFreePhoneResourceLibrary()
{
    Private::XamlTestHookFreeResourceLibrary();
}

BOOL InitializeExtDll()
{
    // Register trace logging provider
    TraceLoggingRegister(g_hTraceProvider);

    // Keeping track of the loaded status of the Dll as some components' destructor may need that info.
    Private::SetIsDllInitialized(true);

    return (TRUE);
}

void DeinitializeExtDll()
{
    // Fire any outstanding telemetry before unregistering the provider
    SendTelemetryOnSuspend();

    // Unregister trace logging provider
    TraceLoggingUnregister(g_hTraceProvider);

    // Keeping track of the loaded status of the Dll as some components' destructor may need that info.
    Private::SetIsDllInitialized(false);
}
