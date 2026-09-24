#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI and the WinUI project structure,
// see https://learn.microsoft.com/windows/apps/winui/winui3/

namespace winrt::XAMLPerf_HelloWorldApp_Cpp_MUX::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
    }
}
