#include "pch.h"
#include "UserControl.xaml.h"
#if __has_include("UserControl.g.cpp")
#include "UserControl.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI and the WinUI project structure,
// see https://learn.microsoft.com/windows/apps/winui/winui3/

namespace winrt::IslandSimple::implementation
{
    UserControl::UserControl()
    {
        InitializeComponent();
    }

    int32_t UserControl::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void UserControl::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

}
