// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "App.g.h"
#include "CustomUserControl.h"

namespace CustomTypes
{
    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    public ref class App sealed
    {
    public:
        virtual void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs^ args) override;
        App();

    private:
        void OnSuspending(Platform::Object^ sender, ::Windows::ApplicationModel::SuspendingEventArgs^ args);
        void OnNavigationFailed(Platform::Object ^sender, Microsoft::UI::Xaml::Navigation::NavigationFailedEventArgs ^args);
    };
}