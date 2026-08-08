// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime

#include "App.xaml.g.h"
#include "CustomMediaPlayerElement.h"

#pragma pop_macro("GetCurrentTime")

namespace winrt::WinUICppDesktopSampleApp::implementation
{
    struct App : AppT<App>
    {
        App();
        ~App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        winrt::Microsoft::UI::Xaml::Window window{ nullptr };
    };
}
