#include "pch.h"
#include "TreeViewUserControl.xaml.h"
#if __has_include("TreeViewUserControl.g.cpp")
#include "TreeViewUserControl.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI and the WinUI project structure,
// see https://learn.microsoft.com/windows/apps/winui/winui3/

namespace winrt::IslandSimple::implementation
{
    TreeViewUserControl::TreeViewUserControl()
    {
        InitializeComponent();
    }

    void TreeViewUserControl::myButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
    }
}
