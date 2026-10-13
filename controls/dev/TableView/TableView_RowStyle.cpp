// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "TableViewGroupHeader.h"

namespace
{
    // Writes Style only when the answer changed. A null answer clears only a Style the selector
    // wrote, so the app's implicit style and the theme default come back. Returns the new flag.
    bool ApplySelectedStyle(winrt::Control const& container, winrt::Style const& style, bool styleFromSelector)
    {
        if (style)
        {
            if (container.Style() != style)
            {
                container.Style(style);
            }

            return true;
        }

        if (styleFromSelector)
        {
            container.ClearValue(winrt::FrameworkElement::StyleProperty());
        }

        return false;
    }
}

void TableView::ApplyRowStyleSelector(winrt::TableViewRow const& row)
{
    if (!row)
    {
        return;
    }

    winrt::Style style{ nullptr };
    if (auto const selector = RowStyleSelector())
    {
        // Called after SetHierarchyStateInternal, so IsExpandable/Level/IsExpanded are current.
        style = selector.SelectStyle(row.DataContext(), row);
    }

    auto* const rowImpl = winrt::get_self<TableViewRow>(row);
    rowImpl->SetStyleFromSelectorInternal(
        ApplySelectedStyle(row, style, rowImpl->IsStyleFromSelectorInternal()));
}

void TableView::ApplyGroupHeaderStyleSelector(winrt::TableViewGroupHeader const& header)
{
    if (!header)
    {
        return;
    }

    winrt::Style style{ nullptr };
    if (auto const selector = RowStyleSelector())
    {
        // Content is the TableViewGroupInfo set earlier in PrepareGroupHeaderElement.
        style = selector.SelectStyle(header.Content(), header);
    }

    auto* const headerImpl = winrt::get_self<TableViewGroupHeader>(header);
    headerImpl->SetStyleFromSelectorInternal(
        ApplySelectedStyle(header, style, headerImpl->IsStyleFromSelectorInternal()));
}

void TableView::OnRowStyleSelectorPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.OldValue() == args.NewValue())
    {
        return;
    }

    auto const repeater = m_rowsRepeater.get();
    if (!repeater)
    {
        return;
    }

    // Pooled containers (index -1) are skipped; they are re-styled when re-prepared. Same filter
    // as RefreshRealizedRowHierarchyState: ForEachRealizedRow itself visits pooled rows too.
    ForEachRealizedRow([this, repeater](winrt::TableViewRow const& row)
    {
        if (repeater.GetElementIndex(row) >= 0)
        {
            ApplyRowStyleSelector(row);
        }
    });

    ForEachRealizedGroupHeader([this](winrt::TableViewGroupHeader const& header, int32_t /*index*/)
    {
        ApplyGroupHeaderStyleSelector(header);
    });
}
