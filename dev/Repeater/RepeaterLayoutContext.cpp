// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "RepeaterLayoutContext.h"
#include "ItemsRepeater.h"

RepeaterLayoutContext::RepeaterLayoutContext(const winrt::ItemsRepeater& owner)
{
    m_owner = winrt::make_weak(owner);
}

#pragma region ILayoutContext

int32_t RepeaterLayoutContext::ItemCountCore()
{
    auto dataSource = GetOwner().ItemsSourceView();
    if (dataSource)
    {
        return dataSource.Count();
    }
    return 0;
}

winrt::UIElement RepeaterLayoutContext::GetElementAtCore(int index, winrt::ElementRealizationOptions const& options)
{
    return winrt::get_self<ItemsRepeater>(GetOwner())->GetElementImpl(index, 
        (options & winrt::ElementRealizationOptions::ForceCreate) == winrt::ElementRealizationOptions::ForceCreate,
        (options & winrt::ElementRealizationOptions::SuppressAutoRecycle) == winrt::ElementRealizationOptions::SuppressAutoRecycle);
}

winrt::IInspectable RepeaterLayoutContext::LayoutStateCore()
{
    return winrt::get_self<ItemsRepeater>(GetOwner())->LayoutState();
}

void RepeaterLayoutContext::LayoutStateCore(winrt::IInspectable const& value)
{
    winrt::get_self<ItemsRepeater>(GetOwner())->LayoutState(value);
}

#pragma endregion

#pragma region IVirtualizingLayoutContextOverrides

winrt::IInspectable RepeaterLayoutContext::GetItemAtCore(
    int index)
{
    return GetOwner().ItemsSourceView().GetAt(index);
}

void RepeaterLayoutContext::RecycleElementCore(winrt::UIElement const& element)
{
    auto owner = winrt::get_self<ItemsRepeater>(GetOwner());
    REPEATER_TRACE_INFO(L"RepeaterLayout - RecycleElement: %d \n", owner->GetElementIndex(element));
    owner->ClearElementImpl(element);
}

winrt::Rect RepeaterLayoutContext::RealizationRectCore()
{
    return winrt::get_self<ItemsRepeater>(GetOwner())->RealizationWindow();
}

int32_t RepeaterLayoutContext::RecommendedAnchorIndexCore()
{
    int anchorIndex = -1;
    auto repeater = winrt::get_self<ItemsRepeater>(GetOwner());
    auto anchor = repeater->SuggestedAnchor();
    if (anchor)
    {
        anchorIndex = repeater->GetElementIndex(anchor);
    }

    return anchorIndex;
}

winrt::Point RepeaterLayoutContext::LayoutOriginCore()
{
    return winrt::get_self<ItemsRepeater>(GetOwner())->LayoutOrigin();
}

void RepeaterLayoutContext::LayoutOriginCore(winrt::Point const& value)
{
    winrt::get_self<ItemsRepeater>(GetOwner())->LayoutOrigin(value);
}

#pragma endregion

winrt::ItemsRepeater RepeaterLayoutContext::GetOwner()
{
    return m_owner.get();
}