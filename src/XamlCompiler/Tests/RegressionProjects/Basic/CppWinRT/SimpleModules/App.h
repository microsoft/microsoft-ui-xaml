// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#ifndef WINRT_IMPORT_MODULE
#include "App.xaml.g.h"
#endif

namespace winrt::Simple::implementation
{
    struct App : public AppT<App>
    {
    public:
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);
        void OnSuspending(IInspectable const&, ::winrt::Windows::ApplicationModel::SuspendingEventArgs const&);
        void OnNavigationFailed(IInspectable const&, Microsoft::UI::Xaml::Navigation::NavigationFailedEventArgs const&);
    };
}
