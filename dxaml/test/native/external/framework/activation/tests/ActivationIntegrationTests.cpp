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
    decltype(RoGetActivationFactory)* realRoGetActivationFactory = RoGetActivationFactory;
    decltype(LoadLibraryW)* realLoadLibraryW = LoadLibraryW;
    decltype(LoadLibraryExW)* realLoadLibraryExW = LoadLibraryExW;
    thread_local bool probingActivation = false;
    unsigned int unregisteredActivationCalls = 0;
    unsigned int activationLoadCalls = 0;
    DWORD activationLoadFlags = 0;

    HRESULT WINAPI GetActivationFactoryWithoutRegistration(HSTRING classId, REFIID iid, void** factory)
    {
        if (probingActivation &&
            wcscmp(WindowsGetStringRawBuffer(classId, nullptr), L"Microsoft.Web.WebView2.Core.CoreWebView2EnvironmentOptions") == 0)
        {
            ++unregisteredActivationCalls;
            *factory = nullptr;
            return REGDB_E_CLASSNOTREG;
        }
        return realRoGetActivationFactory(classId, iid, factory);
    }

    HMODULE WINAPI ObserveActivationLoad(PCWSTR libraryName, HANDLE file, DWORD flags)
    {
        if (probingActivation && libraryName && _wcsicmp(libraryName, L"Microsoft.Web.WebView2.Core.dll") == 0)
        {
            ++activationLoadCalls;
            activationLoadFlags = flags;
            // Stop after observing the request; no Core DLL or browser is needed.
            SetLastError(ERROR_MOD_NOT_FOUND);
            return nullptr;
        }
        return realLoadLibraryExW(libraryName, file, flags);
    }

    HMODULE WINAPI ObserveLegacyActivationLoad(PCWSTR libraryName)
    {
        if (probingActivation && libraryName && _wcsicmp(libraryName, L"Microsoft.Web.WebView2.Core.dll") == 0)
        {
            return ObserveActivationLoad(libraryName, nullptr, 0);
        }
        return realLoadLibraryW(libraryName);
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
            RunOnUIThread([&]()
            {
                auto webView = ref new WebView2();
                auto closeWebView = wil::scope_exit([&]() { webView->Close(); });
                unregisteredActivationCalls = 0;
                activationLoadCalls = 0;
                activationLoadFlags = 0;
                probingActivation = true;
                auto stopProbing = wil::scope_exit([&]() { probingActivation = false; });

                // Hook the implementation exports, including callers using API-set imports.
                const auto loader = GetModuleHandleW(L"kernelbase.dll");
                VERIFY_IS_NOT_NULL(loader);
                realLoadLibraryW = reinterpret_cast<decltype(realLoadLibraryW)>(GetProcAddress(loader, "LoadLibraryW"));
                realLoadLibraryExW = reinterpret_cast<decltype(realLoadLibraryExW)>(GetProcAddress(loader, "LoadLibraryExW"));
                VERIFY_IS_NOT_NULL(realLoadLibraryW);
                VERIFY_IS_NOT_NULL(realLoadLibraryExW);

                auto updateDetours = [](bool attach)
                {
                    VERIFY_ARE_EQUAL(NO_ERROR, DetourTransactionBegin());
                    auto abortTransaction = wil::scope_exit([]() { DetourTransactionAbort(); });
                    VERIFY_ARE_EQUAL(NO_ERROR, DetourUpdateThread(GetCurrentThread()));
                    auto update = attach ? DetourAttach : DetourDetach;
                    VERIFY_ARE_EQUAL(NO_ERROR, update(reinterpret_cast<PVOID*>(&realRoGetActivationFactory), GetActivationFactoryWithoutRegistration));
                    VERIFY_ARE_EQUAL(NO_ERROR, update(reinterpret_cast<PVOID*>(&realLoadLibraryW), ObserveLegacyActivationLoad));
                    VERIFY_ARE_EQUAL(NO_ERROR, update(reinterpret_cast<PVOID*>(&realLoadLibraryExW), ObserveActivationLoad));
                    VERIFY_ARE_EQUAL(NO_ERROR, DetourTransactionCommit());
                    abortTransaction.release();
                };
                updateDetours(true);
                auto detachDetours = wil::scope_exit([&]() { updateDetours(false); });

                // Options activation enters MUXC's real fallback after the forced registration failure.
                auto initialization = webView->EnsureCoreWebView2Async();
                VERIFY_ARE_EQUAL(AsyncStatus::Error, initialization->Status);
                VERIFY_ARE_EQUAL(REGDB_E_CLASSNOTREG, initialization->ErrorCode.Value);

                VERIFY_ARE_EQUAL(1u, unregisteredActivationCalls);
                VERIFY_ARE_EQUAL(1u, activationLoadCalls);
                VERIFY_ARE_EQUAL(static_cast<DWORD>(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS), activationLoadFlags);
            });
        }

} } } } } }
