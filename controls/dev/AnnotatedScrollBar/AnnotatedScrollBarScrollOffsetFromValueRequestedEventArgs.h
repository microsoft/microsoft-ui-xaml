// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AnnotatedScrollBarScrollOffsetFromValueRequestedEventArgs.g.h"


class AnnotatedScrollBarScrollOffsetFromValueRequestedEventArgs :
    public ReferenceTracker<AnnotatedScrollBarScrollOffsetFromValueRequestedEventArgs, winrt::implementation::AnnotatedScrollBarScrollOffsetFromValueRequestedEventArgsT, winrt::composable, winrt::composing>
{
public:
    AnnotatedScrollBarScrollOffsetFromValueRequestedEventArgs(double value);

#pragma region IAnnotatedScrollBarScrollOffsetFromValueRequestedEventArgs
    double Value();
    double ScrollOffset();
    void ScrollOffset(double scrollOffset);
#pragma endregion

private:
    double m_value{ 0.0 };
    double m_scrollOffset{ 0.0 };
};
