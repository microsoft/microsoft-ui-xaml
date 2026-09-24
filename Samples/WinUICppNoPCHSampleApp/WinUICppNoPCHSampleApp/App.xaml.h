// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "App.xaml.g.h"

namespace winrt::WinUICppNoPCHSampleApp::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        winrt::Microsoft::UI::Xaml::Window window{ nullptr };
    };
}
