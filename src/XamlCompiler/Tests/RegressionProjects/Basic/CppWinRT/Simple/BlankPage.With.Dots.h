// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the BlankPageBase and BlankPage classes.
//

#pragma once

#include "BlankPageBase.g.h"
#include "BlankPage.g.h"

namespace winrt::Simple::implementation
{
    struct BlankPageBase : BlankPageBaseT<BlankPageBase>
    {
        BlankPageBase() = default;

        hstring Foo() { return {}; }
        void OnLoaded(IInspectable const&, ::winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) {}
    };

    struct BlankPage : BlankPageT<BlankPage>
    {
        BlankPage();
    };
}

namespace winrt::Simple::factory_implementation
{
    struct BlankPageBase : BlankPageBaseT<BlankPageBase, implementation::BlankPageBase>
    {
    };

    struct BlankPage : BlankPageT<BlankPage, implementation::BlankPage>
    {
    };
}
