// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollerTrace.h"
#include "ScrollerBringingIntoViewEventArgs.h"

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

int32_t ScrollerBringingIntoViewEventArgs::ViewChangeId()
{
    return m_viewChangeId;
}

bool ScrollerBringingIntoViewEventArgs::Cancel()
{
    return m_cancel;
}

void ScrollerBringingIntoViewEventArgs::Cancel(bool value)
{
    m_cancel = value;
}

void ScrollerBringingIntoViewEventArgs::ViewChangeId(int32_t viewChangeId)
{
    m_viewChangeId = viewChangeId;
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
