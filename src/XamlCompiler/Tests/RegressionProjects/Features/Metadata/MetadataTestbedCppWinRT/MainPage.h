// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the MainPage class.
//

#pragma once

#include <vector>
#include "MainPage.g.h"

namespace winrt::MetadataTestbedCppWinRT::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        Windows::Foundation::IInspectable TestProperty();
        com_array<int32_t> MyProperty();
        void MyProperty(array_view<int32_t const> const& value);

        void GetTypeMemberManyTimesClicked(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        void GetTypeMemberTest();

        Microsoft::UI::Xaml::Markup::IXamlMetadataProvider m_provider{ nullptr };
        std::vector<int32_t> m_myProperty;
    };
}

namespace winrt::MetadataTestbedCppWinRT::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
