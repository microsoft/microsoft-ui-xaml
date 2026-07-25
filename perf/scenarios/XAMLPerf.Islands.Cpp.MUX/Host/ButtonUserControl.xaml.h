#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "ButtonUserControl.g.h"

namespace winrt::IslandSimple::implementation
{
    struct ButtonUserControl : ButtonUserControlT<ButtonUserControl>
    {
        ButtonUserControl();

        void myButton_Click(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::IslandSimple::factory_implementation
{
    struct ButtonUserControl : ButtonUserControlT<ButtonUserControl, implementation::ButtonUserControl>
    {
    };
}
