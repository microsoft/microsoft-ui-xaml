#pragma once

#include "App.xaml.g.h"

namespace winrt::ChartAppCppPackaged::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        uint32_t m_primaryThreadId{};
        winrt::Microsoft::UI::Xaml::Window window{ nullptr };
    };
}
