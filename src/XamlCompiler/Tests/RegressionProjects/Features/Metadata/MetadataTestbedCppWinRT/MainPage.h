// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace winrt::MetadataTestbedCppWinRT::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        Windows::Foundation::IInspectable TestProperty();

        void GetTypeMemberManyTimesClicked(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        void GetTypeMemberTest();

        Microsoft::UI::Xaml::Markup::IXamlMetadataProvider m_provider{ nullptr };
    };
}

namespace winrt::MetadataTestbedCppWinRT::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
