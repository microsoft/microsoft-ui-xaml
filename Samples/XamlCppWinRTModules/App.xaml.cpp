#include <windows.h>

#define WINRT_IMPORT_MODULE
import winrt.Windows.Foundation;
import winrt.Microsoft.UI.Xaml;

#include "App.xaml.h"
#include "MainWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::XamlCppWinRTModulesSample::implementation
{
    App::App()
    {
#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
        UnhandledException([](IInspectable const&, UnhandledExceptionEventArgs const& e)
        {
            if (IsDebuggerPresent())
            {
                auto const errorMessage = e.Message();
                (void)errorMessage;
                __debugbreak();
            }
        });
#endif
    }

    void App::OnLaunched(LaunchActivatedEventArgs const&)
    {
        m_window = make<MainWindow>();
        m_window.Activate();
    }
}
