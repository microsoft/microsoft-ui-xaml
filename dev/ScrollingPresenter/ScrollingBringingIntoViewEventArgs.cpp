// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollingPresenterTrace.h"
#include "ScrollingPresenterBringingIntoViewEventArgs.h"

winrt::SnapPointsMode ScrollingPresenterBringingIntoViewEventArgs::SnapPointsMode()
{
    return m_snapPointsMode;
}

void ScrollingPresenterBringingIntoViewEventArgs::SnapPointsMode(winrt::SnapPointsMode snapPointsMode)
{
    m_snapPointsMode = snapPointsMode;
}

winrt::BringIntoViewRequestedEventArgs ScrollingPresenterBringingIntoViewEventArgs::RequestEventArgs()
{
    return m_requestEventArgs;
}

double ScrollingPresenterBringingIntoViewEventArgs::TargetHorizontalOffset()
{
    return m_targetHorizontalOffset;
}

double ScrollingPresenterBringingIntoViewEventArgs::TargetVerticalOffset()
{
    return m_targetVerticalOffset;
}

winrt::ScrollInfo ScrollingPresenterBringingIntoViewEventArgs::ScrollInfo()
{
    return winrt::ScrollInfo{ m_offsetsChangeId };
}

bool ScrollingPresenterBringingIntoViewEventArgs::Cancel()
{
    return m_cancel;
}

void ScrollingPresenterBringingIntoViewEventArgs::Cancel(bool value)
{
    m_cancel = value;
}

void ScrollingPresenterBringingIntoViewEventArgs::OffsetsChangeId(int32_t offsetsChangeId)
{
    m_offsetsChangeId = offsetsChangeId;
}

void ScrollingPresenterBringingIntoViewEventArgs::RequestEventArgs(const winrt::BringIntoViewRequestedEventArgs& requestEventArgs)
{
    m_requestEventArgs = requestEventArgs;
}

void ScrollingPresenterBringingIntoViewEventArgs::TargetOffsets(double targetHorizontalOffset, double targetVerticalOffset)
{
    m_targetHorizontalOffset = targetHorizontalOffset;
    m_targetVerticalOffset = targetVerticalOffset;
}
