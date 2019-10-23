// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollingPresenter.h"
#include "ScrollingZoomCompletedEventArgs.g.h"

class ScrollingZoomCompletedEventArgs :
    public winrt::implementation::ScrollingZoomCompletedEventArgsT<ScrollingZoomCompletedEventArgs>
{
public:
    ScrollingZoomCompletedEventArgs()
    {
        SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ~ScrollingZoomCompletedEventArgs()
    {
        SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    // IZoomCompletedEventArgs overrides
    winrt::ScrollingZoomInfo ZoomInfo();
    ScrollingPresenterViewChangeResult Result();

    void ZoomFactorChangeId(int32_t zoomFactorChangeId);
    void Result(ScrollingPresenterViewChangeResult result);

private:
    int32_t m_zoomFactorChangeId{ -1 };
    ScrollingPresenterViewChangeResult m_result{ ScrollingPresenterViewChangeResult::Completed };
};
