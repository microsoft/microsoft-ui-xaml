#pragma once

#ifndef WINRT_IMPORT_MODULE
#include "App.xaml.g.h"
#endif

namespace winrt::XamlCppWinRTModulesSample::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        Microsoft::UI::Xaml::Window m_window{ nullptr };
    };
}
