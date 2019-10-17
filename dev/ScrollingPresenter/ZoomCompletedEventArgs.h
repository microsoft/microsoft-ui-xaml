// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollingPresenter.h"
#include "ZoomCompletedEventArgs.g.h"

class ZoomCompletedEventArgs :
    public winrt::implementation::ZoomCompletedEventArgsT<ZoomCompletedEventArgs>
{
public:
    ZoomCompletedEventArgs()
    {
        SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ~ZoomCompletedEventArgs()
    {
        SCROLLINGPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    // IZoomCompletedEventArgs overrides
    winrt::ZoomInfo ZoomInfo();
    ScrollingPresenterViewChangeResult Result();

    void ZoomFactorChangeId(int32_t zoomFactorChangeId);
    void Result(ScrollingPresenterViewChangeResult result);

private:
    int32_t m_zoomFactorChangeId{ -1 };
    ScrollingPresenterViewChangeResult m_result{ ScrollingPresenterViewChangeResult::Completed };
};
