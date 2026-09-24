#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "TreeViewUserControl.g.h"

namespace winrt::IslandSimple::implementation
{
    struct TreeViewUserControl : TreeViewUserControlT<TreeViewUserControl>
    {
        TreeViewUserControl();
        void myButton_Click(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::IslandSimple::factory_implementation
{
    struct TreeViewUserControl : TreeViewUserControlT<TreeViewUserControl, implementation::TreeViewUserControl>
    {
    };
}
