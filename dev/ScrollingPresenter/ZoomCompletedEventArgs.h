// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Scroller.h"
#include "ZoomCompletedEventArgs.g.h"

class ZoomCompletedEventArgs :
    public winrt::implementation::ZoomCompletedEventArgsT<ZoomCompletedEventArgs>
{
public:
    ZoomCompletedEventArgs()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ~ZoomCompletedEventArgs()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    // IZoomCompletedEventArgs overrides
    winrt::ZoomInfo ZoomInfo();
    ScrollerViewChangeResult Result();

    void ZoomFactorChangeId(int32_t zoomFactorChangeId);
    void Result(ScrollerViewChangeResult result);

private:
    int32_t m_zoomFactorChangeId{ -1 };
    ScrollerViewChangeResult m_result{ ScrollerViewChangeResult::Completed };
};
