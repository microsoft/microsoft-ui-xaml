// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <ppltasks.h>
#include <roapi.h>
#include <detours.h>
#include <wil/resource.h>
#include "ActivationIntegrationTests.h"

using namespace Platform;
using namespace Concurrency;
using namespace Microsoft::WRL;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;

using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace std;

namespace
{
    // MUXC's activation fallback turns a class name into a DLL name by its namespace.
    // For this class it tries to load Microsoft.Web.WebView2.Core.dll.
    constexpr PCWSTR webView2OptionsClassName = L"Microsoft.Web.WebView2.Core.CoreWebView2EnvironmentOptions";
    constexpr PCWSTR webView2CoreDllName = L"Microsoft.Web.WebView2.Core.dll";

    // Detours notes:
    // * DetourAttach redirects every call to the real function into our hook.
    // * It also updates the "real" pointer so our hook can still call the original.
    decltype(RoGetActivationFactory)* realRoGetActivationFactory = RoGetActivationFactory;
    decltype(LoadLibraryExW)* realLoadLibraryExW = nullptr;

    // The hooks only change behavior on the test's thread, and only while this is set.
    thread_local bool isHookActive = false;

    // What the fallback did when it tried to load the Core DLL.
    unsigned int coreLoadCount = 0;
    DWORD coreLoadFlags = 0;

    // Hook 1: Make WinRT activation of the options class fail with "class not registered".
    // The test host's manifest registers WebView2's classes, so normal activation would succeed
    // and MUXC would never reach its DLL-name fallback.
    HRESULT WINAPI RoGetActivationFactoryHook(HSTRING classId, REFIID iid, void** factory)
    {
        if (isHookActive && wcscmp(WindowsGetStringRawBuffer(classId, nullptr), webView2OptionsClassName) == 0)
        {
            *factory = nullptr;
            return REGDB_E_CLASSNOTREG;
        }
        return realRoGetActivationFactory(classId, iid, factory);
    }

    // Hook 2: Record how the fallback asks for the Core DLL, then report "not found".
    // We only care about the search flags, so there's no need to really load it.
    // KernelBase's LoadLibraryW calls LoadLibraryExW with flags 0, so this catches both.
    HMODULE WINAPI LoadLibraryExWHook(PCWSTR fileName, HANDLE file, DWORD flags)
    {
        if (isHookActive && fileName && _wcsicmp(fileName, webView2CoreDllName) == 0)
        {
            ++coreLoadCount;
            coreLoadFlags = flags;
            SetLastError(ERROR_MOD_NOT_FOUND);
            return nullptr;
        }
        return realLoadLibraryExW(fileName, file, flags);
    }

    void AttachHooks(bool attach)
    {
        VERIFY_ARE_EQUAL(NO_ERROR, DetourTransactionBegin());
        VERIFY_ARE_EQUAL(NO_ERROR, DetourUpdateThread(GetCurrentThread()));
        auto update = attach ? DetourAttach : DetourDetach;
        VERIFY_ARE_EQUAL(NO_ERROR, update(reinterpret_cast<PVOID*>(&realRoGetActivationFactory), RoGetActivationFactoryHook));
        VERIFY_ARE_EQUAL(NO_ERROR, update(reinterpret_cast<PVOID*>(&realLoadLibraryExW), LoadLibraryExWHook));
        VERIFY_ARE_EQUAL(NO_ERROR, DetourTransactionCommit());
    }
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Activation {

        bool ActivationIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        void ActivationIntegrationTests::CreateAFloatCollection()
        {
            RunOnUIThread([&]()
            {
                typedef HRESULT(WINAPI *FpnDllGetActivationFactory)(HSTRING, IActivationFactory**);

                HMODULE hModXAML;
                VERIFY_IS_TRUE(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"Microsoft.UI.Xaml.dll", &hModXAML) == TRUE, L"Unable to get the handle for Microsoft.UI.Xaml.dll.");

                FpnDllGetActivationFactory pfnGetActivationFactory = (FpnDllGetActivationFactory)GetProcAddress(hModXAML, "DllGetActivationFactory");
                VERIFY_IS_NOT_NULL(pfnGetActivationFactory);

                ComPtr<IActivationFactory> spActivationFactory;
                VERIFY_SUCCEEDED(pfnGetActivationFactory(Platform::StringReference(L"Microsoft.UI.Xaml.Media.FloatCollection").GetHSTRING(), &spActivationFactory));

                ComPtr<IInspectable> spFloatCollection;
                VERIFY_SUCCEEDED(spActivationFactory->ActivateInstance(&spFloatCollection));

                VERIFY_IS_NOT_NULL(spFloatCollection.Get());
            });
        }

        void ActivationIntegrationTests::CreateInputEventArgs()
        {
            RunOnUIThread([&]()
            {
                auto args = ref new xaml_input::ManipulationStartingRoutedEventArgs();
                args->Pivot = ref new xaml_input::ManipulationPivot();

                VERIFY_IS_NOT_NULL(ref new xaml_input::ManipulationStartedRoutedEventArgs());
                VERIFY_IS_NOT_NULL(ref new xaml_input::ManipulationCompletedRoutedEventArgs());
                VERIFY_IS_NOT_NULL(ref new xaml_input::TappedRoutedEventArgs());
                VERIFY_IS_NOT_NULL(ref new xaml_input::DoubleTappedRoutedEventArgs());
                VERIFY_IS_NOT_NULL(ref new xaml_input::HoldingRoutedEventArgs());
            });
        }

        void ActivationIntegrationTests::ActivationLibraryUsesCppWinRTSearchPolicy()
        {
            // MUXC replaces C++/WinRT's activation handler. When normal WinRT activation fails, C++/WinRT
            // loads a DLL named after the class's namespace with LOAD_LIBRARY_SEARCH_DEFAULT_DIRS, so
            // directories added with AddDllDirectory are searched. MUXC's fallback has to do the same.
            RunOnUIThread([&]()
            {
                auto webView = ref new WebView2();
                auto closeWebView = wil::scope_exit([&]() { webView->Close(); });

                // Hook the KernelBase export. Product code calls it through API sets,
                // which skip the kernel32 export the import table would give us.
                realLoadLibraryExW = reinterpret_cast<decltype(LoadLibraryExW)*>(
                    GetProcAddress(GetModuleHandleW(L"kernelbase.dll"), "LoadLibraryExW"));
                VERIFY_IS_NOT_NULL(realLoadLibraryExW);

                coreLoadCount = 0;
                coreLoadFlags = 0;
                isHookActive = true;
                AttachHooks(true);
                auto detachHooks = wil::scope_exit([&]()
                {
                    isHookActive = false;
                    AttachHooks(false);
                });

                // This activates the WebView2 options class, which hook 1 makes fail.
                // MUXC then falls back to loading the Core DLL, which hook 2 records.
                webView->EnsureCoreWebView2Async();

                VERIFY_ARE_EQUAL(1u, coreLoadCount);
                VERIFY_ARE_EQUAL(static_cast<DWORD>(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS), coreLoadFlags);
            });
        }

} } } } } }
