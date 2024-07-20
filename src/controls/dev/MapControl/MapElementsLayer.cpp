// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MapElementsLayer.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "MapElementClickEventArgs.h"
#include "MapControl.h"
#include "Vector.h"
#include "MapElement.h"
#include "MapControl.h"

MapElementsLayer::MapElementsLayer()
{
    auto elements = winrt::make<Vector<winrt::MapElement>>();
    auto observableVector = elements.try_as<winrt::IObservableVector<winrt::MapElement>>();
    observableVector.VectorChanged({ this, &MapElementsLayer::OnMapElementsVectorChanged });
    SetValue(s_MapElementsProperty, elements);

    m_elementIds = winrt::make<Vector<winrt::hstring>>();

}

void  MapElementsLayer::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
}
void MapElementsLayer::RaiseMapElementClick(MapElementClickEventArgs const& args)
{
    m_mapElementClickEventSource(*this, args);
}

void MapElementsLayer::OnMapElementsVectorChanged(const winrt::IObservableVector<winrt::MapElement>& sender, const winrt::Collections::IVectorChangedEventArgs& args)
{
    const auto index = args.Index();
    auto map = winrt::get_self<MapControl>(SharedHelpers::GetAncestorOfType<winrt::MapControl>(winrt::VisualTreeHelper::GetParent(*this)));
    if (map)
    {
        if (index)
        {
            map->LayerElementsChanged(*this, sender, args, m_elementIds.GetAt(index));
        }
        else 
        {
            map->LayerElementsChanged(*this, sender, args, L"");
        }
    }

    switch (args.CollectionChange())
    {
    case winrt::Collections::CollectionChange::ItemInserted:
        m_elementIds.Append(winrt::get_self<MapElement>(sender.GetAt(index))->Id);
        break;
    case winrt::Collections::CollectionChange::ItemRemoved:
        m_elementIds.RemoveAt(index);
        break;
    case winrt::Collections::CollectionChange::ItemChanged:
        m_elementIds.SetAt(index, winrt::get_self<MapElement>(sender.GetAt(index))->Id);
        break;
    case winrt::Collections::CollectionChange::Reset:
    {
        m_elementIds.Clear();
        for (uint32_t i = 0; i < sender.Size(); i++)
        {
            auto elementId = winrt::get_self<MapElement>(sender.GetAt(i))->Id;
            m_elementIds.Append(elementId);
        }
        break;
    }
    }
}