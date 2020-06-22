// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollerTestHooksAnchorEvaluatedEventArgs.g.h"

class ScrollerTestHooksAnchorEvaluatedEventArgs :
    public winrt::implementation::ScrollerTestHooksAnchorEvaluatedEventArgsT<ScrollerTestHooksAnchorEvaluatedEventArgs>
{
public:
    ScrollerTestHooksAnchorEvaluatedEventArgs(const winrt::UIElement& anchorElement, double viewportAnchorPointHorizontalOffset, double viewportAanchorPointVerticalOffset);

    // IScrollerTestHooksAnchorEvaluatedEventArgs overrides
    winrt::UIElement AnchorElement();
    double ViewportAnchorPointHorizontalOffset();
    double ViewportAnchorPointVerticalOffset();

private:
    winrt::weak_ref<winrt::UIElement> m_anchorElement;
    double m_viewportAnchorPointHorizontalOffset{ 0.0 };
    double m_viewportAnchorPointVerticalOffset{ 0.0 };
};
