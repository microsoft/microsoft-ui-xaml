// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "DisplayRegionHelper.h"
#include "DisplayRegionHelperTestApi.h"
#include "common.h"

bool DisplayRegionHelperTestApi::SimulateDisplayRegions()
{
    return DisplayRegionHelper::SimulateDisplayRegions();
}

void DisplayRegionHelperTestApi::SimulateDisplayRegions(bool value)
{
    DisplayRegionHelper::SimulateDisplayRegions(value);
}

winrt::TwoPaneViewMode DisplayRegionHelperTestApi::SimulateMode()
{
    return DisplayRegionHelper::SimulateMode();
}

void DisplayRegionHelperTestApi::SimulateMode(winrt::TwoPaneViewMode const& value)
{
    DisplayRegionHelper::SimulateMode(value);
}
