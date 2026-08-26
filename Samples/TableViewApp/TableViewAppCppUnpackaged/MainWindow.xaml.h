#pragma once

#include "MainWindow.g.h"

namespace winrt::TableViewAppCppUnpackaged::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        // Bound from markup via x:Bind; populated before InitializeComponent.
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Windows::Foundation::IInspectable> People() const { return m_people; }

        void ToggleTheme_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        winrt::Microsoft::UI::Xaml::Controls::Tabular::TableViewTemplateColumn MakeColumn(
            winrt::hstring const& header,
            winrt::hstring const& templateKey);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Windows::Foundation::IInspectable> m_people{ nullptr };
    };
}

namespace winrt::TableViewAppCppUnpackaged::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}