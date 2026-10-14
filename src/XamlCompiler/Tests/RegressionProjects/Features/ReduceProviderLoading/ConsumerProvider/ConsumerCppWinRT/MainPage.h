// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace winrt::ConsumerCppWinRT::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        void Button_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        bool m_otherProviderLoaded{ false };
    };
}

namespace winrt::ConsumerCppWinRT::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
