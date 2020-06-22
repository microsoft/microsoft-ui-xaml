// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollingBringingIntoViewEventArgs.g.h"

class ScrollingBringingIntoViewEventArgs :
    public winrt::implementation::ScrollingBringingIntoViewEventArgsT<ScrollingBringingIntoViewEventArgs>
{
public:
    ScrollingBringingIntoViewEventArgs()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ~ScrollingBringingIntoViewEventArgs()
    {
        SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    // IScrollingBringingIntoViewEventArgs overrides
    winrt::SnapPointsMode SnapPointsMode();
    void SnapPointsMode(winrt::SnapPointsMode snapPointsMode);
    winrt::BringIntoViewRequestedEventArgs RequestEventArgs();
    double TargetHorizontalOffset();
    double TargetVerticalOffset();
    winrt::ScrollInfo ScrollInfo();
    bool Cancel();
    void Cancel(bool value); 

    double GetTargetHorizontalOffset() const
    {
        return m_targetHorizontalOffset;
    }

    double GetTargetVerticalOffset() const
    {
        return m_targetVerticalOffset;
    }

    bool GetCancel() const
    {
        return m_cancel;
    }

    void OffsetsChangeId(int32_t offsetsChangeId);
    void RequestEventArgs(const winrt::BringIntoViewRequestedEventArgs& requestEventArgs);
    void TargetOffsets(double targetHorizontalOffset, double targetVerticalOffset);

private:
    winrt::SnapPointsMode m_snapPointsMode{ winrt::SnapPointsMode::Ignore };
    winrt::BringIntoViewRequestedEventArgs m_requestEventArgs{ nullptr };
    double m_targetHorizontalOffset{ 0.0 };
    double m_targetVerticalOffset{ 0.0 };
    bool m_cancel{ false };
    int32_t m_offsetsChangeId{ -1 };
};

