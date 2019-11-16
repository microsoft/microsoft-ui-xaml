// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "Utils.h"
#include "ColumnMajorUniformToLargestGridLayout.h"

CppWinRTActivatableClassWithBasicFactory(ColumnMajorUniformToLargestGridLayout);

// IItemsControlOverrides

ColumnMajorUniformToLargestGridLayout::ColumnMajorUniformToLargestGridLayout()
{
}

winrt::Size ColumnMajorUniformToLargestGridLayout::MeasureOverride(
    winrt::NonVirtualizingLayoutContext const& context,
    winrt::Size const& availableSize)
{
    if (auto const children = context.Children())
    {
        auto const maxColumns = std::max(1, MaximumColumns());
        MUX_ASSERT(maxColumns > 0);
        auto const maxItemsPerColumn = static_cast<int>(std::ceil(static_cast<double>(children.Size()) / static_cast<double>(maxColumns)));

        for (auto const child : children)
        {
            child.Measure(availableSize);
        }
        auto const largestChildSize = LargestChildSize(context);

        auto const actualColumnCount = static_cast<float>(std::min(
            static_cast<double>(maxColumns),
            static_cast<double>(children.Size())));
        return winrt::Size(
            (largestChildSize.Width * actualColumnCount) + 
            (ColumnSpacing() * (actualColumnCount - 1)),
            (largestChildSize.Height * maxItemsPerColumn) +
            (RowSpacing() * (maxItemsPerColumn - 1))
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
        auto const maxColumns = std::max(1, MaximumColumns());
        MUX_ASSERT(maxColumns > 0);
        auto const itemCount = children.Size();
        auto const minitemsPerColumn = static_cast<int>(std::floor(static_cast<double>(itemCount) / static_cast<double>(maxColumns)));
        auto const numberOfColumnsWithExtraElements = static_cast<int>(itemCount % maxColumns);

        auto const largestChildSize = LargestChildSize(context);
        auto const columnSpacing = ColumnSpacing();
        auto const rowSpacing = RowSpacing();

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
                    horizontalOffset += largestChildSize.Width + columnSpacing;
                    verticalOffset = 0.0;
                    column++;
                }
                else
                {
                    verticalOffset += largestChildSize.Height + rowSpacing;
                }
            }
            else
            {
                auto const indexAfterExtraLargeColumns = index - (numberOfColumnsWithExtraElements * (minitemsPerColumn + 1));
                if (indexAfterExtraLargeColumns % minitemsPerColumn == minitemsPerColumn - 1)
                {
                    horizontalOffset += largestChildSize.Width + columnSpacing;
                    verticalOffset = 0.0;
                    column++;
                }
                else
                {
                    verticalOffset += largestChildSize.Height + rowSpacing;
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

winrt::Size ColumnMajorUniformToLargestGridLayout::LargestChildSize(winrt::NonVirtualizingLayoutContext const& context)
{
    auto largestChildWidth = 0.0f;
    auto largestChildHeight = 0.0f;
    for (auto const child : context.Children())
    {
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
}

void ColumnMajorUniformToLargestGridLayout::OnColumnSpacingPropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    InvalidateMeasure();
}

void ColumnMajorUniformToLargestGridLayout::OnRowSpacingPropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    InvalidateMeasure();
}

void ColumnMajorUniformToLargestGridLayout::OnMaximumColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    InvalidateMeasure();
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
