// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AnnotatedScrollBarDetailLabelRequestedEventArgs.g.h"

class AnnotatedScrollBarDetailLabelRequestedEventArgs :
    public ReferenceTracker<AnnotatedScrollBarDetailLabelRequestedEventArgs, winrt::implementation::AnnotatedScrollBarDetailLabelRequestedEventArgsT, winrt::composable, winrt::composing>
{
public:
    AnnotatedScrollBarDetailLabelRequestedEventArgs(double scrollOffset);

#pragma region IAnnotatedScrollBarDetailLabelRequestedEventArgs
    winrt::IInspectable Content();
    void Content(const winrt::IInspectable& content);
    double ScrollOffset();
#pragma endregion

private:
    tracker_ref<winrt::IInspectable> m_content{ this };
    double m_scrollOffset{ 0.0 };
};
