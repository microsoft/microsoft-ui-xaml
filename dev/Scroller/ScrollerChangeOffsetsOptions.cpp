// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollerChangeOffsetsOptions.h"

CppWinRTActivatableClassWithBasicFactory(ScrollerChangeOffsetsOptions);

ScrollerChangeOffsetsOptions::ScrollerChangeOffsetsOptions(
    double horizontalOffset,
    double verticalOffset,
    const winrt::ScrollerViewKind& offsetsKind,
    const winrt::ScrollerViewChangeKind& viewChangeKind,
    const winrt::ScrollerViewChangeSnapPointRespect& snapPointRespect)
{
    SCROLLER_TRACE_VERBOSE(nullptr, L"%s[0x%p](horizontalOffset: %lf, verticalOffset: %lf, offsetsKind: %s, viewChangeKind: %s)\n",
        METH_NAME, this, horizontalOffset, verticalOffset, TypeLogging::ScrollerViewKindToString(offsetsKind).c_str(), TypeLogging::ScrollerViewChangeKindToString(viewChangeKind).c_str());

    m_horizontalOffset = horizontalOffset;
    m_verticalOffset = verticalOffset;
    m_offsetsKind = offsetsKind;
    m_viewChangeKind = viewChangeKind;
    m_snapPointRespect = snapPointRespect;
}

double ScrollerChangeOffsetsOptions::HorizontalOffset()
{
    return m_horizontalOffset;
}

void ScrollerChangeOffsetsOptions::HorizontalOffset(double horizontalOffset)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, horizontalOffset);
    m_horizontalOffset = horizontalOffset;
}

double ScrollerChangeOffsetsOptions::VerticalOffset()
{
    return m_verticalOffset;
}

void ScrollerChangeOffsetsOptions::VerticalOffset(double verticalOffset)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, verticalOffset);
    m_verticalOffset = verticalOffset;
}

winrt::ScrollerViewKind ScrollerChangeOffsetsOptions::OffsetsKind()
{
    return m_offsetsKind;
}

void ScrollerChangeOffsetsOptions::OffsetsKind(winrt::ScrollerViewKind const& offsetsKind)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollerViewKindToString(offsetsKind).c_str());
    m_offsetsKind = offsetsKind;
}

winrt::ScrollerViewChangeKind ScrollerChangeOffsetsOptions::ViewChangeKind()
{
    return m_viewChangeKind;
}

void ScrollerChangeOffsetsOptions::ViewChangeKind(winrt::ScrollerViewChangeKind const& viewChangeKind)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollerViewChangeKindToString(viewChangeKind).c_str());
    m_viewChangeKind = viewChangeKind;
}

winrt::ScrollerViewChangeSnapPointRespect ScrollerChangeOffsetsOptions::SnapPointRespect()
{
    return m_snapPointRespect;
}

void ScrollerChangeOffsetsOptions::SnapPointRespect(winrt::ScrollerViewChangeSnapPointRespect const& snapPointRespect)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::ScrollerViewChangeSnapPointRespectToString(snapPointRespect).c_str());
    m_snapPointRespect = snapPointRespect;
}
