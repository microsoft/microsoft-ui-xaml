// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// App.xaml.cpp
// Implementation of the App class.
//

#include "pch.h"
#include "windows.h"
#include "winrt/Windows.ApplicationModel.Activation.h"
#include "winrt/Microsoft.UI.Xaml.Navigation.h"

#include "App.h"
#include "MainPage.h"

namespace winrt::BindTestbed::implementation
{
    // The Blank Application template is documented at http://go.microsoft.com/fwlink/?LinkId=402347&clcid=0x409

    BindTestbedModel::DataModel App::Model = nullptr;
    BindTestbedModel::DOModel App::DOModel = nullptr;
    wux::Window window = nullptr;

    /// <summary>
    /// Initializes the singleton application object.  This is the first line of authored code
    /// executed, and as such is the logical equivalent of main() or WinMain().
    /// </summary>
    App::App()
    {
        InitializeComponent();

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
        UnhandledException([this](IInspectable const&, wux::UnhandledExceptionEventArgs const& e)
            {
                if (IsDebuggerPresent())
                {
                    auto errorMessage = e.Message();
                    __debugbreak();
                }
            });
#endif

        Model = BindTestbedModel::DataModel();
        DOModel = BindTestbedModel::DOModel();
    }

    /// <summary>
    /// Invoked when the application is launched normally by the end user.	Other entry points
    /// will be used such as when the application is launched to open a specific file.
    /// </summary>
    /// <param name="e">Details about the launch request and process.</param>
    void App::OnLaunched(wux::LaunchActivatedEventArgs const& e)
    {
        wuxc::Frame rootFrame(nullptr);

        // Desktop (Win32) app model: the window is created by the app, not supplied by
        // the platform as Window::Current() did in the app-container model.
        window = wux::Window::Window();
        auto content = window.Content();
        rootFrame = content.try_as<wuxc::Frame>();
        // Do not repeat app initialization when the Window already has content,
        // just ensure that the window is active
        if (rootFrame == nullptr)
        {
            // Create a Frame to act as the navigation context and associate it with
            // a SuspensionManager key
            rootFrame = wuxc::Frame();
            auto launchedActivatedEventArgs = e.UWPLaunchActivatedEventArgs();

            rootFrame.NavigationFailed({ this, &App::OnNavigationFailed });

            if (launchedActivatedEventArgs.PreviousExecutionState() == wa::Activation::ApplicationExecutionState::Terminated)
            {
                // Restore the saved session state only when appropriate, scheduling the
                // final launch steps after the restore is complete

            }
            window.Content(rootFrame);
        }

        if (rootFrame.Content() == nullptr)
        {
            // When the navigation stack isn't restored navigate to the first page,
            // configuring the new page by passing required information as a navigation
            // parameter
            rootFrame.Navigate(xaml_typename<BindTestbed::MainPage>(), box_value(e.Arguments()));
        }
        // Ensure the current window is active
        window.Activate();

    }

    /// <summary>
    /// Invoked when Navigation to a certain page fails
    /// </summary>
    /// <param name="sender">The Frame which failed navigation</param>
    /// <param name="e">Details about the navigation failure</param>
    void App::OnNavigationFailed(IInspectable const&, wux::Navigation::NavigationFailedEventArgs const& e)
    {
        std::wstring message(L"Failed to load Page ");
        throw hresult_error(E_FAIL, message.append(e.SourcePageType().Name));
    }
}