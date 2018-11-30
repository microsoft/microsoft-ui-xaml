// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MaterialHelperTestApi.g.h"

class MaterialHelperTestApi :
    public ReferenceTracker<MaterialHelperTestApi, winrt::implementation::MaterialHelperTestApiT, winrt::composable>
{
public:
    MaterialHelperTestApi();

    static bool SimulateDisabledByPolicy();
    static void SimulateDisabledByPolicy(bool value);
    static bool IgnoreAreEffectsFast();
    static void IgnoreAreEffectsFast(bool value);
};