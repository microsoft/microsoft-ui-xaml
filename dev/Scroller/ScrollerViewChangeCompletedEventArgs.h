// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollerViewChangeCompletedEventArgs.g.h"

class ScrollerViewChangeCompletedEventArgs :
    public winrt::implementation::ScrollerViewChangeCompletedEventArgsT<ScrollerViewChangeCompletedEventArgs>
{
public:
    ScrollerViewChangeCompletedEventArgs()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ~ScrollerViewChangeCompletedEventArgs()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    // IScrollerViewChangeCompletedEventArgs overrides
    int32_t ViewChangeId();
    winrt::ScrollerViewChangeResult Result();

    void ViewChangeId(int viewChangeId);
    void Result(const winrt::ScrollerViewChangeResult& result);

private:
    int32_t m_viewChangeId{ -1 };
    winrt::ScrollerViewChangeResult m_result{ winrt::ScrollerViewChangeResult::Completed };
};
