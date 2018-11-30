// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DisplayRegionHelperTestApi.g.h"

class DisplayRegionHelperTestApi :
    public winrt::implementation::DisplayRegionHelperTestApiT<DisplayRegionHelperTestApi>
{
public:
    static bool SimulateDisplayRegions();
    static void SimulateDisplayRegions(bool value);

    static winrt::TwoPaneViewMode SimulateMode();
    static void SimulateMode(winrt::TwoPaneViewMode const& value);
};