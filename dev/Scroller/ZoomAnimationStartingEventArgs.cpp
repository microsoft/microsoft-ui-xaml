// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "ScrollerTrace.h"
#include "ZoomAnimationStartingEventArgs.h"

winrt::float2 ZoomAnimationStartingEventArgs::CenterPoint()
{
    return m_centerPoint;
}

float ZoomAnimationStartingEventArgs::StartZoomFactor()
{
    return m_startZoomFactor;
}

float ZoomAnimationStartingEventArgs::EndZoomFactor()
{
    return m_endZoomFactor;
}

winrt::CompositionAnimation ZoomAnimationStartingEventArgs::Animation()
{
    return m_animation;
}

void ZoomAnimationStartingEventArgs::Animation(winrt::CompositionAnimation const& value)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, value);
    if (!value)
    {
        throw winrt::hresult_invalid_argument(L"Animation cannot be set to null.");
    }
    m_animation = value;
}

winrt::ZoomInfo ZoomAnimationStartingEventArgs::ZoomInfo()
{
    return winrt::ZoomInfo{ m_zoomFactorChangeId };
}

void ZoomAnimationStartingEventArgs::SetZoomFactorChangeId(int32_t zoomFactorChangeId)
{
    m_zoomFactorChangeId = zoomFactorChangeId;
}

winrt::CompositionAnimation ZoomAnimationStartingEventArgs::GetAnimation() const
{
    return m_animation;
}

void ZoomAnimationStartingEventArgs::SetAnimation(const winrt::CompositionAnimation& animation)
{
    m_animation = animation;
}

void ZoomAnimationStartingEventArgs::SetCenterPoint(const winrt::float2& centerPoint)
{
    m_centerPoint = centerPoint;
}

void ZoomAnimationStartingEventArgs::SetStartZoomFactor(const float& startZoomFactor)
{
    m_startZoomFactor = startZoomFactor;
}

void ZoomAnimationStartingEventArgs::SetEndZoomFactor(const float& endZoomFactor)
{
    m_endZoomFactor = endZoomFactor;
}
