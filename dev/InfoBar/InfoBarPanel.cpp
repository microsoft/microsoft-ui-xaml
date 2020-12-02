// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "InfoBarPanel.h"

winrt::Size InfoBarPanel::MeasureOverride(winrt::Size const& availableSize)
{
    winrt::Size desiredSize{};

    float totalWidth = 0;
    float totalHeight = 0;
    float widthOfWidest = 0;
    float heightOfTallest = 0;
    float heightOfTallestInHorizontal = 0;
    int nItems = 0;

    const auto parent = this->Parent().try_as<winrt::FrameworkElement>();
    const float minHeight = !parent ? 0.0f : (float)(parent.MinHeight() - (Margin().Top + Margin().Bottom));

    for (winrt::UIElement const& child : Children())
    {
        child.Measure(availableSize);
        const auto childDesiredSize = child.DesiredSize();

        if (childDesiredSize.Width != 0 && childDesiredSize.Height != 0)
        {
            // Add up the width of all items if they were laid out horizontally
            const auto horizontalMargin = winrt::InfoBarPanel::GetHorizontalOrientationMargin(child);
            totalWidth += childDesiredSize.Width + (nItems > 0 ? (float)horizontalMargin.Left : 0) + (float)horizontalMargin.Right;

            // Add up the height of all items if they were laid out vertically
            const auto verticalMargin = winrt::InfoBarPanel::GetVerticalOrientationMargin(child);
            totalHeight += childDesiredSize.Height + (nItems > 0 ? (float)verticalMargin.Top : 0) + (float)verticalMargin.Bottom;

            if (childDesiredSize.Width > widthOfWidest)
            {
                widthOfWidest = childDesiredSize.Width;
            }

            if (childDesiredSize.Height > heightOfTallest)
            {
                heightOfTallest = childDesiredSize.Height;
            }

            const float childHeightInHorizontal = childDesiredSize.Height + (float)horizontalMargin.Top + float(horizontalMargin.Bottom);
            if (childHeightInHorizontal > heightOfTallestInHorizontal)
            {
                heightOfTallestInHorizontal = childHeightInHorizontal;
            }

            nItems++;
        }
    }

    // Since this panel is inside a *-sized grid column, availableSize.Width should not be infinite
    // If there is only one item inside the panel, we will count it as vertical (the margins work out better that way)
    // Also, if the height of any item is taller than the desired min height of the InfoBar,
    // the items should be laid out vertically even though they may seem to fit due to text wrapping.
    if (nItems == 1 || totalWidth > availableSize.Width || (minHeight > 0 && heightOfTallestInHorizontal > minHeight))
    {
        m_isVertical = true;
        const auto verticalMargin = VerticalOrientationPadding();

        desiredSize.Width = widthOfWidest;
        desiredSize.Height = totalHeight + (float)verticalMargin.Top + (float)verticalMargin.Bottom;
    }
    else
    {
        m_isVertical = false;
        const auto horizontalMargin = HorizontalOrientationPadding();

        desiredSize.Width = totalWidth + (float)horizontalMargin.Left + (float)horizontalMargin.Right;
        desiredSize.Height = heightOfTallest;
    }

    return desiredSize;
}

winrt::Size InfoBarPanel::ArrangeOverride(winrt::Size const& finalSize)
{
    winrt::Size result = finalSize;

    if (m_isVertical)
    {
        // Layout elements vertically
        float verticalOffset = (float)VerticalOrientationPadding().Top;
        bool hasPreviousElement = false;
        for (winrt::UIElement const& child : Children())
        {
            if (auto childAsFe = child.try_as<winrt::FrameworkElement>())
            {
                auto const desiredSize = child.DesiredSize();
                if (desiredSize.Width != 0 && desiredSize.Height != 0)
                {
                    const auto verticalMargin = winrt::InfoBarPanel::GetVerticalOrientationMargin(child);

                    verticalOffset += hasPreviousElement ? (float)verticalMargin.Top : 0;
                    child.Arrange(winrt::Rect{ (float)verticalMargin.Left, verticalOffset, desiredSize.Width, desiredSize.Height });
                    verticalOffset += desiredSize.Height + (float)verticalMargin.Bottom;

                    hasPreviousElement = true;
                }
            }
        }
    }
    else
    {
        // Layout elements horizontally
        float horizontalOffset = (float)HorizontalOrientationPadding().Left;
        bool hasPreviousElement = false;
        for (winrt::UIElement const& child : Children())
        {
            if (auto childAsFe = child.try_as<winrt::FrameworkElement>())
            {
                auto const desiredSize = child.DesiredSize();
                if (desiredSize.Width != 0 && desiredSize.Height != 0)
                {
                    auto horizontalMargin = winrt::InfoBarPanel::GetHorizontalOrientationMargin(child);

                    horizontalOffset += hasPreviousElement ? (float)horizontalMargin.Left : 0;
                    child.Arrange(winrt::Rect{ horizontalOffset, (float)horizontalMargin.Top, desiredSize.Width, finalSize.Height });
                    horizontalOffset += desiredSize.Width + (float)horizontalMargin.Right;

                    hasPreviousElement = true;
                }
            }
        }
    }

    return result;
}

