// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "Scroller.h"
#include "ScrollControllerOffsetChangeRequestedEventArgs.h"

CppWinRTActivatableClassWithBasicFactory(ScrollControllerOffsetChangeRequestedEventArgs);

ScrollControllerOffsetChangeRequestedEventArgs::ScrollControllerOffsetChangeRequestedEventArgs(
    double offset,
    const winrt::ScrollerViewKind& offsetKind,
    const winrt::ScrollerViewChangeKind& offsetChangeKind)
{
    SCROLLER_TRACE_VERBOSE(nullptr, L"%s[0x%p](offset: %lf, offsetKind: %s, offsetChangeKind: %s)\n",
        METH_NAME, this, offset, TypeLogging::ScrollerViewKindToString(offsetKind).c_str(), TypeLogging::ScrollerViewChangeKindToString(offsetChangeKind).c_str());

    m_offset = offset;
    m_offsetKind = offsetKind;
    m_offsetChangeKind = offsetChangeKind;
}

double ScrollControllerOffsetChangeRequestedEventArgs::Offset()
{
    return m_offset;
}

winrt::ScrollerViewKind ScrollControllerOffsetChangeRequestedEventArgs::OffsetKind()
{
    return m_offsetKind;
}

winrt::ScrollerViewChangeKind ScrollControllerOffsetChangeRequestedEventArgs::OffsetChangeKind()
{
    return m_offsetChangeKind;
}

int32_t ScrollControllerOffsetChangeRequestedEventArgs::ViewChangeId()
{
    return m_viewChangeId;
}

void ScrollControllerOffsetChangeRequestedEventArgs::ViewChangeId(int32_t viewChangeId)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_INT, METH_NAME, this, viewChangeId);
    m_viewChangeId = viewChangeId;
}
