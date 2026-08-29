// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the AnotherBlankUserControl class.
//

#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Interop.h"
#include "AnotherBlankUserControl.g.h"

namespace winrt::StaticLibInRuntimeComponent::implementation
{
    struct AnotherBlankUserControl : AnotherBlankUserControlT<AnotherBlankUserControl>
    {
        AnotherBlankUserControl();

        int32_t Dummy();
        void Dummy(int32_t value);

        void ClickHandler(::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::StaticLibInRuntimeComponent::factory_implementation
{
    struct AnotherBlankUserControl : AnotherBlankUserControlT<AnotherBlankUserControl, implementation::AnotherBlankUserControl>
    {
    };
}
