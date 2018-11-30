// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "ScrollerTrace.h"
#include "ScrollerChangingOffsetsEventArgs.h"

winrt::CompositionAnimation ScrollerChangingOffsetsEventArgs::Animation()
{
    return m_animation;
}

void ScrollerChangingOffsetsEventArgs::Animation(winrt::CompositionAnimation const& value)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, value);
    if (!value)
    {
        throw winrt::hresult_invalid_argument(L"Animation cannot be set to null.");
    }
    m_animation = value;
}

int32_t ScrollerChangingOffsetsEventArgs::ViewChangeId()
{
    return m_viewChangeId;
}

winrt::float2 ScrollerChangingOffsetsEventArgs::StartPosition()
{
    return m_startPosition;
}

winrt::float2 ScrollerChangingOffsetsEventArgs::EndPosition()
{
    return m_endPosition;
}

void ScrollerChangingOffsetsEventArgs::SetViewChangeId(int32_t viewChangeId)
{
    m_viewChangeId = viewChangeId;
}

winrt::CompositionAnimation ScrollerChangingOffsetsEventArgs::GetAnimation() const
{
    return m_animation;
}

void ScrollerChangingOffsetsEventArgs::SetAnimation(const winrt::CompositionAnimation& animation)
{
    m_animation = animation;
}


void ScrollerChangingOffsetsEventArgs::SetStartPosition(const winrt::float2& startPosition)
{
    m_startPosition = startPosition;
}


void ScrollerChangingOffsetsEventArgs::SetEndPosition(const winrt::float2& endPosition)
{
    m_endPosition = endPosition;
}
