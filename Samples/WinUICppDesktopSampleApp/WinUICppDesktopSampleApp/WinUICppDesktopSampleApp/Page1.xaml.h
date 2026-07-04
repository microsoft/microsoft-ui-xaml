// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Page1.g.h"

namespace winrt::WinUICppDesktopSampleApp::implementation
{
    struct Page1 : Page1T<Page1>
    {
        Page1();
        ~Page1();

        void myButton_Click(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void navigationButton_Click(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void missingPageNavigationButton_Click(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::WinUICppDesktopSampleApp::factory_implementation
{
    struct Page1 : Page1T<Page1, implementation::Page1>
    {
    };
}
