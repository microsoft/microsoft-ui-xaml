// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "ScrollerTrace.h"
#include "ScrollerChangingZoomFactorEventArgs.h"

winrt::float2 ScrollerChangingZoomFactorEventArgs::CenterPoint()
{
    return m_centerPoint;
}

float ScrollerChangingZoomFactorEventArgs::StartZoomFactor()
{
    return m_startZoomFactor;
}

float ScrollerChangingZoomFactorEventArgs::EndZoomFactor()
{
    return m_endZoomFactor;
}

winrt::CompositionAnimation ScrollerChangingZoomFactorEventArgs::Animation()
{
    return m_animation;
}

void ScrollerChangingZoomFactorEventArgs::Animation(winrt::CompositionAnimation const& value)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, value);
    if (!value)
    {
        throw winrt::hresult_invalid_argument(L"Animation cannot be set to null.");
    }
    m_animation = value;
}

int32_t ScrollerChangingZoomFactorEventArgs::ViewChangeId()
{
    return m_viewChangeId;
}

void ScrollerChangingZoomFactorEventArgs::SetViewChangeId(int32_t viewChangeId)
{
    m_viewChangeId = viewChangeId;
}

winrt::CompositionAnimation ScrollerChangingZoomFactorEventArgs::GetAnimation() const
{
    return m_animation;
}

void ScrollerChangingZoomFactorEventArgs::SetAnimation(const winrt::CompositionAnimation& animation)
{
    m_animation = animation;
}

void ScrollerChangingZoomFactorEventArgs::SetCenterPoint(const winrt::float2& centerPoint)
{
    m_centerPoint = centerPoint;
}

void ScrollerChangingZoomFactorEventArgs::SetStartZoomFactor(const float& startZoomFactor)
{
    m_startZoomFactor = startZoomFactor;
}

void ScrollerChangingZoomFactorEventArgs::SetEndZoomFactor(const float& endZoomFactor)
{
    m_endZoomFactor = endZoomFactor;
}
