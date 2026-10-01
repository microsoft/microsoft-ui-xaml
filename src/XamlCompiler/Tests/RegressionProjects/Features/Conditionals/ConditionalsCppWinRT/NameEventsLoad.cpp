// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "NameEventsLoad.h"
#include "NameEventsLoad.g.cpp"

using namespace winrt::ConditionalsCppWinRT;
using namespace winrt::ConditionalControls;

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::ConditionalsCppWinRT::implementation
{
    NameEventsLoad::NameEventsLoad()
    {
        InitializeComponent();
    }
    void NameEventsLoad::Button_Click(::winrt::Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        buttonResult().Text(L"always");
    }

    void NameEventsLoad::Button_Click_V1(::winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        IVersionedProperties obj = (IVersionedProperties)sender.try_as<IVersionedProperties>();
        buttonResult().Text(obj.V1Property());
    }

    void NameEventsLoad::Button_Click_V2(::winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        IVersionedProperties obj = (IVersionedProperties)sender.try_as<IVersionedProperties>();
        buttonResult().Text(obj.V2Property());
    }

    void NameEventsLoad::Button_Click_V3(::winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        IVersionedProperties obj = (IVersionedProperties)sender.try_as<IVersionedProperties>();
        buttonResult().Text(obj.V3Property());
    }

    void NameEventsLoad::Button_Click_notV3(::winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        IVersionedProperties obj = (IVersionedProperties)sender.try_as<IVersionedProperties>();
        buttonResult().Text(L"notV3here's" + obj.V2Property());
    }
}
