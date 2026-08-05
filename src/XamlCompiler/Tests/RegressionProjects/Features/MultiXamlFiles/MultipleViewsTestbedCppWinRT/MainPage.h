// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace winrt::MultipleViewsTestbedCppWinRT::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        int32_t Dummy();
        void Dummy(int32_t value);

        void MyControl_Click(::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void MyControl_Checked(::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void WindowSizeChanged(::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::WindowSizeChangedEventArgs const& args);

        void LoadCorrectXamlFile();

        hstring filename;
    };
}

namespace winrt::MultipleViewsTestbedCppWinRT::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
