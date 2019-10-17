// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "ScrollerTrace.h"
#include "ScrollAnimationStartingEventArgs.h"

winrt::CompositionAnimation ScrollAnimationStartingEventArgs::Animation()
{
    return m_animation;
}

void ScrollAnimationStartingEventArgs::Animation(winrt::CompositionAnimation const& value)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, value);
    if (!value)
    {
        throw winrt::hresult_invalid_argument(L"Animation cannot be set to null.");
    }
    m_animation = value;
}

winrt::ScrollInfo ScrollAnimationStartingEventArgs::ScrollInfo()
{
    return winrt::ScrollInfo{ m_offsetsChangeId };
}

winrt::float2 ScrollAnimationStartingEventArgs::StartPosition()
{
    return m_startPosition;
}

winrt::float2 ScrollAnimationStartingEventArgs::EndPosition()
{
    return m_endPosition;
}

void ScrollAnimationStartingEventArgs::SetOffsetsChangeId(int32_t offsetsChangeId)
{
    m_offsetsChangeId = offsetsChangeId;
}

winrt::CompositionAnimation ScrollAnimationStartingEventArgs::GetAnimation() const
{
    return m_animation;
}

void ScrollAnimationStartingEventArgs::SetAnimation(const winrt::CompositionAnimation& animation)
{
    m_animation = animation;
}


void ScrollAnimationStartingEventArgs::SetStartPosition(const winrt::float2& startPosition)
{
    m_startPosition = startPosition;
}


void ScrollAnimationStartingEventArgs::SetEndPosition(const winrt::float2& endPosition)
{
    m_endPosition = endPosition;
}
