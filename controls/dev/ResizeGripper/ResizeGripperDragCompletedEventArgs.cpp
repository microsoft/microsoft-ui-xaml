// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResizeGripperDragCompletedEventArgs.h"

ResizeGripperDragCompletedEventArgs::ResizeGripperDragCompletedEventArgs(double totalDelta, bool canceled)
    : m_totalDelta(totalDelta)
    , m_canceled(canceled)
{
}

double ResizeGripperDragCompletedEventArgs::TotalDelta()
{
    return m_totalDelta;
}

bool ResizeGripperDragCompletedEventArgs::Canceled()
{
    return m_canceled;
}
