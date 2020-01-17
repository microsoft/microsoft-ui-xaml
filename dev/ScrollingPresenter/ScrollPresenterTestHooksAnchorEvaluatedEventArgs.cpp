// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollingPresenterTestHooksAnchorEvaluatedEventArgs.h"

ScrollingPresenterTestHooksAnchorEvaluatedEventArgs::ScrollingPresenterTestHooksAnchorEvaluatedEventArgs(
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

#pragma region IScrollingPresenterTestHooksAnchorEvaluatedEventArgs

winrt::UIElement ScrollingPresenterTestHooksAnchorEvaluatedEventArgs::AnchorElement()
{
    return m_anchorElement.get();
}

double ScrollingPresenterTestHooksAnchorEvaluatedEventArgs::ViewportAnchorPointHorizontalOffset()
{
    return m_viewportAnchorPointHorizontalOffset;
}

double ScrollingPresenterTestHooksAnchorEvaluatedEventArgs::ViewportAnchorPointVerticalOffset()
{
    return m_viewportAnchorPointVerticalOffset;
}

#pragma endregion
