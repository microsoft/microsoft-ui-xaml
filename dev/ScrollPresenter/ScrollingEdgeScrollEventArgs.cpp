// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTrace.h"
#include "ScrollingEdgeScrollEventArgs.h"

ScrollingEdgeScrollEventArgs::ScrollingEdgeScrollEventArgs(
    int correlationId,
    const winrt::float2& offsetsVelocity,
    UINT pointerId) :
    m_correlationId(correlationId),
    m_offsetsVelocity(offsetsVelocity),
    m_pointerId(pointerId)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT_INT, METH_NAME, this, correlationId, pointerId);
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::Float2ToString(offsetsVelocity).c_str());
}

#pragma region IScrollingEdgeScrollEventArgs

int ScrollingEdgeScrollEventArgs::CorrelationId() const
{
    return m_correlationId;
}

winrt::float2 ScrollingEdgeScrollEventArgs::OffsetsVelocity() const
{
    return m_offsetsVelocity;
}

UINT ScrollingEdgeScrollEventArgs::PointerId() const
{
    return m_pointerId;
}

#pragma endregion
