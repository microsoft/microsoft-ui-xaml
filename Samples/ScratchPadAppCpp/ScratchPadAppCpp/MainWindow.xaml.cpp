// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "MainWindow.xaml.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::ScratchPadAppCpp::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
    }
}
