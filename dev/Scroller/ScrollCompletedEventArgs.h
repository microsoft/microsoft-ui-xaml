// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Scroller.h"
#include "ScrollCompletedEventArgs.g.h"

class ScrollCompletedEventArgs :
    public winrt::implementation::ScrollCompletedEventArgsT<ScrollCompletedEventArgs>
{
public:
    ScrollCompletedEventArgs()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ~ScrollCompletedEventArgs()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    // IScrollCompletedEventArgs overrides
    winrt::ScrollInfo ScrollInfo();
    ScrollerViewChangeResult Result();

    void OffsetsChangeId(int32_t offsetsChangeId);
    void Result(ScrollerViewChangeResult result);

private:
    int32_t m_offsetsChangeId{ -1 };
    ScrollerViewChangeResult m_result{ ScrollerViewChangeResult::Completed };
};
