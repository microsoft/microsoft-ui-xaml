// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "Utils.h"
#include "ColumnMajorUniformToLargestGridLayout.h"

winrt::Size ColumnMajorUniformToLargestGridLayout::MeasureOverride(
    winrt::NonVirtualizingLayoutContext const& context,
    winrt::Size const& availableSize)
{
    if (auto const children = context.Children())
    {
        m_largestChildSize = [children, availableSize]()
        {
            auto largestChildWidth = 0.0f;
            auto largestChildHeight = 0.0f;
            for (auto const child : children)
            {
                child.Measure(availableSize);
                auto const desiredSize = child.DesiredSize();
                if (desiredSize.Width > largestChildWidth)
                {
                    largestChildWidth = desiredSize.Width;
                }
                if (desiredSize.Height > largestChildHeight)
                {
                    largestChildHeight = desiredSize.Height;
                }
            }
            return winrt::Size(largestChildWidth, largestChildHeight);
        }();

        m_actualColumnCount = CalculateColumns(children.Size(), m_largestChildSize.Width, availableSize.Width);
        auto const maxItemsPerColumn = static_cast<int>(std::ceil(static_cast<double>(children.Size()) / static_cast<double>(m_actualColumnCount)));
        return winrt::Size(
            (m_largestChildSize.Width * m_actualColumnCount) +
            (static_cast<float>(ColumnSpacing()) * (m_actualColumnCount - 1)),
            (m_largestChildSize.Height * maxItemsPerColumn) +
            (static_cast<float>(RowSpacing()) * (maxItemsPerColumn - 1))
        );
    }
    return winrt::Size(0, 0);
}

winrt::Size ColumnMajorUniformToLargestGridLayout::ArrangeOverride(
    winrt::NonVirtualizingLayoutContext const& context,
    winrt::Size const& finalSize)
{
    if (auto const children = context.Children())
    {
        auto const itemCount = children.Size();
        auto const minitemsPerColumn = static_cast<int>(std::floor(static_cast<float>(itemCount) / m_actualColumnCount));
        auto const numberOfColumnsWithExtraElements = static_cast<int>(itemCount % static_cast<int>(m_actualColumnCount));

        auto const columnSpacing = static_cast<float>(ColumnSpacing());
        auto const rowSpacing = static_cast<float>(RowSpacing());

        auto horizontalOffset = 0.0f;
        auto verticalOffset = 0.0f;
        auto index = 0;
        auto column = 0;
        for (auto const child : children)
        {
            auto const desiredSize = child.DesiredSize();
            child.Arrange(winrt::Rect{ horizontalOffset, verticalOffset, desiredSize.Width, desiredSize.Height });
            if (column < numberOfColumnsWithExtraElements)
            {
                if (index % (minitemsPerColumn + 1) == minitemsPerColumn)
                {
                    horizontalOffset += m_largestChildSize.Width + columnSpacing;
                    verticalOffset = 0.0;
                    column++;
                }
                else
                {
                    verticalOffset += m_largestChildSize.Height + rowSpacing;
                }
            }
            else
            {
                auto const indexAfterExtraLargeColumns = index - (numberOfColumnsWithExtraElements * (minitemsPerColumn + 1));
                if (indexAfterExtraLargeColumns % minitemsPerColumn == minitemsPerColumn - 1)
                {
                    horizontalOffset += m_largestChildSize.Width + columnSpacing;
                    verticalOffset = 0.0;
                    column++;
                }
                else
                {
                    verticalOffset += m_largestChildSize.Height + rowSpacing;
                }
            }
            index++;
        }

        if (m_testHooksEnabled)
        {
            //Testhooks setup
            if (m_largerColumns != numberOfColumnsWithExtraElements ||
                m_columns != column ||
                m_rows != minitemsPerColumn)
            {
                m_largerColumns = numberOfColumnsWithExtraElements;
                m_columns = column;
                m_rows = minitemsPerColumn;

                m_layoutChangedEventSource(*this, nullptr);
            }
        }
    }
    return finalSize;
}

void ColumnMajorUniformToLargestGridLayout::OnColumnSpacingPropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    InvalidateMeasure();
}

void ColumnMajorUniformToLargestGridLayout::OnRowSpacingPropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    InvalidateMeasure();
}

void ColumnMajorUniformToLargestGridLayout::OnMaxColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    InvalidateMeasure();
}

int ColumnMajorUniformToLargestGridLayout::CalculateColumns(int childCount, float maxItemWidth, float availableWidth)
{
    /*
    --------------------------------------------------------------
    |      |-----------|-----------| | widthNeededForExtraColumn |
    |                                |                           |
    |      |------|    |------|      | ColumnSpacing             |
    | |----|      |----|      |----| | maxItemWidth              |
    |  O RB        O RB        O RB  |                           |
    --------------------------------------------------------------
    */

    // Every column execpt the first takes this ammount of space to fit on screen.
    auto const widthNeededForExtraColumn = ColumnSpacing() + maxItemWidth;
    // The number of columns from data and api ignoring available space
    auto const requestedColumnCount = std::min(MaxColumns(), childCount);

    // If columns can be added with effectively 0 extra space return as many columns as needed.
    if (widthNeededForExtraColumn < std::numeric_limits<float>::epsilon())
    {
        return requestedColumnCount;
    }

    auto const extraWidthAfterFirstColumn = availableWidth - maxItemWidth;
    auto const maxExtraColumns = std::max(0.0, std::floor(extraWidthAfterFirstColumn / widthNeededForExtraColumn));

    // The smaller of number of columns from data and api and
    // the number of columns the available space can support
    auto const effectiveColumnCount = std::min(static_cast<double>(requestedColumnCount), maxExtraColumns + 1);
    // return 1 even if there isn't any data
    return std::max(1, static_cast<int>(effectiveColumnCount));
}

void ColumnMajorUniformToLargestGridLayout::ValidateGreaterThanZero(int value)
{
    if (value <= 0)
    {
        throw winrt::hresult_invalid_argument();
    }
}

//Testhooks helpers, only function while m_testHooksEnabled == true
void ColumnMajorUniformToLargestGridLayout::SetTestHooksEnabled(bool enabled)
{
    m_testHooksEnabled = enabled;
}

int ColumnMajorUniformToLargestGridLayout::GetRows()
{
    return m_rows;
}

int ColumnMajorUniformToLargestGridLayout::GetColumns()
{
    return m_columns;
}

int ColumnMajorUniformToLargestGridLayout::GetLargerColumns()
{
    return m_largerColumns;
}

winrt::event_token ColumnMajorUniformToLargestGridLayout::LayoutChanged(winrt::TypedEventHandler<winrt::ColumnMajorUniformToLargestGridLayout, winrt::IInspectable> const& value)
{
    return m_layoutChangedEventSource.add(value);
}

void ColumnMajorUniformToLargestGridLayout::LayoutChanged(winrt::event_token const& token)
{
    m_layoutChangedEventSource.remove(token);
}
