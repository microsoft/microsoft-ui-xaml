// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the BlankUserControl class.
//

#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Interop.h"
#include "winrt/Microsoft.UI.Xaml.Controls.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "BlankUserControl.g.h"

namespace winrt::StaticControlsLib::implementation
{
    struct BlankUserControl : BlankUserControlT<BlankUserControl>
    {
        BlankUserControl();

        int32_t Dummy();
        void Dummy(int32_t value);

        void ClickHandler(::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::StaticControlsLib::factory_implementation
{
    struct BlankUserControl : BlankUserControlT<BlankUserControl, implementation::BlankUserControl>
    {
    };
}
