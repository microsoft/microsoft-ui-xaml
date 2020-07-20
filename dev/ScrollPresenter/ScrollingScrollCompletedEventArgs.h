// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollPresenter.h"
#include "ScrollingScrollCompletedEventArgs.g.h"

class ScrollingScrollCompletedEventArgs :
    public winrt::implementation::ScrollingScrollCompletedEventArgsT<ScrollingScrollCompletedEventArgs>
{
public:
    ScrollingScrollCompletedEventArgs()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ~ScrollingScrollCompletedEventArgs()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    // IScrollingScrollCompletedEventArgs overrides
    int32_t CorrelationId();
    ScrollPresenterViewChangeResult Result();

    void OffsetsChangeCorrelationId(int32_t offsetsChangeCorrelationId);
    void Result(ScrollPresenterViewChangeResult result);

private:
    int32_t m_offsetsChangeCorrelationId{ -1 };
    ScrollPresenterViewChangeResult m_result{ ScrollPresenterViewChangeResult::Completed };
};
