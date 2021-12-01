// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ElementFactory.h"
#include "RecyclingElementFactory.g.h"
#include "RecyclingElementFactory.properties.h"

class RecyclingElementFactory :
    public ReferenceTracker<RecyclingElementFactory, winrt::implementation::RecyclingElementFactoryT, ElementFactory>,
    public RecyclingElementFactoryProperties
{
public:
    RecyclingElementFactory();

#pragma region IRecyclingElementFactory
    winrt::RecyclePool RecyclePool();
    void RecyclePool(winrt::RecyclePool const& value);

    winrt::IMap<winrt::hstring, winrt::DataTemplate> Templates();
    void Templates(winrt::IMap<winrt::hstring, winrt::DataTemplate> const& value);

#pragma endregion

#pragma region IRecyclingElementFactoryOverrides
    winrt::hstring OnSelectTemplateKeyCore(winrt::IInspectable const& dataContext, winrt::UIElement const& owner);
#pragma endregion

#pragma region IElementFactoryOverrides
    winrt::UIElement GetElementCore(winrt::ElementFactoryGetArgs const& args);
    void RecycleElementCore(winrt::ElementFactoryRecycleArgs const& args);
#pragma endregion

private:
    tracker_ref<winrt::RecyclePool> m_recyclePool{ this };
    tracker_ref<winrt::IMap<winrt::hstring, winrt::DataTemplate>> m_templates{ this };
    tracker_ref<winrt::SelectTemplateEventArgs> m_args{ this };
};
