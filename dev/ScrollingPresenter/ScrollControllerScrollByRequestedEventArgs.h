// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollControllerScrollByRequestedEventArgs.g.h"

class ScrollControllerScrollByRequestedEventArgs :
    public winrt::implementation::ScrollControllerScrollByRequestedEventArgsT<ScrollControllerScrollByRequestedEventArgs>
{
public:
    ~ScrollControllerScrollByRequestedEventArgs()
    {
        SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollControllerScrollByRequestedEventArgs(
        double offsetDelta,
        winrt::ScrollOptions const & options);

    double OffsetDelta() const;
    winrt::ScrollOptions Options() const;
    winrt::ScrollingScrollInfo ScrollInfo() const;
    void ScrollInfo(winrt::ScrollingScrollInfo scrollInfo);

private:
    double m_offsetDelta{ 0.0 };
    winrt::ScrollOptions m_options{ nullptr };
    winrt::ScrollingScrollInfo m_scrollInfo{ -1 };
};
