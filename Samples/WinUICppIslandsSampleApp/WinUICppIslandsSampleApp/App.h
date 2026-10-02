// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <atomic>

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

        static uint32_t InstanceCount()
        {
            return s_instanceCount.load();
        }

        winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager m_initialWindowsXamlManager{ nullptr };

    private:
        static inline std::atomic_uint32_t s_instanceCount{ 0 };
    };
}
