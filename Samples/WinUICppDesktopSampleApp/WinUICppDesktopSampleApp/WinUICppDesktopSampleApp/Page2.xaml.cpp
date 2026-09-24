// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "Page2.xaml.h"
#if __has_include("Page2.g.cpp")
#include "Page2.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and project templates, see the WinUI documentation.

namespace winrt::WinUICppDesktopSampleApp::implementation
{
    Page2::Page2()
    {
        InitializeComponent();
    }

    void Page2::navigationButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        Frame().Navigate(xaml_typename<WinUICppDesktopSampleApp::Page1>());
    }

    bool Page2::FooBool()
    {
        return _fooBool;
    }
    void Page2::FooBool(bool value)
    {
        _fooBool = value;
    }
}
