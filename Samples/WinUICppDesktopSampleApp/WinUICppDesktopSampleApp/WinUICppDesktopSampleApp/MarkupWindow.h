// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime

#include "MarkupWindow.g.h"

#pragma pop_macro("GetCurrentTime")

namespace winrt::WinUICppDesktopSampleApp::implementation
{
    struct MarkupWindow : MarkupWindowT<MarkupWindow>
    {
        MarkupWindow();

        Microsoft::UI::Xaml::Controls::Button GetMainWindowActivateButton();

        void ButtonCloseWindow_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ButtonExitApplication_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::WinUICppDesktopSampleApp::factory_implementation
{
    struct MarkupWindow : MarkupWindowT<MarkupWindow, implementation::MarkupWindow>
    {
    };
}
