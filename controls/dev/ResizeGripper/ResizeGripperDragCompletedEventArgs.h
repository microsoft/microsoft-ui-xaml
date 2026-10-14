// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ResizeGripperDragCompletedEventArgs.g.h"

class ResizeGripperDragCompletedEventArgs :
    public winrt::implementation::ResizeGripperDragCompletedEventArgsT<ResizeGripperDragCompletedEventArgs>
{
public:
    ResizeGripperDragCompletedEventArgs(double totalDelta, bool canceled);

    double TotalDelta();
    bool Canceled();

private:
    double m_totalDelta{ 0.0 };
    bool m_canceled{ false };
};
