#include "pch.h"
#include "TestUserControl1.xaml.h"
#if __has_include("TestUserControl1.g.cpp")
#include "TestUserControl1.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::RuntimeComponentThatUsesMUX::implementation
{
    TestUserControl1::TestUserControl1()
    {
        InitializeComponent();
    }
}
