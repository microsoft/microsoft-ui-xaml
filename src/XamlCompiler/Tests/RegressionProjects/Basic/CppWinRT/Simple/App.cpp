// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// App.xaml.cpp
// Implementation of the App class.
//

#include "pch.h"
#include "windows.h"
#include "winrt/Microsoft.UI.Xaml.Navigation.h"
#include <cstdio>
#include <cstdlib>
#include <string_view>

#include "App.h"
#include "MainPage.h"

using namespace ::winrt::Simple::implementation;

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Interop;

// The Blank Application template is documented at http://go.microsoft.com/fwlink/?LinkId=402347&clcid=0x409

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
    InitializeComponent();

    UnhandledException([](IInspectable const&, ::Microsoft::UI::Xaml::UnhandledExceptionEventArgs const& e)
    {
        std::fwprintf(stderr, L"Unhandled exception: %ls\n", e.Message().c_str());
#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
        if (IsDebuggerPresent())
        {
            __debugbreak();
        }
#endif
    });
}

/// <summary>
/// Invoked when the application is launched normally by the end user.	Other entry points
/// will be used such as when the application is launched to open a specific file.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched(winrt::Microsoft::UI::Xaml::LaunchActivatedEventArgs const& e)
{
    Frame rootFrame;
    rootFrame.NavigationFailed({ this, &App::OnNavigationFailed });
    m_window = Window();
    m_window.Content(rootFrame);

    if (__argc == 2 && std::wstring_view(__wargv[1]) == L"FindNameTest")
    {
        m_window.Activate();
        try
        {
            MainPage::ValidateStringNameLookup();
        }
        catch (hresult_error const& error)
        {
            std::fwprintf(stderr, L"Named string lookup test failed: %ls (0x%08X)\n",
                error.message().c_str(), static_cast<uint32_t>(error.code()));
            throw;
        }
        std::puts("Named string lookup tests passed.");
        std::fflush(stdout);
        Exit();
        return;
    }

    auto navigationParameter = e.Arguments();
    if (__argc == 2 && std::wstring_view(__wargv[1]) == L"IncrementalTest")
    {
        navigationParameter = L"IncrementalTest";
    }
    rootFrame.Navigate(xaml_typename<winrt::Simple::MainPage>(), box_value(navigationParameter));
    m_window.Activate();
}

/// <summary>
/// Invoked when Navigation to a certain page fails
/// </summary>
/// <param name="sender">The Frame which failed navigation</param>
/// <param name="e">Details about the navigation failure</param>
void App::OnNavigationFailed(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Navigation::NavigationFailedEventArgs const& e)
{
    std::wstring message(L"Failed to load Page ");
    std::fwprintf(stderr, L"Failed to load Page %ls\n", e.SourcePageType().Name.c_str());
    throw hresult_error(E_FAIL, message.append(e.SourcePageType().Name));
}