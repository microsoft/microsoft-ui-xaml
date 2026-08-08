// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "Page1.xaml.h"
#if __has_include("Page1.g.cpp")
#include "Page1.g.cpp"
#endif
#include "Page2.xaml.h"

#include "ShutdownOrderValidation.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and project templates, see the WinUI documentation.

namespace winrt::WinUICppDesktopSampleApp::implementation
{
    Page1::Page1()
    {
        InitializeComponent();

        Unloaded([](auto&&, auto&&) {
            ShutdownOrderValidation::Log(L"Page1 Unloaded event raised.");
            });
    }

    Page1::~Page1()
    {
        ShutdownOrderValidation::Log(L"Page1::~Page1 called.");
    }

    void Page1::myButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        myButton().Content(box_value(L"Clicked"));
        TestTextBlock().Text(L"Clicked");
    }

    void Page1::navigationButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        Frame().Navigate(xaml_typename<WinUICppDesktopSampleApp::Page2>());
    }

    void Page1::missingPageNavigationButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        Microsoft::UI::Xaml::Controls::Page newContent;
        // Note: There is no resource with the URI "ms-appx:///786B1897-FEB4-4FC8-9796-9F7DD21582CC.xaml", but because
        // we set a custom ResourceManager for the app to handle missing resources this
        // shouldn't raise an error
        ::winrt::Windows::Foundation::Uri resourceLocator{ L"ms-appx:///786B1897-FEB4-4FC8-9796-9F7DD21582CC.xaml" };
        Application::LoadComponent(newContent, resourceLocator);
        Frame().Content(newContent);
    }
}
