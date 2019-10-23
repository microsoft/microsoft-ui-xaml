// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollingPresenter.h"
#include "ScrollingScrollCompletedEventArgs.g.h"

class ScrollingScrollCompletedEventArgs :
    public winrt::implementation::ScrollingScrollCompletedEventArgsT<ScrollingScrollCompletedEventArgs>
{
public:
    ScrollingScrollCompletedEventArgs()
    {
        SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ~ScrollingScrollCompletedEventArgs()
    {
        SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    // IScrollingScrollCompletedEventArgs overrides
    winrt::ScrollingScrollInfo ScrollInfo();
    ScrollingPresenterViewChangeResult Result();

    void OffsetsChangeId(int32_t offsetsChangeId);
    void Result(ScrollingPresenterViewChangeResult result);

private:
    int32_t m_offsetsChangeId{ -1 };
    ScrollingPresenterViewChangeResult m_result{ ScrollingPresenterViewChangeResult::Completed };
};
