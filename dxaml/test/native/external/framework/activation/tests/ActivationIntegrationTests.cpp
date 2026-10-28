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
#include <wil/stl.h>
#include <wil/win32_helpers.h>
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
    const wchar_t* const activationProbeLibrary = L"WinUIActivationProbe.dll";
    decltype(RoGetActivationFactory)* realRoGetActivationFactory = RoGetActivationFactory;
    decltype(LoadLibraryW)* realLoadLibraryW = LoadLibraryW;
    decltype(LoadLibraryExW)* realLoadLibraryExW = LoadLibraryExW;
    thread_local bool probingActivation = false;
    unsigned int unregisteredActivationCalls = 0;
    unsigned int activationProbeCalls = 0;
    DWORD activationProbeFlags = 0;
    wchar_t activationProbePath[MAX_PATH]{};

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

    HMODULE WINAPI LoadActivationProbe(PCWSTR libraryName, HANDLE file, DWORD flags)
    {
        if (probingActivation && libraryName && _wcsicmp(libraryName, L"Microsoft.Web.WebView2.Core.dll") == 0)
        {
            ++activationProbeCalls;
            activationProbeFlags = flags;
            // Avoid the test host's Core DLL manifest redirection, but use the production search flags.
            HMODULE module = realLoadLibraryExW(activationProbeLibrary, file, flags);
            if (module)
            {
                GetModuleFileNameW(module, activationProbePath, ARRAYSIZE(activationProbePath));
            }
            return module;
        }
        return realLoadLibraryExW(libraryName, file, flags);
    }

    HMODULE WINAPI LoadActivationProbeLegacy(PCWSTR libraryName)
    {
        if (probingActivation && libraryName && _wcsicmp(libraryName, L"Microsoft.Web.WebView2.Core.dll") == 0)
        {
            return LoadActivationProbe(libraryName, nullptr, 0);
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

        void ActivationIntegrationTests::ActivationLibrarySearchesAddedDirectories()
        {
            wchar_t temporaryPath[MAX_PATH]{};
            const auto temporaryLength = GetTempPathW(ARRAYSIZE(temporaryPath), temporaryPath);
            VERIFY_IS_TRUE(temporaryLength > 0 && temporaryLength < ARRAYSIZE(temporaryPath));
            wchar_t directory[MAX_PATH]{};
            VERIFY_IS_TRUE(GetTempFileNameW(temporaryPath, L"wva", 0, directory) != 0);
            VERIFY_IS_TRUE(DeleteFileW(directory));
            VERIFY_IS_TRUE(CreateDirectoryW(directory, nullptr));
            auto removeDirectory = wil::scope_exit([&]() { VERIFY_IS_TRUE(RemoveDirectoryW(directory)); });

            wil::unique_hmodule existing(LoadLibraryW(L"version.dll"));
            VERIFY_IS_NOT_NULL(existing.get());

            const std::wstring libraryPath = std::wstring(directory) + L"\\" + activationProbeLibrary;
            const auto sourcePath = wil::GetModuleFileNameW<std::wstring>(existing.get());
            VERIFY_IS_TRUE(CopyFileW(sourcePath.c_str(), libraryPath.c_str(), TRUE));
            auto removeCopy = wil::scope_exit([&]() { VERIFY_IS_TRUE(DeleteFileW(libraryPath.c_str())); });

            // This method has its own process. Exclude user directories from its default search
            // so the host's DLL policy cannot mask a missing explicit fallback.
            VERIFY_IS_TRUE(SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32));
            VERIFY_IS_NULL(GetModuleHandleW(activationProbeLibrary));
            const auto cookie = AddDllDirectory(directory);
            VERIFY_IS_NOT_NULL(cookie);
            auto removeCookie = wil::scope_exit([&]() { VERIFY_IS_TRUE(RemoveDllDirectory(cookie)); });
            wil::unique_hmodule primary(LoadLibraryW(activationProbeLibrary));
            VERIFY_IS_NULL(primary.get());

            RunOnUIThread([&]()
            {
                auto webView = ref new WebView2();
                auto closeWebView = wil::scope_exit([&]() { webView->Close(); });
                unregisteredActivationCalls = 0;
                activationProbeCalls = 0;
                activationProbeFlags = 0;
                activationProbePath[0] = L'\0';
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
                    VERIFY_ARE_EQUAL(NO_ERROR, update(reinterpret_cast<PVOID*>(&realLoadLibraryW), LoadActivationProbeLegacy));
                    VERIFY_ARE_EQUAL(NO_ERROR, update(reinterpret_cast<PVOID*>(&realLoadLibraryExW), LoadActivationProbe));
                    VERIFY_ARE_EQUAL(NO_ERROR, DetourTransactionCommit());
                    abortTransaction.release();
                };
                updateDetours(true);
                auto detachDetours = wil::scope_exit([&]() { updateDetours(false); });

                // Options activation enters MUXC's real fallback. The probe DLL has no
                // activation export, so initialization stops without starting a browser.
                webView->EnsureCoreWebView2Async();
            });

            VERIFY_ARE_EQUAL(1u, unregisteredActivationCalls);
            VERIFY_ARE_EQUAL(1u, activationProbeCalls);
            VERIFY_ARE_EQUAL(static_cast<DWORD>(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS), activationProbeFlags);
            VERIFY_ARE_EQUAL(0, _wcsicmp(libraryPath.c_str(), activationProbePath));
        }

} } } } } }
