// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollPresenterTrace.h"
#include "ScrollingScrollAnimationStartingEventArgs.g.h"

class ScrollingScrollAnimationStartingEventArgs :
    public winrt::implementation::ScrollingScrollAnimationStartingEventArgsT<ScrollingScrollAnimationStartingEventArgs>
{
public:
    ScrollingScrollAnimationStartingEventArgs()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ~ScrollingScrollAnimationStartingEventArgs()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    // IScrollingScrollAnimationStartingEventArgs overrides
    winrt::CompositionAnimation Animation();
    void Animation(winrt::CompositionAnimation const& value);
    int32_t CorrelationId();
    winrt::float2 StartPosition();
    winrt::float2 EndPosition();

    void SetOffsetsChangeCorrelationId(int32_t offsetsChangeCorrelationId);
    winrt::CompositionAnimation GetAnimation() const;
    void SetAnimation(const winrt::CompositionAnimation& animation);
    void SetStartPosition(const winrt::float2& startPosition);
    void SetEndPosition(const winrt::float2& endPosition);

private:
    winrt::CompositionAnimation m_animation{ nullptr };
    int32_t m_offsetsChangeCorrelationId{ -1 };
    winrt::float2 m_startPosition{ 0.0f, 0.0f };
    winrt::float2 m_endPosition{ 0.0f, 0.0f };
};
