// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SplitButtonTestHelper.h"
#include "SplitButtonTestApi.h"

#include "SplitButtonTestApi.properties.cpp"

bool SplitButtonTestApi::SimulateTouch()
{
    return SplitButtonTestHelper::SimulateTouch();
}

void SplitButtonTestApi::SimulateTouch(bool value)
{
    SplitButtonTestHelper::SimulateTouch(value);
}
