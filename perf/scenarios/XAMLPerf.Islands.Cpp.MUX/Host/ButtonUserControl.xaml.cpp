#include "pch.h"
#include "ButtonUserControl.xaml.h"
#if __has_include("ButtonUserControl.g.cpp")
#include "ButtonUserControl.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI and the WinUI project structure,
// see https://learn.microsoft.com/windows/apps/winui/winui3/

namespace winrt::IslandSimple::implementation
{
    ButtonUserControl::ButtonUserControl()
    {
        InitializeComponent();
    }

    void ButtonUserControl::myButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        myButton().Content(box_value(L"Clicked"));
    }
}
