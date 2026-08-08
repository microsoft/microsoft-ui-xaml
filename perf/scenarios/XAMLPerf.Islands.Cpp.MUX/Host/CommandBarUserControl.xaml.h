#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "CommandBarUserControl.g.h"

namespace winrt::IslandSimple::implementation
{
    struct CommandBarUserControl : CommandBarUserControlT<CommandBarUserControl>
    {
        CommandBarUserControl();
        void appbar_Click(Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::IslandSimple::factory_implementation
{
    struct CommandBarUserControl : CommandBarUserControlT<CommandBarUserControl, implementation::CommandBarUserControl>
    {
    };
}
