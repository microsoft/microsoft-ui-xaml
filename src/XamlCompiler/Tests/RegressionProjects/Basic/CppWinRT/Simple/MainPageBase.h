// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "MainPageBase.g.h"
#include "winrt/Microsoft.UI.Xaml.Navigation.h"

namespace winrt::Simple::implementation
{
    struct MainPageBase : MainPageBaseT<MainPageBase>
    {
        MainPageBase() = default;
        MainPageBase(hstring const&);
        void EventHandlerOnBase(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);

        hstring PageName() { return pageName; }

        // Forward navigation virtually so derived C++ pages receive the callback.
        virtual void OnNavigatedTo(Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);

    private:
        hstring pageName;
    };
}

namespace winrt::Simple::factory_implementation
{
    struct MainPageBase : MainPageBaseT<MainPageBase, implementation::MainPageBase>
    {
    };
}
