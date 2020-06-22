// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollControllerScrollToRequestedEventArgs.g.h"

class ScrollControllerScrollToRequestedEventArgs :
    public winrt::implementation::ScrollControllerScrollToRequestedEventArgsT<ScrollControllerScrollToRequestedEventArgs>
{
public:
    ~ScrollControllerScrollToRequestedEventArgs()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollControllerScrollToRequestedEventArgs(
        double offset,
        winrt::ScrollOptions const & options);

    double Offset() const;
    winrt::ScrollOptions Options() const;
    winrt::ScrollInfo Info() const;
    void Info(winrt::ScrollInfo info);

private:
    double m_offset{ 0.0 };
    winrt::ScrollOptions m_options{ nullptr };
    winrt::ScrollInfo m_info{ -1 };
};
