#include "pch.h"
#include "TextBoxUserControl.xaml.h"
#if __has_include("TextBoxUserControl.g.cpp")
#include "TextBoxUserControl.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI and the WinUI project structure,
// see https://learn.microsoft.com/windows/apps/winui/winui3/

namespace winrt::IslandSimple::implementation
{
    TextBoxUserControl::TextBoxUserControl()
    {
        InitializeComponent();
    }
}
