#pragma once

#include "MainWindow.g.h"

namespace winrt::TableViewAppCppUnpackaged::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        void ToggleTheme_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        winrt::Microsoft::UI::Xaml::Controls::Tabular::TableViewTemplateColumn MakeColumn(
            winrt::hstring const& header,
            winrt::hstring const& templateKey);
    };
}

namespace winrt::TableViewAppCppUnpackaged::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}