// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// App.xaml.cpp
// Implementation of the App class.
//

#include "pch.h"

#include "App.h"
#include "MainPage.h"

using namespace winrt;
using namespace ::winrt::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Navigation;
using namespace CppWinRT;
using namespace CppWinRT::implementation;

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
    InitializeComponent();

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
    UnhandledException([this](IInspectable const&, UnhandledExceptionEventArgs const& e)
    {
        if (IsDebuggerPresent())
        {
            auto errorMessage = e.Message();
            __debugbreak();
        }
    });
#endif
}

/// <summary>
/// Invoked when the application is launched.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const& e)
{
    m_window = Window();

    Frame rootFrame;
    rootFrame.NavigationFailed({ this, &App::OnNavigationFailed });
    rootFrame.Navigate(xaml_typename<CppWinRT::MainPage>(), box_value(e.Arguments()));

    m_window.Content(rootFrame);
    m_window.Activate();
}

/// <summary>
/// Invoked when Navigation to a certain page fails
/// </summary>
/// <param name="sender">The Frame which failed navigation</param>
/// <param name="e">Details about the navigation failure</param>
void App::OnNavigationFailed(IInspectable const&, NavigationFailedEventArgs const& e)
{
    throw hresult_error(E_FAIL, hstring(L"Failed to load Page ") + e.SourcePageType().Name);
}
