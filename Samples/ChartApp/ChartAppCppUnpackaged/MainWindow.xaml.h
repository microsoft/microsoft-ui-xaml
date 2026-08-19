#pragma once

#include "MainWindow.g.h"

namespace winrt::ChartAppCppUnpackaged::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        void ToggleTheme_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::ChartAppCppUnpackaged::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
