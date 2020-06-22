// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollerTrace.h"
#include "ScrollAnimationStartingEventArgs.g.h"

class ScrollAnimationStartingEventArgs :
    public winrt::implementation::ScrollAnimationStartingEventArgsT<ScrollAnimationStartingEventArgs>
{
public:
    ScrollAnimationStartingEventArgs()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ~ScrollAnimationStartingEventArgs()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    // IScrollAnimationStartingEventArgs overrides
    winrt::CompositionAnimation Animation();
    void Animation(winrt::CompositionAnimation const& value);
    winrt::ScrollInfo ScrollInfo();
    winrt::float2 StartPosition();
    winrt::float2 EndPosition();

    void SetOffsetsChangeId(int32_t offsetsChangeId);
    winrt::CompositionAnimation GetAnimation() const;
    void SetAnimation(const winrt::CompositionAnimation& animation);
    void SetStartPosition(const winrt::float2& startPosition);
    void SetEndPosition(const winrt::float2& endPosition);

private:
    winrt::CompositionAnimation m_animation{ nullptr };
    int32_t m_offsetsChangeId{ -1 };
    winrt::float2 m_startPosition{ 0.0f, 0.0f };
    winrt::float2 m_endPosition{ 0.0f, 0.0f };
};
