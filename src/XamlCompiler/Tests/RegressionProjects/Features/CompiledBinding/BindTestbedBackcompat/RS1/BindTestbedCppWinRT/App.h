// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "App.xaml.g.h"

namespace winrt::BindTestbed::implementation
{
    struct App : public AppT<App>
    {
    public:
        App();

        static BindTestbedModel::DataModel Model;
        static BindTestbedModel::DOModel DOModel;
        static BindTestbedCXModel::ModelCX ModelCX;

        void OnLaunched(wux::LaunchActivatedEventArgs const& e);
        void OnSuspending(IInspectable const& sender, wa::SuspendingEventArgs const& e);
        void OnNavigationFailed(IInspectable const& sender, wux::Navigation::NavigationFailedEventArgs const& e);
    };
}