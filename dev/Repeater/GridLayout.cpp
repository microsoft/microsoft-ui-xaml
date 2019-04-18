// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "GridLayout.h"
#include "RuntimeProfiler.h"
#include "Vector.h"

GridLayout::GridLayout()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_GridLayout);

    m_templateColumns = winrt::make<Vector<winrt::GridTrackInfo>>();
    m_templateRows = winrt::make<Vector<winrt::GridTrackInfo>>();
    m_autoColumns = winrt::make<Vector<winrt::GridTrackInfo>>();
    m_autoRows = winrt::make<Vector<winrt::GridTrackInfo>>();
}

winrt::IVector<winrt::GridTrackInfo> GridLayout::TemplateColumns()
{
    return m_templateColumns;
}

void GridLayout::TemplateColumns(winrt::IVector<winrt::GridTrackInfo> const& value)
{
    m_templateColumns = value;
}

winrt::IVector<winrt::GridTrackInfo> GridLayout::TemplateRows()
{
    return m_templateRows;
}

void GridLayout::TemplateRows(winrt::IVector<winrt::GridTrackInfo> const& value)
{
    m_templateRows = value;
}

double GridLayout::ColumnGap()
{
    return m_columnGap;
}

void GridLayout::ColumnGap(double const& value)
{
    m_columnGap = value;
}

double GridLayout::RowGap()
{
    return m_rowGap;
}

void GridLayout::RowGap(double const& value)
{
    m_rowGap = value;
}

winrt::GridJustifyItems GridLayout::JustifyItems()
{
    return m_justifyItems;
}

void GridLayout::JustifyItems(winrt::GridJustifyItems const& value)
{
    m_justifyItems = value;
}

winrt::GridAlignItems GridLayout::AlignItems()
{
    return m_alignItems;
}

void GridLayout::AlignItems(winrt::GridAlignItems const& value)
{
    m_alignItems = value;
}

winrt::GridJustifyContent GridLayout::JustifyContent()
{
    return m_justifyContent;
}

void GridLayout::JustifyContent(winrt::GridJustifyContent const& value)
{
    m_justifyContent = value;
}

winrt::GridAlignContent GridLayout::AlignContent()
{
    return m_alignContent;
}

void GridLayout::AlignContent(winrt::GridAlignContent const& value)
{
    m_alignContent = value;
}

winrt::IVector<winrt::GridTrackInfo> GridLayout::AutoColumns()
{
    return m_autoColumns;
}

void GridLayout::AutoColumns(winrt::IVector<winrt::GridTrackInfo> const& value)
{
    m_autoColumns = value;
}

winrt::IVector<winrt::GridTrackInfo> GridLayout::AutoRows()
{
    return m_autoRows;
}

void GridLayout::AutoRows(winrt::IVector<winrt::GridTrackInfo> const& value)
{
    m_autoRows = value;
}

winrt::GridAutoFlow GridLayout::AutoFlow()
{
    return m_autoFlow;
}

void GridLayout::AutoFlow(winrt::GridAutoFlow const& value)
{
    m_autoFlow = value;
}

void GridLayout::InitializeForContextCore(winrt::LayoutContext const& context)
{
#if FALSE
    auto state = context.LayoutState();
    winrt::com_ptr<GridLayoutState> gridState = nullptr;
    if (state)
    {
        gridState = GetAsGridState(state);
    }

    if (!gridState)
    {
        if (state)
        {
            throw winrt::hresult_error(E_FAIL, L"LayoutState must derive from GridLayoutState.");
        }

        // Custom deriving layouts could potentially be stateful.
        // If that is the case, we will just create the base state required by ourselves.
        gridState = winrt::make_self<GridLayoutState>();
    }

    gridState->InitializeForContext(context, this);
#endif
}

void GridLayout::UninitializeForContextCore(winrt::LayoutContext const& context)
{
#if FALSE
    auto gridState = GetAsGridState(context.LayoutState());
    gridState->UninitializeForContext(context);
#endif
}

winrt::Size GridLayout::MeasureOverride(
    winrt::LayoutContext const& context,
    winrt::Size const& availableSize)
{
    return { availableSize.Width, availableSize.Height > 9999 ? 9999 : availableSize.Height };
}

winrt::Size GridLayout::ArrangeOverride(
    winrt::LayoutContext const& context,
    winrt::Size const& finalSize)
{
    return { finalSize.Width, finalSize.Height };
}

void GridLayout::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
}
