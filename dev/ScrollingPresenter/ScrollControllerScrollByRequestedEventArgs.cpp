// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollingPresenterTypeLogging.h"
#include "ScrollControllerScrollByRequestedEventArgs.h"

CppWinRTActivatableClassWithBasicFactory(ScrollControllerScrollByRequestedEventArgs);

ScrollControllerScrollByRequestedEventArgs::ScrollControllerScrollByRequestedEventArgs(
    double offsetDelta,
    winrt::ScrollOptions const & options)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_DBL, METH_NAME, this,
        TypeLogging::ScrollOptionsToString(options).c_str(), offsetDelta);

    m_offsetDelta = offsetDelta;
    m_options = options;
}

double ScrollControllerScrollByRequestedEventArgs::OffsetDelta() const
{
    return m_offsetDelta;
}

winrt::ScrollOptions ScrollControllerScrollByRequestedEventArgs::Options() const
{
    return m_options;
}

winrt::ScrollingScrollInfo ScrollControllerScrollByRequestedEventArgs::ScrollInfo() const
{
    return m_scrollInfo;
}

void ScrollControllerScrollByRequestedEventArgs::ScrollInfo(winrt::ScrollingScrollInfo scrollInfo)
{
    SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, scrollInfo.OffsetsChangeId);

    m_scrollInfo = scrollInfo;
}
