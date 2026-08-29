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

void MapElementsLayer::SetParentMapControl(const winrt::MapControl& parent)
{
    m_parentMapControl = winrt::make_weak(parent);
}

void  MapElementsLayer::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
}
void MapElementsLayer::RaiseMapElementClick(MapElementClickEventArgs const& args)
{
    m_mapElementClickEventSource(*this, args);
}

winrt::fire_and_forget MapElementsLayer::HandleMapElementsChanged(const winrt::IObservableVector<winrt::MapElement>& sender, const winrt::Collections::IVectorChangedEventArgs& args)
{
    const auto index = args.Index();
    if (const auto& mapControl = m_parentMapControl.get())
    {
        auto mapControlImpl = winrt::get_self<MapControl>(mapControl);
        switch (args.CollectionChange())
        {
            case winrt::Collections::CollectionChange::ItemInserted:
            {
                co_await mapControlImpl->LayerElementsChanged(*this, MapElements(), args, L"");
                m_elementIds.Append(winrt::get_self<MapElement>(MapElements().GetAt(index))->Id);
                break;
            }
            case winrt::Collections::CollectionChange::ItemRemoved:
            {
                co_await mapControlImpl->LayerElementsChanged(*this, MapElements(), args, m_elementIds.GetAt(index));
                m_elementIds.RemoveAt(index);
                break;
            }
            case winrt::Collections::CollectionChange::ItemChanged:
            {
                auto elements = MapElements();
                co_await mapControlImpl->LayerElementsChanged(*this, elements, args, m_elementIds.GetAt(index));
                m_elementIds.SetAt(index, winrt::get_self<MapElement>(elements.GetAt(index))->Id);
                break;
            }
            case winrt::Collections::CollectionChange::Reset:
            {
                auto elements = MapElements();
                co_await mapControlImpl->LayerElementsChanged(*this, elements, args, L"");
                m_elementIds.Clear();
                for (uint32_t i = 0; i < elements.Size(); i++)
                {
                    auto elementId = winrt::get_self<MapElement>(elements.GetAt(i))->Id;
                    m_elementIds.Append(elementId);
                }
                break;
            }
        }
    }
}

void MapElementsLayer::OnMapElementsVectorChanged(const winrt::IObservableVector<winrt::MapElement>& sender, const winrt::Collections::IVectorChangedEventArgs& args)
{
    HandleMapElementsChanged(sender, args);
}
