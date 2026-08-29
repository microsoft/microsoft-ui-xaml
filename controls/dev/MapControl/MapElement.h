// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "MapElement.g.h"

class MapElement :
    public winrt::implementation::MapElementT<MapElement, winrt::composable>
{
public:
    MapElement();
    winrt::hstring Id;
};
