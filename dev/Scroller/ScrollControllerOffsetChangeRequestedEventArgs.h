// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollControllerOffsetChangeRequestedEventArgs.g.h"

class ScrollControllerOffsetChangeRequestedEventArgs :
    public winrt::implementation::ScrollControllerOffsetChangeRequestedEventArgsT<ScrollControllerOffsetChangeRequestedEventArgs>
{
public:
    ~ScrollControllerOffsetChangeRequestedEventArgs()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollControllerOffsetChangeRequestedEventArgs(
        double offset,
        const winrt::ScrollerViewKind& offsetsKind,
        const winrt::ScrollerViewChangeKind& offsetChangeKind);

    double Offset();
    winrt::ScrollerViewKind OffsetKind();
    winrt::ScrollerViewChangeKind OffsetChangeKind();
    int32_t ViewChangeId();
    void ViewChangeId(int32_t viewChangeId);

private:
    double m_offset{ 0.0 };
    winrt::ScrollerViewKind m_offsetKind{ winrt::ScrollerViewKind::Absolute };
    winrt::ScrollerViewChangeKind m_offsetChangeKind{ winrt::ScrollerViewChangeKind::AllowAnimation };
    int32_t m_viewChangeId{ -1 };
};
