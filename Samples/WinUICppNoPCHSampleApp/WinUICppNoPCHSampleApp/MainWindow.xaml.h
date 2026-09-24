// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "MainWindow.g.h"

namespace winrt::WinUICppNoPCHSampleApp::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See the C++/WinRT InitializeComponent documentation.
        }

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void myButton_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::WinUICppNoPCHSampleApp::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
