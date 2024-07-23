// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "MapElementsLayer.g.h"
#include "MapElementsLayer.properties.h"
#include "MapLayer.h"
#include "MapElementClickEventArgs.h"
#include "MapElement.h"

class MapElementsLayer :
    public ReferenceTracker<MapElementsLayer, winrt::implementation::MapElementsLayerT, MapLayer>,
    public MapElementsLayerProperties
{

public:
    MapElementsLayer();

    winrt::IVector<winrt::hstring> m_elementIds;

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void RaiseMapElementClick(MapElementClickEventArgs const& args);
    void OnMapElementsVectorChanged(const winrt::IObservableVector<winrt::MapElement>&, const winrt::Collections::IVectorChangedEventArgs& args);
    void SetParentMapControl(const winrt::MapControl& parent);

private:
    winrt::weak_ref<winrt::MapControl> m_parentMapControl{ nullptr };
    winrt::fire_and_forget HandleMapElementsChanged(const winrt::IObservableVector<winrt::MapElement>&, const winrt::Collections::IVectorChangedEventArgs& args);

};
