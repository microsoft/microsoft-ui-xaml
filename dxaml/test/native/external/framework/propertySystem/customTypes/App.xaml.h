// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "App.g.h"

namespace CustomTypes
{
    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    public ref class App sealed
    {
    public:
        App() {}
        virtual void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs^ args) override {}

    private:
        void OnSuspending(Platform::Object^ sender, ::Windows::ApplicationModel::SuspendingEventArgs^ e) {}
    };
}
