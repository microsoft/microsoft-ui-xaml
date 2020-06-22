// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollerTypeLogging.h"
#include "ScrollControllerScrollByRequestedEventArgs.h"

#include "ScrollControllerScrollByRequestedEventArgs.properties.cpp"

ScrollControllerScrollByRequestedEventArgs::ScrollControllerScrollByRequestedEventArgs(
    double offsetDelta,
    winrt::ScrollOptions const & options)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR_DBL, METH_NAME, this,
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

winrt::ScrollInfo ScrollControllerScrollByRequestedEventArgs::Info() const
{
    return m_info;
}

void ScrollControllerScrollByRequestedEventArgs::Info(winrt::ScrollInfo info)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, info.OffsetsChangeId);

    m_info = info;
}
