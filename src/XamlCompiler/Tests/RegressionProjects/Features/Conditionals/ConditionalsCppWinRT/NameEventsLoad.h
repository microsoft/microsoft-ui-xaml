// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the NameEventsLoad class.
//

#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Interop.h"
#include "NameEventsLoad.g.h"

namespace winrt::ConditionalsCppWinRT::implementation
{
    struct NameEventsLoad : NameEventsLoadT<NameEventsLoad>
    {
        NameEventsLoad();

        void Button_Click(::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void Button_Click_V1(::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void Button_Click_V2(::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void Button_Click_V3(::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void Button_Click_notV3(::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::ConditionalsCppWinRT::factory_implementation
{
    struct NameEventsLoad : NameEventsLoadT<NameEventsLoad, implementation::NameEventsLoad>
    {
    };
}
