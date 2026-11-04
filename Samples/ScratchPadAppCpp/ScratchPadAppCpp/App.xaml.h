// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "App.xaml.g.h"

namespace winrt::ScratchPadAppCpp::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        Microsoft::UI::Xaml::Window m_window{ nullptr };
    };
}
