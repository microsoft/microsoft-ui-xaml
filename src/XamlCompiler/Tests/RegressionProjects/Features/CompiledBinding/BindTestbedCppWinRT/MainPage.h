// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "MainPage.g.h"

namespace winrt::BindTestbed::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        BindTestbed::MainModel MainModel() { return _mainModel; }
        void DetectLeaks_Click(IInspectable const& sender, wux::RoutedEventArgs const& e);

    public:
        BindTestbed::MainModel _mainModel { nullptr };
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
