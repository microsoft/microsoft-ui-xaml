#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::DisableXamlGeneratedMainNoCtorCpp::implementation
{
    void MainWindow::OnRootLoaded(
        winrt::Windows::Foundation::IInspectable const& /*sender*/,
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*args*/)
    {
        // Surface the marker recorded by App's parameterized constructor so that
        // automated tests can verify the app really launched through wWinMain and
        // the custom App(int) constructor.
        entryPointTextBlock().Text(g_launchMarker);
    }
}
