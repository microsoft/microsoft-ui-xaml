// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "MapLayer.g.h"

class MapLayer :
    public winrt::implementation::MapLayerT<MapLayer, winrt::composable>
{
public:
    MapLayer();
    winrt::hstring Id;
};
