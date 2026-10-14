#pragma once

#include "MainWindow.g.h"

namespace winrt::XamlCppWinRTModulesSample::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        Models::GreetingModel Model() const noexcept
        {
            return m_model;
        }

    private:
        Models::GreetingModel m_model{ nullptr };
    };
}

namespace winrt::XamlCppWinRTModulesSample::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
