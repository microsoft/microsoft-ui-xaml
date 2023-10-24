// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RuntimeProfiler.h"
#include "WinUIrc.ver"
#include <winstring.h>
#include <initguid.h>
#include <wrl\module.h>
#include "LifetimeHandler.h"

using namespace Microsoft::WRL;

#ifndef MUX_EXPERIMENTAL
#include "TraceLogging.h"
#endif

HINSTANCE g_hInstance = nullptr;

//  Version of the binary, used to distinguish between versions of types
const char *gFileVersion = (const char *)(VER_FILEVERSION_STR);

STDAPI_(void) SendTelemetryOnSuspend();

STDAPI_(BOOL) DllMain(_In_ HINSTANCE hInstance, _In_ DWORD reason, _In_opt_ void *)
{
    if (DLL_PROCESS_ATTACH == reason)
    {
        g_hInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);
#ifndef MUX_EXPERIMENTAL
        RegisterTraceLogging();
#endif

        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    }
    else if (DLL_PROCESS_DETACH == reason)
    {
#ifndef MUX_EXPERIMENTAL
        SendTelemetryOnSuspend();
        UnRegisterTraceLogging();
#endif
    }

    return TRUE;
}

HRESULT WINAPI DllGetActivationFactory(_In_ HSTRING activatableClassId, _Out_ ::IActivationFactory** factory)
{
    // This "FrameworkPackageDetector" class is a breadcrumb to help us detect if we're running in a framework package.
    // The way this works is that our WinMD does not contain this type so attempting to activate it will fail. However,
    // our framework package AppX contains an activatable class registration for it, so code that tries to activate
    // it will succeed in that context.
    uint32_t length{};
    wchar_t const* const buffer = WindowsGetStringRawBuffer(activatableClassId, &length);
    std::wstring_view const name{ buffer, length };
    if (name == L"Microsoft.UI.Private.Controls.FrameworkPackageDetector"sv ||
        name == L"Microsoft.UI.Private.Controls.CBSPackageDetector"sv)
    {
        winrt::hstring resources{L"Microsoft.UI.Xaml.Controls.XamlControlsResources"sv};
        // It doesn't matter *what* we return so return a type that everyone uses.
        return WINRT_GetActivationFactory(winrt::get_abi(resources), reinterpret_cast<void**>(factory));
    }

    return WINRT_GetActivationFactory(activatableClassId, reinterpret_cast<void**>(factory));
}

__control_entrypoint(DllExport)
HRESULT __stdcall DllCanUnloadNow()
{
    return WINRT_CanUnloadNow();
}

// Needed to clear our list of dependency properties between XAML initializations.
STDAPI_(void) DeinitializeMUXC()
{
    XamlMetadataProvider::ClearTypes();
    MUXControlsFactory::Deinitialize();
    RuntimeProfiler::UninitializeRuntimeProfiler();
    LifetimeHandler::ClearMaterialHelperInstance();
}

