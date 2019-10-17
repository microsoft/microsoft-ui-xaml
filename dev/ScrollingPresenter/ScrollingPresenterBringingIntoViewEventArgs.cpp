// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollerTrace.h"
#include "ScrollerBringingIntoViewEventArgs.h"

winrt::SnapPointsMode ScrollerBringingIntoViewEventArgs::SnapPointsMode()
{
    return m_snapPointsMode;
}

void ScrollerBringingIntoViewEventArgs::SnapPointsMode(winrt::SnapPointsMode snapPointsMode)
{
    m_snapPointsMode = snapPointsMode;
}

winrt::BringIntoViewRequestedEventArgs ScrollerBringingIntoViewEventArgs::RequestEventArgs()
{
    return m_requestEventArgs;
}

double ScrollerBringingIntoViewEventArgs::TargetHorizontalOffset()
{
    return m_targetHorizontalOffset;
}

double ScrollerBringingIntoViewEventArgs::TargetVerticalOffset()
{
    return m_targetVerticalOffset;
}

winrt::ScrollInfo ScrollerBringingIntoViewEventArgs::ScrollInfo()
{
    return winrt::ScrollInfo{ m_offsetsChangeId };
}

bool ScrollerBringingIntoViewEventArgs::Cancel()
{
    return m_cancel;
}

void ScrollerBringingIntoViewEventArgs::Cancel(bool value)
{
    m_cancel = value;
}

void ScrollerBringingIntoViewEventArgs::OffsetsChangeId(int32_t offsetsChangeId)
{
    m_offsetsChangeId = offsetsChangeId;
}

void ScrollerBringingIntoViewEventArgs::RequestEventArgs(const winrt::BringIntoViewRequestedEventArgs& requestEventArgs)
{
    m_requestEventArgs = requestEventArgs;
}

void ScrollerBringingIntoViewEventArgs::TargetOffsets(double targetHorizontalOffset, double targetVerticalOffset)
{
    m_targetHorizontalOffset = targetHorizontalOffset;
    m_targetVerticalOffset = targetVerticalOffset;
}
