// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RuntimeProfiler.h"
#include "WinUIrc.ver"
#include <winstring.h>
#include <initguid.h>
#include <wrl\module.h>
#include <roapi.h>
#include <atomic>
#include "LifetimeHandler.h"

using namespace Microsoft::WRL;

#include "MuxcTraceLogging.h"


HINSTANCE g_hInstance = nullptr;

//  Version of the binary, used to distinguish between versions of types
const char *gFileVersion = (const char *)(VER_FILEVERSION_STR);

STDAPI_(void) SendTelemetryOnSuspend();

// ---------------------------------------------------------------------------
// Activation factory bypass for MUX types
//
// When MUXC (this DLL) activates a type from MUX (Microsoft.ui.xaml.dll),
// C++/WinRT normally calls RoGetActivationFactory, which does a catalog
// lookup, walks the package graph, finds the DLL, and calls its
// DllGetActivationFactory.  That's expensive!  MUX is already loaded
// in-process, so we can call its DllGetActivationFactory directly.
//
// C++/WinRT checks the global 'winrt_activation_handler' function pointer
// BEFORE calling RoGetActivationFactory.  We set it during DLL_PROCESS_ATTACH
// to intercept Microsoft.UI.Xaml.* type activations and route them straight
// to MUX.
// ---------------------------------------------------------------------------

