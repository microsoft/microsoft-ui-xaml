// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace winrt::BindPhasingTestBedCppWinRT::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        void CreateTestItems();

        void MyGridView_ContainerContentChanging(
            Microsoft::UI::Xaml::Controls::ListViewBase const& sender,
            Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs const& args);

    protected:
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
