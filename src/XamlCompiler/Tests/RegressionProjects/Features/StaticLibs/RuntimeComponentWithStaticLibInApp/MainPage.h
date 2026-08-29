// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace winrt::RuntimeComponentWithStaticLibInApp::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        int32_t Dummy();
        void Dummy(int32_t value);

        void ClickHandler(::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::RuntimeComponentWithStaticLibInApp::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
