// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "ScrollPresenterTrace.h"
#include "ScrollingScrollAnimationStartingEventArgs.h"

winrt::CompositionAnimation ScrollingScrollAnimationStartingEventArgs::Animation()
{
    return m_animation;
}

void ScrollingScrollAnimationStartingEventArgs::Animation(winrt::CompositionAnimation const& value)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, value);
    if (!value)
    {
        throw winrt::hresult_invalid_argument(L"Animation cannot be set to null.");
    }
    m_animation = value;
}

winrt::ScrollInfo ScrollingScrollAnimationStartingEventArgs::ScrollInfo()
{
    return winrt::ScrollInfo{ m_offsetsChangeId };
}

winrt::float2 ScrollingScrollAnimationStartingEventArgs::StartPosition()
{
    return m_startPosition;
}

winrt::float2 ScrollingScrollAnimationStartingEventArgs::EndPosition()
{
    return m_endPosition;
}

void ScrollingScrollAnimationStartingEventArgs::SetOffsetsChangeId(int32_t offsetsChangeId)
{
    m_offsetsChangeId = offsetsChangeId;
}

winrt::CompositionAnimation ScrollingScrollAnimationStartingEventArgs::GetAnimation() const
{
    return m_animation;
}

void ScrollingScrollAnimationStartingEventArgs::SetAnimation(const winrt::CompositionAnimation& animation)
{
    m_animation = animation;
}


void ScrollingScrollAnimationStartingEventArgs::SetStartPosition(const winrt::float2& startPosition)
{
    m_startPosition = startPosition;
}


void ScrollingScrollAnimationStartingEventArgs::SetEndPosition(const winrt::float2& endPosition)
{
    m_endPosition = endPosition;
}
