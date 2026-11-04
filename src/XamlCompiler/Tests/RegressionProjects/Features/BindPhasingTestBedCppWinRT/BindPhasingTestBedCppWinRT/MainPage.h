// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
// The generated MainPage.xaml.g.hpp uses implementation::MyItem for the x:Phase bindings, and
// it only includes MainPage.h - same as BindTestbedCppWinRT's PhasingTests.h.
#include "MyItem.h"

namespace winrt::BindPhasingTestBedCppWinRT::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        void CreateTestItems();

        void MyGridView_ContainerContentChanging(
            Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);

        // Overrides are dispatched by the projection through IPageOverridesT, which calls
        // shim().OnNavigatedTo(...) from outside this class, so it has to be public.
        void OnNavigatedTo(Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);

    private:
        void ShowTitle(
            Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);

        void ShowSubtitle(
            Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);

        void ShowDescription(
            Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);

        Windows::Foundation::Collections::IVector<BindPhasingTestBedCppWinRT::MyItem> m_myItems{ nullptr };
    };
}

namespace winrt::BindPhasingTestBedCppWinRT::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
