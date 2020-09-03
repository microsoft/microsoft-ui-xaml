// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "InfoBarPanel.h"

winrt::Size InfoBarPanel::MeasureOverride(winrt::Size const& availableSize)
{
    WCHAR strOut[1024];
    StringCchPrintf(strOut, ARRAYSIZE(strOut), L"InfoBarPanel::MeasureOverride: availableSize %f %f\n", availableSize.Width, availableSize.Height);
    OutputDebugString(strOut);

    winrt::Size desiredSize{};

    float totalWidth = 0;
    float totalHeight = 0;
    float widthOfWidest = 0;
    float heightOfTallest = 0;

    for (winrt::UIElement const& child : Children())
    {
        child.Measure(availableSize);
        const auto childDesiredSize = child.DesiredSize();

        if (childDesiredSize.Width != 0 && childDesiredSize.Height != 0)
        {
            const auto horizontalMargin = winrt::InfoBarPanel::GetHorizontalMargin(child);
            totalWidth += childDesiredSize.Width + (float)horizontalMargin.Left + (float)horizontalMargin.Right;

            const auto verticalMargin = winrt::InfoBarPanel::GetVerticalMargin(child);
            totalHeight += childDesiredSize.Height + (float)verticalMargin.Top + (float)verticalMargin.Bottom;

            // ### maybe this needs to be fixed with the margins and stuff.
            if (childDesiredSize.Width > widthOfWidest)
            {
                widthOfWidest = childDesiredSize.Width;
            }

            if (childDesiredSize.Height > heightOfTallest)
            {
                heightOfTallest = childDesiredSize.Height;
            }
        }
    }

    // Since this panel is inside a *-sized grid column, availableSize.Width should not be infinite
    if (totalWidth > availableSize.Width)
    {
        m_isVertical = true;
        desiredSize.Width = widthOfWidest;
        desiredSize.Height = totalHeight;
    }
    else
    {
        m_isVertical = false;
        desiredSize.Width = totalWidth;
        desiredSize.Height = heightOfTallest;
    }

    StringCchPrintf(strOut, ARRAYSIZE(strOut), L"InfoBarPanel::MeasureOverride: %s layout, size %f %f\n",
        m_isVertical ? L"vertical" : L"horizontal", desiredSize.Width, desiredSize.Height);
    OutputDebugString(strOut);

    return desiredSize;
}

winrt::Size InfoBarPanel::ArrangeOverride(winrt::Size const& finalSize)
{
    winrt::Size result = finalSize;

    WCHAR strOut[1024];
    StringCchPrintf(strOut, ARRAYSIZE(strOut), L"InfoBarPanel::ArrangeOverride: finalSize %f %f\n", finalSize.Width, finalSize.Height);
    OutputDebugString(strOut);

    if (m_isVertical)
    {
        OutputDebugString(L"InfoBarPanel::ArrangeOverride: layout vertical\n");

        // Layout elements vertically
        float verticalOffset = 0.0;
        for (winrt::UIElement const& child : Children())
        {
            if (auto childAsFe = child.try_as<winrt::FrameworkElement>())
            {
                auto const desiredSize = child.DesiredSize();
                if (desiredSize.Width != 0 && desiredSize.Height != 0)
                {
                    const auto verticalMargin = winrt::InfoBarPanel::GetVerticalMargin(child);

                    StringCchPrintf(strOut, ARRAYSIZE(strOut), L" - InfoBarPanel::ArrangeOverride: child height %f\n", desiredSize.Height);
                    OutputDebugString(strOut);

                    verticalOffset += (float)verticalMargin.Top;
                    child.Arrange(winrt::Rect{ (float)verticalMargin.Left, verticalOffset, desiredSize.Width, desiredSize.Height });
                    verticalOffset += desiredSize.Height + (float)verticalMargin.Bottom;
                }
            }
        }
    }
    else
    {
        OutputDebugString(L"InfoBarPanel::ArrangeOverride: layout horizontal\n");

        // Layout elements horizontally
        // ###.... can this just to a stackpanel?
        float horizontalOffset = 0.0;
        for (winrt::UIElement const& child : Children())
        {
            if (auto childAsFe = child.try_as<winrt::FrameworkElement>())
            {
                auto const desiredSize = child.DesiredSize();
                if (desiredSize.Width != 0 && desiredSize.Height != 0)
                {
                    auto horizontalMargin = winrt::InfoBarPanel::GetHorizontalMargin(child);

                    horizontalOffset += (float)horizontalMargin.Left;
                    child.Arrange(winrt::Rect{ horizontalOffset, (float)horizontalMargin.Top, desiredSize.Width, finalSize.Height });
                    horizontalOffset += desiredSize.Width + (float)horizontalMargin.Right;
                }
            }
        }
    }

    return result;
}

