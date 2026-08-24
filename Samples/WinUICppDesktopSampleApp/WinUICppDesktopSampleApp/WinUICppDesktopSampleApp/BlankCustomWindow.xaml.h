// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "BlankCustomWindow.g.h"

namespace winrt::WinUICppDesktopSampleApp::implementation
{
    struct BlankCustomWindow : BlankCustomWindowT<BlankCustomWindow>
    {
        BlankCustomWindow();
        ~BlankCustomWindow();
        void ButtonCloseWindow_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonTestLonelyWebView2_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::WinUICppDesktopSampleApp::factory_implementation
{
    struct BlankCustomWindow : BlankCustomWindowT<BlankCustomWindow, implementation::BlankCustomWindow>
    {
    };
}
