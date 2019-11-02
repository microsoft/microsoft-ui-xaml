// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "ScrollingPresenter.h"
#include "ScrollingPresenterTrace.h"
#include "ScrollingEdgeScrollEventArgs.h"

ScrollingEdgeScrollEventArgs::ScrollingEdgeScrollEventArgs(const winrt::ScrollingPresenter& scrollingPresenter)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, scrollingPresenter);

    m_scrollingPresenter.set(scrollingPresenter);
}

#pragma region IScrollingEdgeScrollEventArgs

winrt::ScrollingScrollInfo ScrollingEdgeScrollEventArgs::ScrollInfo() const
{
    return winrt::ScrollingScrollInfo{ -1 };
}

winrt::float2 ScrollingEdgeScrollEventArgs::OffsetsVelocity() const
{
    return winrt::float2{};
}

UINT ScrollingEdgeScrollEventArgs::PointerId() const
{
    return 0;
}

#pragma endregion
