#include <windows.h>

#define WINRT_IMPORT_MODULE
import winrt.Windows.Foundation;
import winrt.Microsoft.UI.Xaml;

#include "MainWindow.xaml.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

namespace winrt::XamlCppWinRTModulesSample::implementation
{
    MainWindow::MainWindow()
        : m_model(Models::GreetingModel())
    {
        InitializeComponent();
    }
}
