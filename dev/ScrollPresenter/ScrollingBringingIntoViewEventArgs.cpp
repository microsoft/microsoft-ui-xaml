// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollPresenterTrace.h"
#include "ScrollingBringingIntoViewEventArgs.h"

winrt::ScrollingSnapPointsMode ScrollingBringingIntoViewEventArgs::SnapPointsMode()
{
    return m_snapPointsMode;
}

void ScrollingBringingIntoViewEventArgs::SnapPointsMode(winrt::ScrollingSnapPointsMode snapPointsMode)
{
    m_snapPointsMode = snapPointsMode;
}

winrt::BringIntoViewRequestedEventArgs ScrollingBringingIntoViewEventArgs::RequestEventArgs()
{
    return m_requestEventArgs;
}

double ScrollingBringingIntoViewEventArgs::TargetHorizontalOffset()
{
    return m_targetHorizontalOffset;
}

double ScrollingBringingIntoViewEventArgs::TargetVerticalOffset()
{
    return m_targetVerticalOffset;
}

winrt::ScrollingScrollInfo ScrollingBringingIntoViewEventArgs::ScrollInfo()
{
    return winrt::ScrollingScrollInfo{ m_offsetsChangeId };
}

bool ScrollingBringingIntoViewEventArgs::Cancel()
{
    return m_cancel;
}

void ScrollingBringingIntoViewEventArgs::Cancel(bool value)
{
    m_cancel = value;
}

void ScrollingBringingIntoViewEventArgs::OffsetsChangeId(int32_t offsetsChangeId)
{
    m_offsetsChangeId = offsetsChangeId;
}

void ScrollingBringingIntoViewEventArgs::RequestEventArgs(const winrt::BringIntoViewRequestedEventArgs& requestEventArgs)
{
    m_requestEventArgs = requestEventArgs;
}

void ScrollingBringingIntoViewEventArgs::TargetOffsets(double targetHorizontalOffset, double targetVerticalOffset)
{
    m_targetHorizontalOffset = targetHorizontalOffset;
    m_targetVerticalOffset = targetVerticalOffset;
}
