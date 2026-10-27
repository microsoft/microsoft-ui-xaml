// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <ppltasks.h>
#include <wil/resource.h>
#include <wil/stl.h>
#include <wil/win32_helpers.h>
#include <ActivationHelpers.h>
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
            wil::unique_hmodule repeated(LoadLibraryForActivation(L"version.dll"));
            VERIFY_ARE_EQUAL(existing.get(), repeated.get());

            const wchar_t* libraryName = L"WinUIActivationProbe.dll";
            const std::wstring libraryPath = std::wstring(directory) + L"\\" + libraryName;
            const auto sourcePath = wil::GetModuleFileNameW<std::wstring>(existing.get());
            VERIFY_IS_TRUE(CopyFileW(sourcePath.c_str(), libraryPath.c_str(), TRUE));
            auto removeCopy = wil::scope_exit([&]() { VERIFY_IS_TRUE(DeleteFileW(libraryPath.c_str())); });

            // This method has its own process. Exclude user directories from its default search
            // so the host's DLL policy cannot mask a missing explicit fallback.
            VERIFY_IS_TRUE(SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32));
            VERIFY_IS_NULL(GetModuleHandleW(libraryName));
            const auto cookie = AddDllDirectory(directory);
            VERIFY_IS_NOT_NULL(cookie);
            auto removeCookie = wil::scope_exit([&]() { VERIFY_IS_TRUE(RemoveDllDirectory(cookie)); });
            wil::unique_hmodule primary(LoadLibraryW(libraryName));
            VERIFY_IS_NULL(primary.get());

            wil::unique_hmodule module(LoadLibraryForActivation(libraryName));
            VERIFY_IS_NOT_NULL(module.get());
            const auto loadedPath = wil::GetModuleFileNameW<std::wstring>(module.get());
            VERIFY_ARE_EQUAL(0, _wcsicmp(libraryPath.c_str(), loadedPath.c_str()));
        }

        void ActivationIntegrationTests::WebView2EnvironmentFailureCompletesCreation()
        {
            TestCleanupWrapper cleanup;
            WebView2^ webView;
            wil::unique_cotaskmem_string originalBrowserFolder;
            RunOnUIThread([&]()
            {
                VERIFY_SUCCEEDED(wil::TryGetEnvironmentVariableW(L"WEBVIEW2_BROWSER_EXECUTABLE_FOLDER", originalBrowserFolder));
                webView = ref new WebView2();
            });
            auto restoreBrowserFolder = wil::scope_exit([&]()
            {
                RunOnUIThread([&]()
                {
                    VERIFY_IS_TRUE(SetEnvironmentVariableW(L"WEBVIEW2_BROWSER_EXECUTABLE_FOLDER", originalBrowserFolder.get()));
                });
            });
            auto closeWebView = wil::scope_exit([&]()
            {
                RunOnUIThread([&]() { webView->Close(); });
            });

            unsigned int initializedCount = 0;
            IAsyncAction^ initialization;
            auto initializationCompleted = std::make_shared<Event>(L"WebView2 initialization completed");
            auto observeCompletion = [&](IAsyncAction^ action)
            {
                initialization = action;
                action->Completed = ref new AsyncActionCompletedHandler(
                    [initializationCompleted](IAsyncAction^, AsyncStatus)
                {
                    initializationCompleted->Set();
                });
            };
            auto initializedRegistration = CreateSafeEventRegistration(WebView2, CoreWebView2Initialized);
            RunOnUIThread([&]()
            {
                const auto missingBrowserFolder = wil::ExpandEnvironmentStringsW<std::wstring>(
                    L"%TEMP%\\WinUI-Missing-WebView2-Runtime-") + std::to_wstring(GetCurrentProcessId());
                VERIFY_ARE_EQUAL(INVALID_FILE_ATTRIBUTES, GetFileAttributesW(missingBrowserFolder.c_str()));
                VERIFY_IS_TRUE(SetEnvironmentVariableW(L"WEBVIEW2_BROWSER_EXECUTABLE_FOLDER", missingBrowserFolder.c_str()));

                initializedRegistration.Attach(webView, ref new TypedEventHandler<WebView2^, CoreWebView2InitializedEventArgs^>(
                    [&](WebView2^ sender, CoreWebView2InitializedEventArgs^ args)
                {
                    VERIFY_IS_TRUE(FAILED(args->Exception.Value));
                    if (++initializedCount == 1)
                    {
                        // Join the failing initialization before it completes.
                        observeCompletion(sender->EnsureCoreWebView2Async());
                    }
                }));
            });

            for (unsigned int attempt = 0; attempt < 3; ++attempt)
            {
                initializationCompleted->Reset();
                RunOnUIThread([&]()
                {
                    if (attempt == 0)
                    {
                        webView->Source = ref new Uri(L"about:blank");
                    }
                    else
                    {
                        observeCompletion(webView->EnsureCoreWebView2Async());
                    }
                });
                initializationCompleted->WaitForDefault();
                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(AsyncStatus::Completed, initialization->Status);
                    initialization->GetResults();
                    VERIFY_ARE_EQUAL(attempt + 1, initializedCount);
                });
            }
        }

} } } } } }
