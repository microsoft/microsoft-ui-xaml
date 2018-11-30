// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Scroller.h"

#include "ScrollerChangeOffsetsOptions.g.h"

class ScrollerChangeOffsetsOptions :
    public winrt::implementation::ScrollerChangeOffsetsOptionsT<ScrollerChangeOffsetsOptions>
{
public:
    ~ScrollerChangeOffsetsOptions()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollerChangeOffsetsOptions(
        double horizontalOffset,
        double verticalOffset,
        const winrt::ScrollerViewKind& offsetsKind,
        const winrt::ScrollerViewChangeKind& viewChangeKind,
        const winrt::ScrollerViewChangeSnapPointRespect& snapPointRespect);

    double HorizontalOffset();
    void HorizontalOffset(double horizontalOffset);

    double VerticalOffset();
    void VerticalOffset(double verticalOffset);

    winrt::ScrollerViewKind OffsetsKind();
    void OffsetsKind(winrt::ScrollerViewKind const& offsetsKind);

    winrt::ScrollerViewChangeKind ViewChangeKind();
    void ViewChangeKind(winrt::ScrollerViewChangeKind const& viewChangeKind);

    winrt::ScrollerViewChangeSnapPointRespect SnapPointRespect();
    void SnapPointRespect(winrt::ScrollerViewChangeSnapPointRespect const& snapPointRespect);

private:
    double m_horizontalOffset{ 0.0 };
    double m_verticalOffset{ 0.0 };
    winrt::ScrollerViewKind m_offsetsKind{ winrt::ScrollerViewKind::Absolute };
    winrt::ScrollerViewChangeKind m_viewChangeKind{ winrt::ScrollerViewChangeKind::AllowAnimation };
    winrt::ScrollerViewChangeSnapPointRespect m_snapPointRespect{ winrt::ScrollerViewChangeSnapPointRespect::IgnoreSnapPoints };
};
