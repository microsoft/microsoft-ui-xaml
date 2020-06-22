// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollerTestHooksAnchorEvaluatedEventArgs.h"

ScrollerTestHooksAnchorEvaluatedEventArgs::ScrollerTestHooksAnchorEvaluatedEventArgs(
    const winrt::UIElement& anchorElement,
    double viewportAnchorPointHorizontalOffset,
    double viewportAnchorPointVerticalOffset)
{
    if (anchorElement)
    {
        m_anchorElement = winrt::make_weak(anchorElement);
    }
    m_viewportAnchorPointHorizontalOffset = viewportAnchorPointHorizontalOffset;
    m_viewportAnchorPointVerticalOffset = viewportAnchorPointVerticalOffset;
}

#pragma region IScrollerTestHooksAnchorEvaluatedEventArgs

winrt::UIElement ScrollerTestHooksAnchorEvaluatedEventArgs::AnchorElement()
{
    return m_anchorElement.get();
}

double ScrollerTestHooksAnchorEvaluatedEventArgs::ViewportAnchorPointHorizontalOffset()
{
    return m_viewportAnchorPointHorizontalOffset;
}

double ScrollerTestHooksAnchorEvaluatedEventArgs::ViewportAnchorPointVerticalOffset()
{
    return m_viewportAnchorPointVerticalOffset;
}

#pragma endregion
