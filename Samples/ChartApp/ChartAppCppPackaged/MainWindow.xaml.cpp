#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls::Charts;

namespace winrt::ChartAppCppPackaged::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        auto codeChart = Microsoft::UI::Xaml::Controls::Charts::Chart{};
        codeChart.Height(160);
        CodeChartHost().Children().Append(codeChart);
        StatusText().Text(L"Markup chart + code-behind chart OK");
    }

    void MainWindow::ToggleTheme_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto currentTheme = RootGrid().RequestedTheme();
        RootGrid().RequestedTheme(currentTheme == ElementTheme::Dark ? ElementTheme::Light : ElementTheme::Dark);
    }
}
