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
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollControllerScrollToRequestedEventArgs(
        double offset,
        winrt::ScrollingScrollOptions const & options);

    double Offset() const;
    winrt::ScrollingScrollOptions Options() const;
    int32_t CorrelationId() const;
    void CorrelationId(int32_t correlationId);

private:
    double m_offset{ 0.0 };
    winrt::ScrollingScrollOptions m_options{ nullptr };
    int32_t m_correlationId{ -1 };
};
