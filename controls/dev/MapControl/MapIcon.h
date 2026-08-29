// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "MapIcon.g.h"
#include "MapIcon.properties.h"
#include "MapElement.h"

class MapIcon :
    public ReferenceTracker<MapIcon, winrt::implementation::MapIconT, MapElement>,
    public MapIconProperties
{

public:
    MapIcon();

    
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
};
