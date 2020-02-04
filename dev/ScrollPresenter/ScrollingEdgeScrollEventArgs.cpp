// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
//#include "ScrollingPresenter.h"
#include "ScrollingPresenterTrace.h"
#include "ScrollingEdgeScrollEventArgs.h"

ScrollingEdgeScrollEventArgs::ScrollingEdgeScrollEventArgs(
    //const winrt::ScrollingPresenter& scrollingPresenter,
    const winrt::ScrollingScrollInfo& scrollInfo,
    const winrt::float2& offsetsVelocity,
    UINT pointerId) :
    m_scrollInfo(scrollInfo),
    m_offsetsVelocity(offsetsVelocity),
    m_pointerId(pointerId)
{
    //SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, scrollingPresenter);

    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT_INT, METH_NAME, this, scrollInfo.OffsetsChangeId, pointerId);
    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::Float2ToString(offsetsVelocity).c_str());

    //m_scrollingPresenter.set(scrollingPresenter);
}

#pragma region IScrollingEdgeScrollEventArgs

winrt::ScrollingScrollInfo ScrollingEdgeScrollEventArgs::ScrollInfo() const
{
    return m_scrollInfo;
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
