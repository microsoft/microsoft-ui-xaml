// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MaterialHelper.h"
#include "MaterialHelperTestApiFactory.h"

bool MaterialHelperTestApi::IgnoreAreEffectsFast()
{
    return MaterialHelper::IgnoreAreEffectsFast();
}

void MaterialHelperTestApi::IgnoreAreEffectsFast(bool value)
{
    MaterialHelper::IgnoreAreEffectsFast(value);
}

bool MaterialHelperTestApi::SimulateDisabledByPolicy()
{
    return MaterialHelper::SimulateDisabledByPolicy();
}

void MaterialHelperTestApi::SimulateDisabledByPolicy(bool value)
{
    MaterialHelper::SimulateDisabledByPolicy(value);
}