namespace {

using DllGetActivationFactory_t = HRESULT(WINAPI*)(HSTRING, IActivationFactory**);
std::atomic<DllGetActivationFactory_t> s_muxGetFactory{ nullptr };
std::atomic<bool> s_muxFactoryResolved{ false };

DllGetActivationFactory_t GetMuxActivationFactoryFn()
{
    if (!s_muxFactoryResolved.load(std::memory_order_acquire))
    {
        // Find MUX (Microsoft.ui.xaml.dll) -- it should be in the same
        // directory as MUXC.  We look up by module name first (more
        // reliable than full-path matching), then verify the path is
        // next to us.  If it's a different copy of MUX (multiple WinUI
        // versions side-by-side), we skip the bypass and let
        // RoGetActivationFactory do the right thing.
        const wchar_t* failureReason = nullptr;

        // Build the expected sibling path: <MUXC dir>\Microsoft.ui.xaml.dll
        wchar_t expectedPath[MAX_PATH]{};
        DWORD len = GetModuleFileNameW(g_hInstance, expectedPath, MAX_PATH);
        if (len == 0 || len >= MAX_PATH)
        {
            failureReason = L"GetModuleFileNameW failed or path too long";
        }
        else
        {
            wchar_t* lastSlash = wcsrchr(expectedPath, L'\\');
            if (!lastSlash)
            {
                failureReason = L"No backslash in MUXC path";
            }
            else
            {
                *(lastSlash + 1) = L'\0';
                wcscat_s(expectedPath, L"Microsoft.ui.xaml.dll");

                // Find MUX by module name, then check it's the right one.
                HMODULE mod = GetModuleHandleW(L"Microsoft.ui.xaml.dll");
                if (!mod)
                {
                    failureReason = L"MUX DLL not loaded";
                }
                else
                {
                    wchar_t actualPath[MAX_PATH]{};
                    DWORD actualLen = GetModuleFileNameW(mod, actualPath, MAX_PATH);
                    if (actualLen == 0 || actualLen >= MAX_PATH ||
                        _wcsicmp(actualPath, expectedPath) != 0)
                    {
                        failureReason = L"MUX DLL path mismatch";
                    }
                    else
                    {
                        auto fn = reinterpret_cast<DllGetActivationFactory_t>(
                            GetProcAddress(mod, "DllGetActivationFactory"));
                        if (!fn)
                        {
                            failureReason = L"DllGetActivationFactory export not found";
                        }
                        s_muxGetFactory.store(fn, std::memory_order_relaxed);
                    }
                }
            }
        }

        // One-shot trace so we can tell from an ETL whether the bypass is active.
        TraceLoggingWrite(
            g_hLoggingProvider,
            "MuxActivationBypass",
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingValue(failureReason == nullptr, "IsActive"),
            TraceLoggingWideString(failureReason ? failureReason : L"", "FailureReason"));

        s_muxFactoryResolved.store(true, std::memory_order_release);
    }
    return s_muxGetFactory.load(std::memory_order_relaxed);
}

int32_t __stdcall MuxcActivationHandler(
    void* classId,
    winrt::guid const& iid,
    void** factory) noexcept
{
    *factory = nullptr;

    HSTRING hstr = static_cast<HSTRING>(classId);
    uint32_t len = 0;
    const wchar_t* buf = WindowsGetStringRawBuffer(hstr, &len);
    std::wstring_view name{ buf, len };

    if (name.starts_with(L"Microsoft.UI.Xaml."))
    {
        auto muxGetFactory = GetMuxActivationFactoryFn();
        if (muxGetFactory)
        {
            ComPtr<IActivationFactory> af;
            HRESULT hr = muxGetFactory(hstr, af.GetAddressOf());
            if (SUCCEEDED(hr) && af)
            {
                hr = af->QueryInterface(
                    reinterpret_cast<const IID&>(iid), factory);
                return hr;
            }
            // MUX didn't handle it -- fall through to RoGetActivationFactory.
        }
    }

    // Rest of this method copies cppwinrt behavior here (see base.h):
    HRESULT hr = RoGetActivationFactory(
        hstr, reinterpret_cast<const IID&>(iid), factory);
    if (hr == CO_E_NOTINITIALIZED)
    {
        // increment the MTA usage count and retry the activation.
        CO_MTA_USAGE_COOKIE cookie{};
        std::ignore = CoIncrementMTAUsage(&cookie);
        hr = RoGetActivationFactory(
            hstr, reinterpret_cast<const IID&>(iid), factory);
    }

    if (SUCCEEDED(hr))
    {
        return hr;
    }

    // Mirror base.h's DLL-name fallback: strip trailing components from the
    // type name to form candidate DLL names (e.g. "Foo.Bar.Baz" tries
    // "Foo.Bar.dll", "Foo.dll") and call each one's DllGetActivationFactory
    // directly.  This is required on Win10 in unpackaged/test
    // scenarios where RoGetActivationFactory fails if we are unable to
    // use RoGAF and CoIncrementMTAUsage, but the DLL is loadable.
    std::wstring path{ buf, len };
    std::size_t dotPos{};

    while (std::wstring::npos != (dotPos = path.rfind(L'.')))
    {
        path.resize(dotPos);
        path += L".dll";
        HMODULE hModule = LoadLibraryW(path.c_str());
        path.resize(path.size() - 4);

        if (!hModule)
        {
            continue;
        }

        auto getFactoryMethod = reinterpret_cast<HRESULT(WINAPI*)(HSTRING, IActivationFactory**)>(
            GetProcAddress(hModule, "DllGetActivationFactory"));

        if (!getFactoryMethod)
        {
            FreeLibrary(hModule);
            continue;
        }

        ComPtr<IActivationFactory> activationFactory;
        if (FAILED(getFactoryMethod(hstr, activationFactory.GetAddressOf())) || !activationFactory)
        {
            FreeLibrary(hModule);
            continue;
        }

        // QI for the requested interface.
        hr = activationFactory->QueryInterface(
            reinterpret_cast<const IID&>(iid), factory);
        if (SUCCEEDED(hr))
        {
            return hr;
        }
        else
        {
            FreeLibrary(hModule);
        }
    }

    return hr;
}

} // anonymous namespace

STDAPI_(BOOL) DllMain(_In_ HINSTANCE hInstance, _In_ DWORD reason, _In_opt_ void *)
{
    if (DLL_PROCESS_ATTACH == reason)
    {
        g_hInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);
        RegisterTraceLogging();
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

        // Route MUX type activations directly to MUX, skipping RoGetActivationFactory.
        // Cpp/WinRT will call our winrt_activation_handler when activating types for this DLL.
        winrt_activation_handler = MuxcActivationHandler;
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

