#pragma once

#include "MainWindow.g.h"

namespace winrt::CompositionSwitcherSample::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow() = default;

        void OnRootLoaded(
            Windows::Foundation::IInspectable const&,
            Microsoft::UI::Xaml::RoutedEventArgs const&);

    private:
        Microsoft::UI::Composition::SpriteVisual m_sprite{ nullptr };
    };
}

namespace winrt::CompositionSwitcherSample::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
