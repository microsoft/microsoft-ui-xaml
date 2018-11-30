// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlControlsResources.g.h"

class ThemeResources :
    public ReferenceTracker<ThemeResources, winrt::implementation::XamlControlsResourcesT, winrt::composable>
{
public:
    ThemeResources();

    static void EnsureRevealLights(winrt::UIElement const& element);
};