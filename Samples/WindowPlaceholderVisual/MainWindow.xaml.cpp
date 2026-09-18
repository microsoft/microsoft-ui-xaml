#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Dispatching;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;

namespace winrt::WindowPlaceholderVisual::implementation
{
    void MainWindow::BeginDelayedContent()
    {
        m_contentTimer = DispatcherQueue().CreateTimer();
        m_contentTimer.Interval(std::chrono::seconds(2));
        m_contentTimer.IsRepeating(false);

        auto weakThis = get_weak();
        m_contentTimer.Tick([weakThis](DispatcherQueueTimer const&, IInspectable const&)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->AttachFinalContent();
            }
        });
        m_contentTimer.Start();
    }

    void MainWindow::AttachFinalContent()
    {
        m_contentTimer.Stop();
        m_contentTimer = nullptr;

        auto root = Grid();
        root.Background(SolidColorBrush(Windows::UI::Color{ 0xff, 0x28, 0x0a, 0x50 }));

        auto panel = StackPanel();
        panel.HorizontalAlignment(HorizontalAlignment::Center);
        panel.VerticalAlignment(VerticalAlignment::Center);
        panel.Spacing(12);
        panel.Margin(Thickness{ 40 });

        auto heading = TextBlock();
        heading.Text(L"Contrasting content attached after a 2-second delay");
        heading.FontSize(26);
        heading.FontWeight(Windows::UI::Text::FontWeights::SemiBold());
        heading.TextWrapping(TextWrapping::Wrap);
        heading.Foreground(SolidColorBrush(Windows::UI::Colors::White()));
        panel.Children().Append(heading);

        const bool noRedirectionEnabled = g_noRedirectionMode == FeatureMode::On;
        const bool placeholderEnabled = g_placeholderMode == FeatureMode::On;
        const auto noRedirectionMode = noRedirectionEnabled ? L"ON" : L"OFF";
        const auto placeholderMode = placeholderEnabled ? L"ON" : L"OFF";
        const auto requestedTheme =
            Application::Current().RequestedTheme() == ApplicationTheme::Dark ? L"Dark" : L"Light";

        auto noRedirectionText = TextBlock();
        noRedirectionText.Text(hstring{
            std::wstring{ L"No-redirection change: " } + noRedirectionMode });
        noRedirectionText.FontSize(22);
        noRedirectionText.Foreground(
            SolidColorBrush(Windows::UI::Color{ 0xff, 0xff, 0xd7, 0x00 }));
        panel.Children().Append(noRedirectionText);

        auto placeholderText = TextBlock();
        placeholderText.Text(hstring{
            std::wstring{ L"Window placeholder change: " } + placeholderMode });
        placeholderText.FontSize(22);
        placeholderText.Foreground(
            SolidColorBrush(Windows::UI::Color{ 0xff, 0xff, 0xd7, 0x00 }));
        panel.Children().Append(placeholderText);

        const wchar_t* combination =
            noRedirectionEnabled
                ? (placeholderEnabled ? L"Combined mitigation" : L"No redirection only")
                : (placeholderEnabled ? L"Placeholder only" : L"Baseline");

        auto combinationText = TextBlock();
        combinationText.Text(hstring{ std::wstring{ L"Combination: " } + combination });
        combinationText.FontSize(20);
        combinationText.Foreground(SolidColorBrush(Windows::UI::Colors::White()));
        panel.Children().Append(combinationText);

        auto themeText = TextBlock();
        themeText.Text(L"Application requested theme: " + hstring{ requestedTheme });
        themeText.FontSize(18);
        themeText.Foreground(SolidColorBrush(Windows::UI::Colors::White()));
        panel.Children().Append(themeText);

        auto instructions = TextBlock();
        instructions.Text(hstring{
            L"Launch with --no-redirection=on|off --placeholder=on|off to compare all four combinations." });
        instructions.FontSize(18);
        instructions.TextWrapping(TextWrapping::Wrap);
        instructions.Foreground(SolidColorBrush(Windows::UI::Colors::White()));
        panel.Children().Append(instructions);

        root.Children().Append(panel);
        Content(root);
    }
}
