// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "App.xaml.g.h"

namespace winrt::ConditionalsCppWinRT::implementation
{
    struct App : public AppT<App>
    {
    public:
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);
        void OnSuspending(IInspectable const&, ::Windows::ApplicationModel::SuspendingEventArgs const&);
        void OnNavigationFailed(IInspectable const&, Microsoft::UI::Xaml::Navigation::NavigationFailedEventArgs const&);
    };
}
