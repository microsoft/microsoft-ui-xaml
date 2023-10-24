// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AnnotatedScrollBarValueFromScrollOffsetRequestedEventArgs.g.h"


class AnnotatedScrollBarValueFromScrollOffsetRequestedEventArgs :
    public ReferenceTracker<AnnotatedScrollBarValueFromScrollOffsetRequestedEventArgs, winrt::implementation::AnnotatedScrollBarValueFromScrollOffsetRequestedEventArgsT, winrt::composable, winrt::composing>
{
public:
    AnnotatedScrollBarValueFromScrollOffsetRequestedEventArgs(double scrollOffset);

#pragma region IAnnotatedScrollBarValueFromScrollOffsetRequestedEventArgs
    double Value();
    void Value(double value);
    double ScrollOffset();
#pragma endregion

private:
    double m_value{ 0.0 };
    double m_scrollOffset{ 0.0 };
};
