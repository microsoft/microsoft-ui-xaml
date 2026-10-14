// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResizeGripperDragDeltaEventArgs.h"

ResizeGripperDragDeltaEventArgs::ResizeGripperDragDeltaEventArgs(double delta, double totalDelta)
    : m_delta(delta)
    , m_totalDelta(totalDelta)
{
}

double ResizeGripperDragDeltaEventArgs::Delta()
{
    return m_delta;
}

double ResizeGripperDragDeltaEventArgs::TotalDelta()
{
    return m_totalDelta;
}
