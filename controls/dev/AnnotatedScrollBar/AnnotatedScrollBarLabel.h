// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AnnotatedScrollBarLabel.g.h"

class AnnotatedScrollBarLabel :
    public ReferenceTracker<AnnotatedScrollBarLabel, winrt::implementation::AnnotatedScrollBarLabelT, winrt::composable, winrt::composing>
{

public:
    AnnotatedScrollBarLabel(const winrt::IInspectable& content, double scrollOffset);
    winrt::IInspectable Content();
    double ScrollOffset();

private:
    tracker_ref<winrt::IInspectable> m_content{ this };
    double m_scrollOffset{ 0.0 };
};
