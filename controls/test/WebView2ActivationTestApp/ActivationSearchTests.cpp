// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <windows.h>
#undef GetCurrentTime
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.Web.WebView2.Core.h>
#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace xaml = winrt::Microsoft::UI::Xaml;
namespace controls = winrt::Microsoft::UI::Xaml::Controls;
namespace webview = winrt::Microsoft::Web::WebView2::Core;

namespace
{
    std::wstring mode;
    std::filesystem::path pluginDirectory;
    DLL_DIRECTORY_COOKIE directoryCookie{};
    int result = 1;

    struct App : xaml::ApplicationT<App>
    {
        xaml::Window window{nullptr};
        controls::WebView2 web{nullptr};
        std::vector<HRESULT> initializedResults;
        winrt::Windows::Foundation::IAsyncAction pending{nullptr};

        winrt::fire_and_forget Run()
        {
            auto lifetime = get_strong();
            try
            {
                if (mode == L"recovery")
                {
                    web.Source(winrt::Windows::Foundation::Uri{L"about:blank"});
                    std::printf("Source-first events: %zu\n", initializedResults.size());
                    std::fflush(stdout);
                    if (pending)
                    {
                        co_await pending;
                        pending = nullptr;
                        std::printf("Pending initialization completed\n");
                    }
                    directoryCookie = AddDllDirectory(pluginDirectory.c_str());
                    winrt::check_bool(directoryCookie != nullptr);
                    auto environment = co_await webview::CoreWebView2Environment::CreateWithOptionsAsync(
                        L"", L"", webview::CoreWebView2EnvironmentOptions{});
                    co_await web.EnsureCoreWebView2Async(environment);
                }
                else
                {
                    co_await web.EnsureCoreWebView2Async();
                }

                wchar_t loadedPath[MAX_PATH]{};
                const auto core = GetModuleHandleW(L"Microsoft.Web.WebView2.Core.dll");
                if (core)
                {
                    winrt::check_bool(GetModuleFileNameW(core, loadedPath, ARRAYSIZE(loadedPath)) != 0);
                }
                std::printf("Ensure completed; events: %zu\nNative Core: %ls\n",
                    initializedResults.size(), loadedPath);
                const auto expectedCount = mode == L"recovery" ? 2u : 1u;
                bool correctEvents = initializedResults.size() == expectedCount;
                for (unsigned index = 0; index < initializedResults.size(); ++index)
                {
                    correctEvents = correctEvents && (mode == L"recovery" && index == 0 ?
                        FAILED(initializedResults[index]) : initializedResults[index] == S_OK);
                }
                const auto expectedPath = pluginDirectory / L"Microsoft.Web.WebView2.Core.dll";
                const bool correctPath = _wcsicmp(expectedPath.c_str(), loadedPath) == 0;
                result = correctEvents && correctPath && web.CoreWebView2() ? 0 : 1;
            }
            catch (winrt::hresult_error const& error)
            {
                std::printf("Ensure error: 0x%08X; events: %zu\n",
                    static_cast<unsigned>(error.code().value), initializedResults.size());
            }

            std::printf("Result: %d\n", result);
            std::fflush(stdout);
            web.Close();
            window.Content(nullptr);
            window.Close();
        }

        void OnLaunched(xaml::LaunchActivatedEventArgs const&)
        {
            if (mode != L"preload" && GetModuleHandleW(L"Microsoft.Web.WebView2.Core.dll"))
            {
                throw winrt::hresult_error(E_UNEXPECTED, L"The probe requires an uncached WebView2 activation factory.");
            }
            window = xaml::Window{};
            web = controls::WebView2{};
            web.CoreWebView2Initialized([this](auto const&, auto const& args)
            {
                initializedResults.push_back(args.Exception().value);
                std::printf("Initialized: 0x%08X\n", static_cast<unsigned>(args.Exception().value));
                std::fflush(stdout);
                if (initializedResults.size() == 1 && FAILED(args.Exception().value))
                {
                    // Register a waiter before the failing creation operation finishes.
                    pending = web.EnsureCoreWebView2Async();
                }
            });
            window.Content(web);
            window.Activate();
            Run();
        }
    };
}

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2 || (std::wstring_view(argv[1]) != L"added" &&
        std::wstring_view(argv[1]) != L"preload" && std::wstring_view(argv[1]) != L"recovery" &&
        std::wstring_view(argv[1]) != L"path"))
    {
        std::fprintf(stderr, "Usage: ActivationSearchTests.exe added|preload|recovery|path\n");
        return 2;
    }

    mode = argv[1];
    try
    {
        wchar_t executable[MAX_PATH]{};
        const auto length = GetModuleFileNameW(nullptr, executable, ARRAYSIZE(executable));
        winrt::check_bool(length > 0 && length < ARRAYSIZE(executable));
        pluginDirectory = std::filesystem::path(executable).parent_path() / L"plugin";

        wchar_t temporaryDirectory[MAX_PATH]{};
        const auto temporaryLength = GetTempPathW(ARRAYSIZE(temporaryDirectory), temporaryDirectory);
        winrt::check_bool(temporaryLength > 0 && temporaryLength < ARRAYSIZE(temporaryDirectory));
        const auto profile = std::filesystem::path(temporaryDirectory) /
            (L"WinUIActivation-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
        winrt::check_bool(SetEnvironmentVariableW(L"WEBVIEW2_USER_DATA_FOLDER", profile.c_str()));

        if (mode == L"path")
        {
            const auto pathSize = GetEnvironmentVariableW(L"PATH", nullptr, 0);
            winrt::check_bool(pathSize != 0);
            std::wstring processPath(pathSize, L'\0');
            const auto pathLength = GetEnvironmentVariableW(L"PATH", processPath.data(), pathSize);
            winrt::check_bool(pathLength > 0 && pathLength < pathSize);
            processPath.resize(pathLength);
            winrt::check_bool(SetEnvironmentVariableW(L"PATH",
                (pluginDirectory.wstring() + L";" + processPath).c_str()));
        }
        else if (mode != L"recovery")
        {
            directoryCookie = AddDllDirectory(pluginDirectory.c_str());
            winrt::check_bool(directoryCookie != nullptr);
        }
        if (mode == L"preload")
        {
            winrt::check_bool(LoadLibraryW((pluginDirectory / L"Microsoft.Web.WebView2.Core.dll").c_str()) != nullptr);
        }

        // Match the SDK's self-contained reg-free WinRT auto-initializer.
        winrt::check_bool(LoadLibraryW(L"Microsoft.WindowsAppRuntime.dll") != nullptr);
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        xaml::Application::Start([](auto const&) { winrt::make<App>(); });
    }
    catch (winrt::hresult_error const& error)
    {
        std::printf("Startup error: 0x%08X\n", static_cast<unsigned>(error.code().value));
    }
    if (directoryCookie && !RemoveDllDirectory(directoryCookie))
    {
        std::fprintf(stderr, "RemoveDllDirectory failed: %lu\n", GetLastError());
        result = 1;
    }
    return result;
}
