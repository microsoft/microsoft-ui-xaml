// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the BlankPageBase class, the unsealed base of BlankPage.
//
// It lives in its own header, and not in BlankPage.With.Dots.h, because the XAML-generated
// BlankPage header refers to implementation::BlankPageBase and therefore has to see it first.
// This mirrors the MainPageBase.h / MainPage.h pairing in this project.
//

#pragma once

#include "BlankPageBase.g.h"

namespace winrt::Simple::implementation
{
    struct BlankPageBase : BlankPageBaseT<BlankPageBase>
    {
        BlankPageBase() = default;

        hstring Foo() { return {}; }
        void OnLoaded(IInspectable const&, ::winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) {}
    };
}

namespace winrt::Simple::factory_implementation
{
    struct BlankPageBase : BlankPageBaseT<BlankPageBase, implementation::BlankPageBase>
    {
    };
}
