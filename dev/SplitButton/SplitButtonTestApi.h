// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SplitButtonTestApi.g.h"

class SplitButtonTestApi :
    public winrt::implementation::SplitButtonTestApiT<SplitButtonTestApi>
{
public:
    static bool SimulateTouch();
    static void SimulateTouch(bool value);
};
