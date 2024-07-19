﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//
// App.xaml.h
// Declaration of the App class.
//

#pragma once

#include "App.g.h"

using namespace Microsoft::UI::Xaml::Controls;

namespace TestAppCX
{
    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    ref class App sealed
    {
    protected:
        virtual void OnLaunched(Microsoft::UI::Xaml::Microsoft.UI.Xaml.Microsoft::UI::Xaml::LaunchActivatedEventArgs^ e) override;

    internal:
        App();

    internal:
        Frame^ RootFrame;

    private:
        void OnSuspending(Platform::Object^ sender, ::Windows::ApplicationModel::SuspendingEventArgs^ e);
        void OnNavigationFailed(Platform::Object ^sender, Microsoft::UI::Xaml::Navigation::NavigationFailedEventArgs ^e);
    };
}
