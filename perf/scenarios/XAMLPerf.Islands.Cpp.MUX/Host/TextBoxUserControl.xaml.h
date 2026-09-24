#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "TextBoxUserControl.g.h"

namespace winrt::IslandSimple::implementation
{
    struct TextBoxUserControl : TextBoxUserControlT<TextBoxUserControl>
    {
        TextBoxUserControl();
    };
}

namespace winrt::IslandSimple::factory_implementation
{
    struct TextBoxUserControl : TextBoxUserControlT<TextBoxUserControl, implementation::TextBoxUserControl>
    {
    };
}
