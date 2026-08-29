// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#include "App.h"
#include "MainWindow.h"
#include <winrt/Microsoft.Windows.ApplicationModel.Resources.h>
#include "ShutdownOrderValidation.h"

using namespace winrt;
using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Navigation;
using namespace WinUICppDesktopSampleApp;
using namespace WinUICppDesktopSampleApp::implementation;

// To learn more about WinUI, the WinUI project structure,
// and project templates, see the WinUI documentation.

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
    InitializeComponent();

    // Use our own ResourceManager so that we can subscribe to the ResourceNotFound event
    // and provide a "404" page. (This is actually really bad design to use IRL because it
    // doesn't handle obvious scenarios like an unresolvable ResourceDictionary.Source.)
    Application::Current().ResourceManagerRequested([this](IInspectable const&, ResourceManagerRequestedEventArgs const& args)
    {
        auto resourceManager = Microsoft::Windows::ApplicationModel::Resources::ResourceManager();
        resourceManager.ResourceNotFound([](IInspectable const&, Microsoft::Windows::ApplicationModel::Resources::ResourceNotFoundEventArgs const& eventArgs)
        {
            // Look specifically for the sentinel page name to avoid accidentally polluting scenarios wherein we are
            // probing possible names for a resource.
            if (eventArgs.Name() == L"Files/786B1897-FEB4-4FC8-9796-9F7DD21582CC.xbf")
            {
                auto resMan = Microsoft::Windows::ApplicationModel::Resources::ResourceManager();
                auto candidate = resMan.MainResourceMap().GetValue(L"Files/Page404.xbf", eventArgs.Context());
                eventArgs.SetResolvedCandidate(candidate);
            }
        });
        
        args.CustomResourceManager(resourceManager);
    });

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

App::~App()
{
    ShutdownOrderValidation::Log(L"App::~App called.");
}

/// <summary>
/// Invoked when the application is launched normally by the end user.  Other entry points
/// will be used such as when the application is launched to open a specific file.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched(LaunchActivatedEventArgs const&)
{
    window = make<MainWindow>();
    window.Activate();
    window.Title(L"WinUICppDesktopSampleApp");
    window.as<MainWindow>()->RootFrame().Navigate(xaml_typename<Page1>());
}
