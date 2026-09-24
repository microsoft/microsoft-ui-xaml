#include "pch.h"
#include "ExampleUserControl.xaml.h"
#if __has_include("ExampleUserControl.g.cpp")
#include "ExampleUserControl.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI and the WinUI project structure,
// see https://learn.microsoft.com/windows/apps/winui/winui3/

namespace winrt::IslandSimple::implementation
{
    ExampleUserControl::ExampleUserControl()
    {
        InitializeComponent();
    }

    void ExampleUserControl::myButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
    }
}
