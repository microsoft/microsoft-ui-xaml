#include "pch.h"

#include "App.h"

using namespace winrt;
using namespace ::Windows::Foundation;

using namespace winrt::Microsoft::UI::Input;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Hosting;
using namespace winrt::Microsoft::UI::Xaml::Navigation;

using namespace TabViewTearOutApp;
using namespace TabViewTearOutApp::implementation;

winrt::com_ptr<App> App::s_instance{ nullptr };

App::App()
{
    WindowsXamlManager::InitializeForCurrentThread();
    
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

void App::InitInstance()
{
    if (!s_instance)
    {
        winrt::make<winrt::TabViewTearOutApp::implementation::App>().as(s_instance);
    }
}

com_ptr<App> App::GetInstance()
{
    return s_instance;
}
