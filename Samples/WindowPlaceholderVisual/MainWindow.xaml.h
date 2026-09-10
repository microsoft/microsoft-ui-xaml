#pragma once

#include "MainWindow.g.h"

namespace winrt::WindowPlaceholderVisual::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow() = default;

        void BeginDelayedContent();

    private:
        void AttachFinalContent();

        Microsoft::UI::Dispatching::DispatcherQueueTimer m_contentTimer{ nullptr };
    };
}

namespace winrt::WindowPlaceholderVisual::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
