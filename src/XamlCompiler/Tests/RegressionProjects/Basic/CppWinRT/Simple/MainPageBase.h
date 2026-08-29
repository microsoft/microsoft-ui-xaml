// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "MainPageBase.g.h"

namespace winrt::Simple::implementation
{
    struct MainPageBase : MainPageBaseT<MainPageBase>
    {
        MainPageBase() = default;
        MainPageBase(hstring const&);
        void EventHandlerOnBase(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);

        hstring PageName() { return pageName; }

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

