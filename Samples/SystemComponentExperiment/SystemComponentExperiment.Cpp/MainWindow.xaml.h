#pragma once

#include "MainWindow.g.h"

namespace winrt::SystemComponentExperiment::Cpp::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        void ScenarioList_SelectionChanged(
            Windows::Foundation::IInspectable const&,
            Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);

        void RunScenario_Click(
            Windows::Foundation::IInspectable const&,
            Microsoft::UI::Xaml::RoutedEventArgs const&);

    private:
        void RunEnvironmentScenario();
        void RunCompositionScenario();
        void RunDispatcherQueueScenario();
    };
}

namespace winrt::SystemComponentExperiment::Cpp::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
