// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <ppltasks.h>
#include <wil/resource.h>
#include "..\..\..\..\..\..\..\controls\dev\dll\ActivationLibrary.h"
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

            wchar_t systemDirectory[MAX_PATH]{};
            const auto systemLength = GetSystemDirectoryW(systemDirectory, ARRAYSIZE(systemDirectory));
            VERIFY_IS_TRUE(systemLength > 0 && systemLength < ARRAYSIZE(systemDirectory));
            const std::wstring source = std::wstring(systemDirectory) + L"\\version.dll";
            const wchar_t* name = L"WinUIActivationProbe.dll";
            const std::wstring copy = std::wstring(directory) + L"\\" + name;
            VERIFY_IS_TRUE(CopyFileW(source.c_str(), copy.c_str(), TRUE));
            auto removeCopy = wil::scope_exit([&]() { VERIFY_IS_TRUE(DeleteFileW(copy.c_str())); });

            wil::unique_hmodule existing(LoadLibraryW(L"version.dll"));
            VERIFY_IS_NOT_NULL(existing.get());
            wil::unique_hmodule repeated(MuxcActivation::LoadLibraryForActivation(L"version.dll"));
            VERIFY_ARE_EQUAL(existing.get(), repeated.get());

            auto verifyLoad = [&]()
            {
                VERIFY_IS_NULL(GetModuleHandleW(name));
                wil::unique_hmodule module(MuxcActivation::LoadLibraryForActivation(name));
                VERIFY_IS_NOT_NULL(module.get());
                wchar_t loadedPath[MAX_PATH]{};
                const auto loadedLength = GetModuleFileNameW(module.get(), loadedPath, ARRAYSIZE(loadedPath));
                VERIFY_IS_TRUE(loadedLength > 0 && loadedLength < ARRAYSIZE(loadedPath));
                VERIFY_ARE_EQUAL(0, _wcsicmp(copy.c_str(), loadedPath));
            };

            // This method has its own process. Exclude user directories from its default search
            // so the host's DLL policy cannot mask a missing explicit fallback.
            VERIFY_IS_TRUE(SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32));
            VERIFY_IS_NULL(GetModuleHandleW(name));
            auto cookie = AddDllDirectory(directory);
            VERIFY_IS_NOT_NULL(cookie);
            auto removeCookie = wil::scope_exit([&]() { VERIFY_IS_TRUE(RemoveDllDirectory(cookie)); });
            wil::unique_hmodule primary(LoadLibraryW(name));
            VERIFY_IS_NULL(primary.get());
            verifyLoad();
        }

        void ActivationIntegrationTests::WebView2EnvironmentFailureCompletesCreation()
        {
            std::wstring originalFolder;
            bool hadOriginalFolder = false;
            RunOnUIThread([&]()
            {
                const auto size = GetEnvironmentVariableW(L"WEBVIEW2_BROWSER_EXECUTABLE_FOLDER", nullptr, 0);
                const auto error = GetLastError();
                VERIFY_IS_TRUE(size > 0 || error == ERROR_ENVVAR_NOT_FOUND || error == ERROR_SUCCESS);
                hadOriginalFolder = size > 0 || error == ERROR_SUCCESS;
                if (size > 0)
                {
                    originalFolder.resize(size);
                    const auto length = GetEnvironmentVariableW(
                        L"WEBVIEW2_BROWSER_EXECUTABLE_FOLDER", &originalFolder[0], size);
                    VERIFY_IS_TRUE(length < size);
                    originalFolder.resize(length);
                }
                wchar_t temporaryPath[MAX_PATH]{};
                const auto length = GetTempPathW(ARRAYSIZE(temporaryPath), temporaryPath);
                VERIFY_IS_TRUE(length > 0 && length < ARRAYSIZE(temporaryPath));
                const auto missingFolder = std::wstring(temporaryPath) +
                    L"WinUI-Missing-WebView2-Runtime-" + std::to_wstring(GetCurrentProcessId());
                VERIFY_ARE_EQUAL(INVALID_FILE_ATTRIBUTES, GetFileAttributesW(missingFolder.c_str()));
                VERIFY_IS_TRUE(SetEnvironmentVariableW(L"WEBVIEW2_BROWSER_EXECUTABLE_FOLDER", missingFolder.c_str()));
            });
            auto restoreFolder = wil::scope_exit([&]()
            {
                RunOnUIThread([&]()
                {
                    VERIFY_IS_TRUE(SetEnvironmentVariableW(L"WEBVIEW2_BROWSER_EXECUTABLE_FOLDER",
                        hadOriginalFolder ? originalFolder.c_str() : nullptr));
                });
            });

            WebView2^ webView;
            auto initializedResults = std::make_shared<std::vector<HRESULT>>();
            auto pendingCompleted = std::make_shared<Event>();
            auto pendingStatus = std::make_shared<AsyncStatus>(AsyncStatus::Started);
            RunOnUIThread([&]()
            {
                webView = ref new WebView2();
                webView->CoreWebView2Initialized += ref new TypedEventHandler<WebView2^, CoreWebView2InitializedEventArgs^>(
                    [initializedResults, pendingCompleted, pendingStatus](WebView2^ sender, CoreWebView2InitializedEventArgs^ args)
                {
                    initializedResults->push_back(args->Exception.Value);
                    if (initializedResults->size() == 1)
                    {
                        auto pending = sender->EnsureCoreWebView2Async();
                        pending->Completed = ref new AsyncActionCompletedHandler(
                            [pendingCompleted, pendingStatus](IAsyncAction^, AsyncStatus status)
                        {
                            *pendingStatus = status;
                            pendingCompleted->Set();
                        });
                    }
                });
            });
            auto closeWebView = wil::scope_exit([&]()
            {
                RunOnUIThread([&]() { webView->Close(); });
            });

            RunOnUIThread([&]() { webView->Source = ref new Uri(L"about:blank"); });
            pendingCompleted->WaitForDefault();
            VERIFY_ARE_EQUAL(AsyncStatus::Completed, *pendingStatus);

            for (int attempt = 0; attempt < 2; ++attempt)
            {
                auto completed = std::make_shared<Event>();
                IAsyncAction^ initialization;
                RunOnUIThread([&]()
                {
                    initialization = webView->EnsureCoreWebView2Async();
                    initialization->Completed = ref new AsyncActionCompletedHandler(
                        [completed](IAsyncAction^, AsyncStatus) { completed->Set(); });
                });
                completed->WaitForDefault();
                RunOnUIThread([&]() { VERIFY_ARE_EQUAL(AsyncStatus::Completed, initialization->Status); });
            }

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(3u, static_cast<unsigned>(initializedResults->size()));
                for (auto hr : *initializedResults)
                {
                    VERIFY_IS_TRUE(FAILED(hr));
                }
            });
        }

} } } } } }
