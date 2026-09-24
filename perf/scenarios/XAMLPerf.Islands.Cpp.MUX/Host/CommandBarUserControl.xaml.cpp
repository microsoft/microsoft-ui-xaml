#include "pch.h"
#include "CommandBarUserControl.xaml.h"
#if __has_include("CommandBarUserControl.g.cpp")
#include "CommandBarUserControl.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI and the WinUI project structure,
// see https://learn.microsoft.com/windows/apps/winui/winui3/

namespace winrt::IslandSimple::implementation
{
    CommandBarUserControl::CommandBarUserControl()
    {
        InitializeComponent();
    }

    void CommandBarUserControl::appbar_Click(IInspectable const&, RoutedEventArgs const&)
    {
    }
}
