// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "VirtualizingLayoutContext.h"
#include "LayoutContextAdapter.h"

LayoutContextAdapter::LayoutContextAdapter(winrt::NonVirtualizingLayoutContext const& nonVirtualizingContext)
{
    m_nonVirtualizingContext = winrt::make_weak(nonVirtualizingContext);
}

#pragma region ILayoutContextOverrides

winrt::IInspectable LayoutContextAdapter::LayoutStateCore()
{
    if (auto context = m_nonVirtualizingContext.get())
    {
        return context.LayoutState();
    }
    return nullptr;
}

void LayoutContextAdapter::LayoutStateCore(winrt::IInspectable const& state)
{
    if (auto context = m_nonVirtualizingContext.get())
    {
        context.LayoutStateCore(state);
    }
}

#pragma endregion

#pragma region IVirtualizingLayoutContextOverrides

int32_t LayoutContextAdapter::ItemCountCore()
{
    if (auto context = m_nonVirtualizingContext.get())
    {
        return context.Children().Size();
    }
    return 0;
}

winrt::IInspectable LayoutContextAdapter::GetItemAtCore(int index)
{
    if (auto context = m_nonVirtualizingContext.get())
    {
        return context.Children().GetAt(index);
    }
    return nullptr;
}

winrt::UIElement LayoutContextAdapter::GetOrCreateElementAtCore(int index, winrt::ElementRealizationOptions const& options)
{
    if (auto context = m_nonVirtualizingContext.get())
    {
        return context.Children().GetAt(index);
    }
    return nullptr;
}

void LayoutContextAdapter::RecycleElementCore(winrt::UIElement const& element)
{

}

int32_t LayoutContextAdapter::GetElementIndexCore(winrt::UIElement const& element)
{
    if (auto context = m_nonVirtualizingContext.get())
    {
        auto children = context.Children();
        for (unsigned int i = 0; i < children.Size(); i++)
        {
            if (children.GetAt(i) == element)
            {
                return i;
            }
        }
    }
    
    return -1;
}

winrt::Rect LayoutContextAdapter::RealizationRectCore()
{
    return winrt::Rect{ 0, 0, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };
}

int LayoutContextAdapter::RecommendedAnchorIndexCore()
{
    return -1;
}

winrt::Point LayoutContextAdapter::LayoutOriginCore()
{
    return winrt::Point(0, 0);
}

void LayoutContextAdapter::LayoutOriginCore(winrt::Point const& value)
{
    if (value != winrt::Point(0, 0))
    {
        throw winrt::hresult_invalid_argument(L"LayoutOrigin must be at (0,0) when RealizationRect is infinite sized.");
    }
}

#pragma endregion
