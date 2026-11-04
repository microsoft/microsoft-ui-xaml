// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ResizeGripperDragDeltaEventArgs.g.h"

class ResizeGripperDragDeltaEventArgs :
    public winrt::implementation::ResizeGripperDragDeltaEventArgsT<ResizeGripperDragDeltaEventArgs>
{
public:
    ResizeGripperDragDeltaEventArgs(double delta, double totalDelta);

    double Delta();
    double TotalDelta();

private:
    double m_delta{ 0.0 };
    double m_totalDelta{ 0.0 };
};
