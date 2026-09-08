// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "MainWindow.g.h"

namespace winrt::ScratchPadAppCpp::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow() = default;
    };
}

namespace winrt::ScratchPadAppCpp::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
