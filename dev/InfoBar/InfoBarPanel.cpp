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
    int nItems = 0;

    for (winrt::UIElement const& child : Children())
    {
        child.Measure(availableSize);
        const auto childDesiredSize = child.DesiredSize();

        if (childDesiredSize.Width != 0 && childDesiredSize.Height != 0)
        {
            const auto horizontalMargin = winrt::InfoBarPanel::GetHorizontalMargin(child);
            totalWidth += childDesiredSize.Width + (nItems > 0 ? (float)horizontalMargin.Left : 0) + (float)horizontalMargin.Right;

            const auto verticalMargin = winrt::InfoBarPanel::GetVerticalMargin(child);
            totalHeight += childDesiredSize.Height + (nItems > 0 ? (float)verticalMargin.Top : 0) + (float)verticalMargin.Bottom;

            // ### maybe this needs to be fixed with the margins and stuff.
            if (childDesiredSize.Width > widthOfWidest)
            {
                widthOfWidest = childDesiredSize.Width;
            }

            if (childDesiredSize.Height > heightOfTallest)
            {
                heightOfTallest = childDesiredSize.Height;
            }

            nItems++;
        }
    }

    // Since this panel is inside a *-sized grid column, availableSize.Width should not be infinite
    if (nItems == 1 || totalWidth > availableSize.Width)
    {
        m_isVertical = true;
        const auto verticalMargin = winrt::InfoBarPanel::GetVerticalMargin(*this);

        desiredSize.Width = widthOfWidest;
        desiredSize.Height = totalHeight + (float)verticalMargin.Top + (float)verticalMargin.Bottom;
    }
    else
    {
        m_isVertical = false;
        const auto horizontalMargin = winrt::InfoBarPanel::GetHorizontalMargin(*this);

        desiredSize.Width = totalWidth + (float)horizontalMargin.Left + (float)horizontalMargin.Right;
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
        float verticalOffset = (float)winrt::InfoBarPanel::GetVerticalMargin(*this).Top;
        bool hasPreviousElement = false;
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
        OutputDebugString(L"InfoBarPanel::ArrangeOverride: layout horizontal\n");

        // Layout elements horizontally
        float horizontalOffset = (float)winrt::InfoBarPanel::GetHorizontalMargin(*this).Left;
        bool hasPreviousElement = false;
        for (winrt::UIElement const& child : Children())
        {
            if (auto childAsFe = child.try_as<winrt::FrameworkElement>())
            {
                auto const desiredSize = child.DesiredSize();
                if (desiredSize.Width != 0 && desiredSize.Height != 0)
                {
                    auto horizontalMargin = winrt::InfoBarPanel::GetHorizontalMargin(child);

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

