// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "App.g.h"

namespace TaefHostApp
{
    ref class App sealed
    {
    protected:
        virtual void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs^ e) override;

    internal:
        App();
    };
}
