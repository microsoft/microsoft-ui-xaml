// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollerChangeZoomFactorOptions.h"

CppWinRTActivatableClassWithBasicFactory(ScrollerChangeZoomFactorOptions);

ScrollerChangeZoomFactorOptions::ScrollerChangeZoomFactorOptions(
    float zoomFactor,
    const winrt::ScrollerViewKind& zoomFactorKind,
    const winrt::float2& centerPoint,
    const winrt::ScrollerViewChangeKind& viewChangeKind,
    const winrt::ScrollerViewChangeSnapPointRespect& snapPointRespect)
{
    SCROLLER_TRACE_VERBOSE(nullptr, L"%s[0x%p](zoomFactor: %f, zoomFactorKind: %s, centerPoint: (%f, %f), viewChangeKind: %s)\n",
        METH_NAME, this, zoomFactor, TypeLogging::ScrollerViewKindToString(zoomFactorKind).c_str(), centerPoint.x, centerPoint.y, TypeLogging::ScrollerViewChangeKindToString(viewChangeKind).c_str());

    m_zoomFactor = zoomFactor;
    m_zoomFactorKind = zoomFactorKind;
    m_centerPoint = centerPoint;
    m_viewChangeKind = viewChangeKind;
    m_snapPointRespect = snapPointRespect;
}

float ScrollerChangeZoomFactorOptions::ZoomFactor()
{
    return m_zoomFactor;
}

void ScrollerChangeZoomFactorOptions::ZoomFactor(float zoomFactor)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_FLT, METH_NAME, this, zoomFactor);
    m_zoomFactor = zoomFactor;
}

winrt::ScrollerViewKind ScrollerChangeZoomFactorOptions::ZoomFactorKind()
{
    return m_zoomFactorKind;
}

void ScrollerChangeZoomFactorOptions::ZoomFactorKind(winrt::ScrollerViewKind const& zoomFactorKind)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollerViewKindToString(zoomFactorKind).c_str());
    m_zoomFactorKind = zoomFactorKind;
}

winrt::float2 ScrollerChangeZoomFactorOptions::CenterPoint()
{
    return m_centerPoint;
}

void ScrollerChangeZoomFactorOptions::CenterPoint(winrt::float2 const& centerPoint)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, centerPoint.x, centerPoint.y);
    m_centerPoint = centerPoint;
}

winrt::ScrollerViewChangeKind ScrollerChangeZoomFactorOptions::ViewChangeKind()
{
    return m_viewChangeKind;
}

void ScrollerChangeZoomFactorOptions::ViewChangeKind(winrt::ScrollerViewChangeKind const& viewChangeKind)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollerViewChangeKindToString(viewChangeKind).c_str());
    m_viewChangeKind = viewChangeKind;
}

winrt::ScrollerViewChangeSnapPointRespect ScrollerChangeZoomFactorOptions::SnapPointRespect()
{
    return m_snapPointRespect;
}

void ScrollerChangeZoomFactorOptions::SnapPointRespect(const winrt::ScrollerViewChangeSnapPointRespect& snapPointRespect)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollerViewChangeSnapPointRespectToString(snapPointRespect).c_str());
    m_snapPointRespect = snapPointRespect;
}
