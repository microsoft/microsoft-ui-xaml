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
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollControllerScrollByRequestedEventArgs(
        double offsetDelta,
        winrt::ScrollingScrollOptions const & options);

    double OffsetDelta() const;
    winrt::ScrollingScrollOptions Options() const;
    winrt::ScrollInfo Info() const;
    void Info(winrt::ScrollInfo info);

private:
    double m_offsetDelta{ 0.0 };
    winrt::ScrollingScrollOptions m_options{ nullptr };
    winrt::ScrollInfo m_info{ -1 };
};
