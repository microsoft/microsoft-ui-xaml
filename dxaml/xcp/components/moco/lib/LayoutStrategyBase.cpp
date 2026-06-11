// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Contains common logic between the stacking and wrapping layout
// strategies.

#include "precomp.h"
#include "LayoutStrategyBase.h"

namespace DirectUI { namespace Components { namespace Moco {

float wf::Point::* 
LayoutStrategyBase::PointInNonVirtualizingDirection() const
{
    switch (m_virtualizationDirection)
    {
    case xaml_controls::Orientation_Horizontal:
        return &wf::Point::Y;
    case xaml_controls::Orientation_Vertical:
        return &wf::Point::X;
    default:
        XAML_FAIL_FAST();
        return nullptr;
    }
}

float wf::Point::* 
LayoutStrategyBase::PointInVirtualizingDirection() const
{
    switch (m_virtualizationDirection)
    {
    case xaml_controls::Orientation_Horizontal:
        return &wf::Point::X;
    case xaml_controls::Orientation_Vertical:
        return &wf::Point::Y;
    default:
        XAML_FAIL_FAST();
        return nullptr;
    }
}

float wf::Size::* 
LayoutStrategyBase::SizeInNonVirtualizingDirection() const
{
    switch (m_virtualizationDirection)
    {
    case xaml_controls::Orientation_Horizontal:
        return &wf::Size::Height;
    case xaml_controls::Orientation_Vertical:
        return &wf::Size::Width;
    default:
        XAML_FAIL_FAST();
        return nullptr;
    }
}

float wf::Size::* 
LayoutStrategyBase::SizeInVirtualizingDirection() const
{
    switch (m_virtualizationDirection)
    {
    case xaml_controls::Orientation_Horizontal:
        return &wf::Size::Width;
    case xaml_controls::Orientation_Vertical:
        return &wf::Size::Height;
    default:
        XAML_FAIL_FAST();
        return nullptr;
    }
}

float wf::Rect::*
LayoutStrategyBase::PointFromRectInNonVirtualizingDirection() const
{
    switch (m_virtualizationDirection)
    {
    case xaml_controls::Orientation_Horizontal:
        return &wf::Rect::Y;
    case xaml_controls::Orientation_Vertical:
        return &wf::Rect::X;
    default:
        XAML_FAIL_FAST();
        return nullptr;
    }
}

float wf::Rect::* 
LayoutStrategyBase::PointFromRectInVirtualizingDirection() const
{
    switch (m_virtualizationDirection)
    {
    case xaml_controls::Orientation_Horizontal:
        return &wf::Rect::X;
    case xaml_controls::Orientation_Vertical:
        return &wf::Rect::Y;
    default:
        XAML_FAIL_FAST();
        return nullptr;
    }
}

float wf::Rect::* 
LayoutStrategyBase::SizeFromRectInNonVirtualizingDirection() const
{
    switch (m_virtualizationDirection)
    {
    case xaml_controls::Orientation_Horizontal:
        return &wf::Rect::Height;
    case xaml_controls::Orientation_Vertical:
        return &wf::Rect::Width;
    default:
        XAML_FAIL_FAST();
        return nullptr;
    }
}

float wf::Rect::* 
LayoutStrategyBase::SizeFromRectInVirtualizingDirection() const
{
    switch (m_virtualizationDirection)
    {
    case xaml_controls::Orientation_Horizontal:
        return &wf::Rect::Width;
    case xaml_controls::Orientation_Vertical:
        return &wf::Rect::Height;
    default:
        XAML_FAIL_FAST();
        return nullptr;
    }
}

wf::Size LayoutStrategyBase::GetGroupPaddingAtStart() const
{
    return wf::Size { static_cast<float>(m_groupPadding.Left), static_cast<float>(m_groupPadding.Top) };
}

wf::Size LayoutStrategyBase::GetGroupPaddingAtEnd() const
{
    return wf::Size { static_cast<float>(m_groupPadding.Right), static_cast<float>(m_groupPadding.Bottom) };
}

//static
int LayoutStrategyBase::GetRemainingGroups(
    _In_ int referenceGroupIndex,
    _In_ int totalGroups,
    _In_ RelativePosition positionOfReference)
{
    ASSERT(0 <= totalGroups);
    ASSERT(0 <= referenceGroupIndex && referenceGroupIndex <= totalGroups);

    switch (positionOfReference)
    {
    case RelativePosition::Before:
        // Our reference is before the region we're counting
        // Notice no "count-1" here. Groups and containers after and including our reference are still considered valid traversal candidates
        return totalGroups - referenceGroupIndex;
        break;

    case RelativePosition::After:
        return referenceGroupIndex;
        break;

    default:
        return 0;
        break;
    }
}

//static
int LayoutStrategyBase::GetRemainingItems(
    _In_ int referenceItemIndex,
    _In_ int totalItems,
    _In_ RelativePosition positionOfReference)
{
    ASSERT(0 <= totalItems);
    ASSERT(0 <= referenceItemIndex && referenceItemIndex <= totalItems);

    switch (positionOfReference)
    {
    case RelativePosition::Before:
        // Our reference is before the region we're counting
        // Notice no "count-1" here. Groups and containers after and including our reference are still considered valid traversal candidates
        return totalItems - referenceItemIndex;
        break;

    case RelativePosition::After:
        return referenceItemIndex;
        break;

    default:
        return 0;
        break;
    }
}

// Determine if a point is inside the window, or is before or after it in the virtualizing direction.
RelativePosition LayoutStrategyBase::GetReferenceDirectionFromWindow(
    _In_ wf::Rect referenceRect,
    _In_ wf::Rect window) const
{
    const float firstReferenceEdge = referenceRect.*PointFromRectInVirtualizingDirection();
    const float lastReferenceEdge = firstReferenceEdge + referenceRect.*SizeFromRectInVirtualizingDirection();
    const float firstWindowEdge = window.*PointFromRectInVirtualizingDirection();
    const float lastWindowEdge = firstWindowEdge + window.*SizeFromRectInVirtualizingDirection();

    RelativePosition result;

    if (lastReferenceEdge < firstWindowEdge)
    {
        result = RelativePosition::Before;
    }
    else if (lastWindowEdge < firstReferenceEdge)
    {
        result = RelativePosition::After;
    }
    else
    {
        result = RelativePosition::Inside;
    }

    return result;
}

} } }