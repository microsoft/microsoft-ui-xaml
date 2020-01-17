// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "ScrollPresenterTrace.h"
#include "ScrollingZoomAnimationStartingEventArgs.h"

winrt::float2 ScrollingZoomAnimationStartingEventArgs::CenterPoint()
{
    return m_centerPoint;
}

float ScrollingZoomAnimationStartingEventArgs::StartZoomFactor()
{
    return m_startZoomFactor;
}

float ScrollingZoomAnimationStartingEventArgs::EndZoomFactor()
{
    return m_endZoomFactor;
}

winrt::CompositionAnimation ScrollingZoomAnimationStartingEventArgs::Animation()
{
    return m_animation;
}

void ScrollingZoomAnimationStartingEventArgs::Animation(winrt::CompositionAnimation const& value)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, value);
    if (!value)
    {
        throw winrt::hresult_invalid_argument(L"Animation cannot be set to null.");
    }
    m_animation = value;
}

winrt::ZoomInfo ScrollingZoomAnimationStartingEventArgs::ZoomInfo()
{
    return winrt::ZoomInfo{ m_zoomFactorChangeId };
}

void ScrollingZoomAnimationStartingEventArgs::SetZoomFactorChangeId(int32_t zoomFactorChangeId)
{
    m_zoomFactorChangeId = zoomFactorChangeId;
}

winrt::CompositionAnimation ScrollingZoomAnimationStartingEventArgs::GetAnimation() const
{
    return m_animation;
}

void ScrollingZoomAnimationStartingEventArgs::SetAnimation(const winrt::CompositionAnimation& animation)
{
    m_animation = animation;
}

void ScrollingZoomAnimationStartingEventArgs::SetCenterPoint(const winrt::float2& centerPoint)
{
    m_centerPoint = centerPoint;
}

void ScrollingZoomAnimationStartingEventArgs::SetStartZoomFactor(const float& startZoomFactor)
{
    m_startZoomFactor = startZoomFactor;
}

void ScrollingZoomAnimationStartingEventArgs::SetEndZoomFactor(const float& endZoomFactor)
{
    m_endZoomFactor = endZoomFactor;
}
