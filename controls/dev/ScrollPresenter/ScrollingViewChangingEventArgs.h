// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollPresenter.h"
#include "ScrollingViewChangingEventArgs.g.h"

class ScrollingViewChangingEventArgs :
    public winrt::implementation::ScrollingViewChangingEventArgsT<ScrollingViewChangingEventArgs>
{
public:
    ScrollingViewChangingEventArgs()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ~ScrollingViewChangingEventArgs()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    // IScrollingViewChangingEventArgs overrides
    double HorizontalOffset() const;
    double VerticalOffset() const;
    float ZoomFactor() const;

    void SetHorizontalOffset(double horizontalOffset);
    void SetVerticalOffset(double verticalOffset);
    void SetZoomFactor(float zoomFactor);

#ifdef DBG
    int32_t CorrelationIdDbg() const;
    void SetCorrelationIdDbg(int32_t correlationIdDbg);
#endif // DBG

private:
    double m_horizontalOffset{};
    double m_verticalOffset{};
    float m_zoomFactor{};

#ifdef DBG
    int32_t m_correlationIdDbg{ -1 };
#endif // DBG
};
