#pragma once

#ifndef WINRT_IMPORT_MODULE
#include "App.xaml.g.h"
#endif

namespace winrt::NuGetModules::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        winrt::Microsoft::UI::Xaml::Window window{ nullptr };
    };
}
