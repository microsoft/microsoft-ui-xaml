#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::ChartsSample::implementation
{
    App::App()
    {
        m_primaryThreadId = GetCurrentThreadId();
        InitializeComponent();
#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
        UnhandledException([](IInspectable const&, UnhandledExceptionEventArgs const& e)
        {
            if (IsDebuggerPresent())
            {
                auto errorMessage = e.Message();
                __debugbreak();
            }
        });
#endif
    }

    void App::OnLaunched([[maybe_unused]] LaunchActivatedEventArgs const& e)
    {
        // Initializing XAML on another STA must not create or replace the primary window.
        if (GetCurrentThreadId() != m_primaryThreadId) return;
        if (!window) window = make<winrt::ChartsSample::implementation::MainWindow>(L"C++/WinRT | Unpackaged | Synthetic data");
        window.Activate();
    }
}
