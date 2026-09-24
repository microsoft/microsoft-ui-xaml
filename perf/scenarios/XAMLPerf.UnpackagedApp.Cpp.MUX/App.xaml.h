#pragma once

#include "App.xaml.g.h"

namespace winrt::XAMLPerf_UnpackagedApp_Cpp_MUX::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        winrt::Microsoft::UI::Xaml::Window window{ nullptr };
    };
}
