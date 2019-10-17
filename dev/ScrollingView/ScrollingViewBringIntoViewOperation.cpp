// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ScrollingViewBringIntoViewOperation.h"
#include "ScrollingViewTrace.h"

ScrollingViewBringIntoViewOperation::ScrollingViewBringIntoViewOperation(winrt::UIElement const& targetElement)
{
    SCROLLINGVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR, METH_NAME, this, targetElement);

    m_targetElement = winrt::make_weak(targetElement);
}

ScrollingViewBringIntoViewOperation::~ScrollingViewBringIntoViewOperation()
{
    SCROLLINGVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR_INT, METH_NAME, this, m_targetElement.get(), m_ticksCount);
}

bool ScrollingViewBringIntoViewOperation::HasMaxTicksCount() const
{
    MUX_ASSERT(m_ticksCount <= s_maxTicksCount);

    return m_ticksCount == s_maxTicksCount;
}

winrt::UIElement ScrollingViewBringIntoViewOperation::TargetElement() const
{
    return m_targetElement.get();
}

int8_t ScrollingViewBringIntoViewOperation::TicksCount() const
{
    return m_ticksCount;
}

int8_t ScrollingViewBringIntoViewOperation::TickOperation()
{
    SCROLLINGVIEW_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR_INT, METH_NAME, this, m_targetElement.get(), m_ticksCount);

    MUX_ASSERT(m_ticksCount < s_maxTicksCount);

    return ++m_ticksCount;
}
