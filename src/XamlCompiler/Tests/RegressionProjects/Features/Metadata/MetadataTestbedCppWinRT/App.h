// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "App.xaml.g.h"

namespace winrt::MetadataTestbedCppWinRT::implementation
{
    struct App : public AppT<App>
    {
    public:
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);
        void OnNavigationFailed(IInspectable const&, Microsoft::UI::Xaml::Navigation::NavigationFailedEventArgs const&);

    private:
        Microsoft::UI::Xaml::Window m_window{ nullptr };
    };
}
