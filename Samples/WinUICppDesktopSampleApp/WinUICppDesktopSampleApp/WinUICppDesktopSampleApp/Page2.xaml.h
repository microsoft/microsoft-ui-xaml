// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "Page1.xaml.h"
#include "Page2.g.h"

namespace winrt::WinUICppDesktopSampleApp::implementation
{
    struct Page2 : Page2T<Page2>
    {
        Page2();

        void navigationButton_Click(winrt::Windows::Foundation::IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);

        bool FooBool();
        void FooBool(bool value);

    private:
        bool _fooBool = true;
    };
}

namespace winrt::WinUICppDesktopSampleApp::factory_implementation
{
    struct Page2 : Page2T<Page2, implementation::Page2>
    {
    };
}
