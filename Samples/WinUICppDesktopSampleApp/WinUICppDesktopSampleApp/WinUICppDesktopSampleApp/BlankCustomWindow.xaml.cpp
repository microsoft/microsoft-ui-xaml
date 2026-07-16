// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "BlankCustomWindow.xaml.h"
#if __has_include("BlankCustomWindow.g.cpp")
#include "BlankCustomWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Documents;

// To learn more about WinUI, the WinUI project structure,
// and project templates, see the WinUI documentation.

namespace winrt::WinUICppDesktopSampleApp::implementation
{
    BlankCustomWindow::BlankCustomWindow()
    {
        InitializeComponent();
    }

    BlankCustomWindow::~BlankCustomWindow()
    {
        Application::Current().Exit();
    }

    void BlankCustomWindow::ButtonCloseWindow_Click(IInspectable const&, RoutedEventArgs const&)
    {
        Close();
    }

    // Set up root for testing tab navigation when WebView2 is the only focusable element
    void BlankCustomWindow::ButtonTestLonelyWebView2_Click(IInspectable const& sender, RoutedEventArgs const& /*args*/)
    {
        StackPanel containerSP = (sender.as<FrameworkElement>().FindName(L"containerSP")).as<StackPanel>();
        containerSP.Children().Clear();

        TextBlock myTextBlock = TextBlock();
        myTextBlock.FontSize(18.0);
        myTextBlock.Text(L"Since WebView2 is the only focusable element, tabbing should cycle through the web content and \"wrap around\" when reaching the end...");
        containerSP.Children().Append(myTextBlock);

        WebView2 myWebView2 = WebView2();
        myWebView2.Width(640);
        myWebView2.Height(480);
        containerSP.Children().Append(myWebView2);
        winrt::Windows::Foundation::Uri uri{ L"http://www.microsoft.com" };
        myWebView2.Source(uri);
    }
}
