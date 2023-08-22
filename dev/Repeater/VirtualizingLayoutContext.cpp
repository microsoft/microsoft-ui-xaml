// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "VirtualizingLayoutContext.h"
#include "VirtualLayoutContextAdapter.h"

#include "VirtualizingLayoutContext.properties.cpp"

#pragma region IVirtualizingLayoutContext

int32_t VirtualizingLayoutContext::ItemCount()
{
    return overridable().ItemCountCore();
}

winrt::IInspectable VirtualizingLayoutContext::GetItemAt(int index)
{
    return overridable().GetItemAtCore(index);
}

winrt::UIElement VirtualizingLayoutContext::GetOrCreateElementAt(int index)
{
    // Calling this way because GetOrCreateElementAtCore is ambiguous.
    // Use .as instead of try_as because try_as uses non-delegating inner and we need to call the outer for overrides.
    return get_strong().as<winrt::IVirtualizingLayoutContextOverrides>().GetOrCreateElementAtCore(index, winrt::ElementRealizationOptions::None);
}

winrt::UIElement VirtualizingLayoutContext::GetOrCreateElementAt(int index, winrt::ElementRealizationOptions const& options)
{
    // Calling this way because GetOrCreateElementAtCore is ambiguous.
    // Use .as instead of try_as because try_as uses non-delegating inner and we need to call the outer for overrides.
    return get_strong().as<winrt::IVirtualizingLayoutContextOverrides>().GetOrCreateElementAtCore(index, options);
}

void VirtualizingLayoutContext::RecycleElement(winrt::UIElement const& element)
{
    overridable().RecycleElementCore(element);
}

winrt::Rect VirtualizingLayoutContext::RealizationRect()
{
    return overridable().RealizationRectCore();
}

int32_t VirtualizingLayoutContext::RecommendedAnchorIndex()
{
    return overridable().RecommendedAnchorIndexCore();
}

winrt::Point VirtualizingLayoutContext::LayoutOrigin()
{
    return overridable().LayoutOriginCore();
}

void VirtualizingLayoutContext::LayoutOrigin(winrt::Point const& value)
{
    overridable().LayoutOriginCore(value);
}

#pragma endregion

#pragma region IVirtualizingLayoutContextOverrides

winrt::IInspectable VirtualizingLayoutContext::GetItemAtCore(int /*index*/)
{
    throw winrt::hresult_not_implemented();
}

winrt::UIElement VirtualizingLayoutContext::GetOrCreateElementAtCore(int /*index*/, winrt::ElementRealizationOptions const& /* options */)
{
    throw winrt::hresult_not_implemented();
}

void VirtualizingLayoutContext::RecycleElementCore(winrt::UIElement const& /*element*/)
{
    throw winrt::hresult_not_implemented();
}

winrt::Rect VirtualizingLayoutContext::RealizationRectCore()
{
    throw winrt::hresult_not_implemented();
}

int32_t VirtualizingLayoutContext::RecommendedAnchorIndexCore()
{
    throw winrt::hresult_not_implemented();
}

winrt::Point VirtualizingLayoutContext::LayoutOriginCore()
{
    throw winrt::hresult_not_implemented();
}

void VirtualizingLayoutContext::LayoutOriginCore(winrt::Point const& /*value*/)
{
    throw winrt::hresult_not_implemented();
}

int32_t VirtualizingLayoutContext::ItemCountCore()
{
    throw winrt::hresult_not_implemented();
}

#pragma endregion

winrt::NonVirtualizingLayoutContext VirtualizingLayoutContext::GetNonVirtualizingContextAdapter()
{
    if (!m_contextAdapter)
    {
        m_contextAdapter = winrt::make<VirtualLayoutContextAdapter>(get_strong().as<winrt::VirtualizingLayoutContext>());
    }

    return m_contextAdapter;
}
