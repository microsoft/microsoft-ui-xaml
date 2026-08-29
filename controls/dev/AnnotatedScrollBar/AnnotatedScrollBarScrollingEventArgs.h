// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AnnotatedScrollBarScrollingEventArgs.g.h"

class AnnotatedScrollBarScrollingEventArgs :
    public ReferenceTracker<AnnotatedScrollBarScrollingEventArgs, winrt::implementation::AnnotatedScrollBarScrollingEventArgsT, winrt::composable, winrt::composing>
{
public:
    AnnotatedScrollBarScrollingEventArgs(double scrollOffset, winrt::AnnotatedScrollBarScrollingEventKind scrollingEventKind);

#pragma region IAnnotatedScrollBarScrollingEventArgs
    winrt::AnnotatedScrollBarScrollingEventKind ScrollingEventKind();
    double ScrollOffset();
    void Cancel(bool cancel);
    bool Cancel();
#pragma endregion

private:
    winrt::AnnotatedScrollBarScrollingEventKind m_scrollingEventKind;
    double m_scrollOffset{ 0.0 };
    bool m_cancel{ false };
};
