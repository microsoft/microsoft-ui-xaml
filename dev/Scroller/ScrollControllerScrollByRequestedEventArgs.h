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
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollControllerScrollByRequestedEventArgs(
        double offsetDelta,
        winrt::ScrollOptions const & options);

    [[nodiscard]] double OffsetDelta() const;
    [[nodiscard]] winrt::ScrollOptions Options() const;
    [[nodiscard]] winrt::ScrollInfo Info() const;
    void Info(winrt::ScrollInfo info);

private:
    double m_offsetDelta{ 0.0 };
    winrt::ScrollOptions m_options{ nullptr };
    winrt::ScrollInfo m_info{ -1 };
};
