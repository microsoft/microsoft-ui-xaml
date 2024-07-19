// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "MapElement.h"
#include "MapElementClickEventArgs.g.h"
#include <winrt/Windows.Devices.Geolocation.h>


class MapElementClickEventArgs :
    public winrt::implementation::MapElementClickEventArgsT<MapElementClickEventArgs>
{
public:

    MapElementClickEventArgs(winrt::Geopoint location, winrt::MapElement element) :
        m_location(location), m_element(element) {};

    winrt::Geopoint Location() { return m_location; };
    winrt::MapElement Element() { return m_element; };

private:
    winrt::Geopoint m_location;
    winrt::MapElement m_element;
};
