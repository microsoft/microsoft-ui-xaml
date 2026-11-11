// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MainPageBase.h"
#include "MainPageBase.g.cpp"

namespace winrt::Simple::implementation
{
    MainPageBase::MainPageBase(hstring const& name)
        : pageName(name)
    {}

    void MainPageBase::EventHandlerOnBase(IInspectable  const&, Microsoft::UI::Xaml::RoutedEventArgs const&)
    {}

    void MainPageBase::OnNavigatedTo(Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e)
    {
        Microsoft::UI::Xaml::Controls::IPageOverridesT<MainPageBase>::OnNavigatedTo(e);
    }
}
