// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollingEdgeScrollEventArgs.g.h"

class ScrollingEdgeScrollEventArgs :
    public ReferenceTracker<ScrollingEdgeScrollEventArgs, winrt::implementation::ScrollingEdgeScrollEventArgsT, winrt::composable, winrt::composing>
{
public:
    ~ScrollingEdgeScrollEventArgs()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollingEdgeScrollEventArgs(
        int correlationId,
        const winrt::float2& offsetsVelocity,
        UINT pointerId);

#pragma region IScrollingEdgeScrollEventArgs
    int CorrelationId() const;
    winrt::float2 OffsetsVelocity() const;
    UINT PointerId() const;
#pragma endregion

private:
    int m_correlationId{ -1 };
    winrt::float2 m_offsetsVelocity{};
    UINT m_pointerId{ 0 };
};
