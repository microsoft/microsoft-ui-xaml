#include "pch.h"

#include "App.h"

using namespace winrt;
using namespace ::Windows::Foundation;

using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Navigation;

using namespace WinUITextInputXamlIslandApp;
using namespace WinUITextInputXamlIslandApp::implementation;

// To learn more about WinUI, the WinUI project structure,
// and project templates, see the public WinUI documentation.

void OnAppCreated();

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
    ::OutputDebugString(L">>> App::App has been called.\n");
    OnAppCreated();
    
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

App::~App()
{
    ::OutputDebugString(L">>> App::~App has been called.\n");
}

/// <summary>
/// Invoked when the application is launched normally by the end user.  Other entry points
/// will be used such as when the application is launched to open a specific file.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched(LaunchActivatedEventArgs const&)
{
    ::OutputDebugString(L">>> App::OnLaunched has been called.\n");
}
