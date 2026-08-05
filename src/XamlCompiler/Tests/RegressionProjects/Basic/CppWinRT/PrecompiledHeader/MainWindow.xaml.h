// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MainWindow.g.h"

namespace winrt::PrecompiledHeader::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void myButton_Click(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::PrecompiledHeader::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
