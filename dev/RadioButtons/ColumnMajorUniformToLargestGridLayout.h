#pragma once
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "common.h"
#include "ColumnMajorUniformToLargestGridLayout.g.h"
#include "ColumnMajorUniformToLargestGridLayout.properties.h"
#include "Layout.h"
#include "NonVirtualizingLayout.h"

class ColumnMajorUniformToLargestGridLayout :
    public ReferenceTracker<ColumnMajorUniformToLargestGridLayout, winrt::implementation::ColumnMajorUniformToLargestGridLayoutT, NonVirtualizingLayout>,
    public ColumnMajorUniformToLargestGridLayoutProperties
{
public:
    ColumnMajorUniformToLargestGridLayout() = default;
    winrt::Size MeasureOverride(
        winrt::NonVirtualizingLayoutContext const& context,
        winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(
        winrt::NonVirtualizingLayoutContext const& context,
        winrt::Size const& finalSize);

    void OnColumnSpacingPropertyChanged(const winrt::DependencyPropertyChangedEventArgs&);
    void OnRowSpacingPropertyChanged(const winrt::DependencyPropertyChangedEventArgs&);
    void OnMaxColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs&);

    static void ValidateGreaterThanZero(int value);

    //Testhooks helpers, only function while m_testHooksEnabled == true
    void SetTestHooksEnabled(bool enabled);
    int GetRows();
    int GetColumns();
    int GetLargerColumns();
    winrt::event_token LayoutChanged(winrt::TypedEventHandler<winrt::ColumnMajorUniformToLargestGridLayout, winrt::IInspectable> const& value);
    void LayoutChanged(winrt::event_token const& token);

private:
    int CalculateColumns(int childCount, float maxItemWidth, float availableWidth);
    int m_actualColumnCount{ 1 };
    winrt::Size m_largestChildSize{ 0,0 };

    //Testhooks helpers, only function while m_testHooksEnabled == true
    bool m_testHooksEnabled{ false };
    winrt::event<winrt::TypedEventHandler<winrt::ColumnMajorUniformToLargestGridLayout, winrt::IInspectable>> m_layoutChangedEventSource;

    int m_rows{ -1 };
    int m_columns{ -1 };
    int m_largerColumns{ -1 };
};

