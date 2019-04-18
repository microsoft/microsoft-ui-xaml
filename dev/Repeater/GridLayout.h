// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NonVirtualizingLayout.h"
#include "GridLayout.g.h"
#include "GridLayout.properties.h"

class GridLayout :
    public ReferenceTracker<GridLayout, winrt::implementation::GridLayoutT, NonVirtualizingLayout>,
    public GridLayoutProperties
{
public:
    GridLayout();

    double ColumnGap();
    void ColumnGap(double const& value);

    double RowGap();
    void RowGap(double const& value);

    winrt::GridJustifyItems JustifyItems();
    void JustifyItems(winrt::GridJustifyItems const& value);

    winrt::GridAlignItems AlignItems();
    void AlignItems(winrt::GridAlignItems const& value);

    winrt::GridJustifyContent JustifyContent();
    void JustifyContent(winrt::GridJustifyContent const& value);

    winrt::GridAlignContent AlignContent();
    void AlignContent(winrt::GridAlignContent const& value);

    winrt::GridAutoFlow AutoFlow();
    void AutoFlow(winrt::GridAutoFlow const& value);

#pragma region INonVirtualizingLayoutOverrides
    void InitializeForContextCore(winrt::LayoutContext const& context);
    void UninitializeForContextCore(winrt::LayoutContext const& context);

    winrt::Size MeasureOverride(winrt::LayoutContext const& context, winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::LayoutContext const& context, winrt::Size const& finalSize);
#pragma endregion

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    double m_columnGap;
    double m_rowGap;
    winrt::GridJustifyItems m_justifyItems;
    winrt::GridAlignItems m_alignItems;
    winrt::GridJustifyContent m_justifyContent;
    winrt::GridAlignContent m_alignContent;
    winrt::GridAutoFlow m_autoFlow;
};
