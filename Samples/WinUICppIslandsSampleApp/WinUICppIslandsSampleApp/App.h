// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime

#include "App.xaml.g.h"

#pragma pop_macro("GetCurrentTime")

namespace winrt::WinUICppIslandsSampleApp::implementation
{
    struct App : AppT<App>
    {
        App();
        ~App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

        winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager m_initialWindowsXamlManager{ nullptr };
        static inline bool s_hasExistedInProcess{ false };
    };
}
