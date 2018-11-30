// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Scroller.h"

#include "ScrollerChangeZoomFactorOptions.g.h"

class ScrollerChangeZoomFactorOptions :
    public winrt::implementation::ScrollerChangeZoomFactorOptionsT<ScrollerChangeZoomFactorOptions>
{
public:
    ~ScrollerChangeZoomFactorOptions()
    {
        SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }

    ScrollerChangeZoomFactorOptions(
        float zoomFactor,
        const winrt::ScrollerViewKind& zoomFactorKind,
        const winrt::float2& centerPoint,
        const winrt::ScrollerViewChangeKind& viewChangeKind,
        const winrt::ScrollerViewChangeSnapPointRespect& snapPointRespect);

    float ZoomFactor();
    void ZoomFactor(float zoomFactor);

    winrt::ScrollerViewKind ZoomFactorKind();
    void ZoomFactorKind(winrt::ScrollerViewKind const& zoomFactorKind);

    winrt::float2 CenterPoint();
    void CenterPoint(winrt::float2 const& centerPoint);

    winrt::ScrollerViewChangeKind ViewChangeKind();
    void ViewChangeKind(winrt::ScrollerViewChangeKind const& viewChangeKind);

    winrt::ScrollerViewChangeSnapPointRespect SnapPointRespect();
    void SnapPointRespect(const winrt::ScrollerViewChangeSnapPointRespect& snapPointRespect);

private:
    float m_zoomFactor{};
    winrt::float2 m_centerPoint{};
    winrt::ScrollerViewKind m_zoomFactorKind{ winrt::ScrollerViewKind::Absolute };
    winrt::ScrollerViewChangeKind m_viewChangeKind{ winrt::ScrollerViewChangeKind::AllowAnimation };
    winrt::ScrollerViewChangeSnapPointRespect m_snapPointRespect{ winrt::ScrollerViewChangeSnapPointRespect::IgnoreSnapPoints };
};
