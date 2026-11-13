// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewColumn.h"
#include "TableViewRow.h"

namespace
{
    bool IsColumnInColumns(winrt::TableView const& owner, winrt::TableViewColumn const& column)
    {
        if (!owner || !column)
        {
            return false;
        }

        if (auto columns = owner.Columns())
        {
            for (auto const& candidate : columns)
            {
                if (candidate == column)
                {
                    return true;
                }
            }
        }

        return false;
    }
}

// COM identity, not ABI-pointer equality: IInspectable's operator== compares raw abi pointers, so
// the same object reached through a different interface - boxed values, projected interfaces, a
// future wrapping layer - would not match. A member rather than a file-local helper so every
// comparison of data items in the control uses the same one.
bool TableView::SameInspectableIdentity(winrt::IInspectable const& lhs, winrt::IInspectable const& rhs)
{
    if (lhs == rhs)
    {
        return true;
    }
    if (!lhs || !rhs)
    {
        return false;
    }

    try
    {
        return lhs.as<winrt::Windows::Foundation::IUnknown>() ==
            rhs.as<winrt::Windows::Foundation::IUnknown>();
    }
    catch (...)
    {
        return false;
    }
}

// Single choke point for turning a row's container-level item into the data item. Identity today;
// it exists as a seam for a layer that wraps items (grouping).
winrt::IInspectable TableView::UnwrapDataItem(winrt::IInspectable const& item) const
{
    return item;
}

void TableView::SetCurrentItem(winrt::IInspectable const& item)
{
    m_currentItem.set(item);
}

void TableView::UpdateCurrentColumn(winrt::TableViewColumn const& column)
{
    if (IsColumnInColumns(*this, column))
    {
        m_currentColumn.set(column);
    }
    else
    {
        m_currentColumn.set(nullptr);
    }
}

void TableView::SetCurrentCell(winrt::IInspectable const& item, winrt::TableViewColumn const& column)
{
    SetCurrentItem(item);
    UpdateCurrentColumn(column);
}

bool TableView::TryGetItemAtRowIndex(int32_t rowIndex, winrt::IInspectable& item) const
{
    item = nullptr;
    if (rowIndex < 0)
    {
        return false;
    }

    if (auto repeater = m_rowsRepeater.get())
    {
        if (auto row = repeater.TryGetElement(rowIndex).try_as<winrt::TableViewRow>())
        {
            if (auto dataItem = UnwrapDataItem(row.DataContext()))
            {
                item = dataItem;
                return true;
            }
        }

        if (auto view = repeater.ItemsSourceView())
        {
            if (rowIndex < view.Count())
            {
                if (auto dataItem = UnwrapDataItem(view.GetAt(rowIndex)))
                {
                    item = dataItem;
                    return true;
                }
            }
        }
    }

    return false;
}

winrt::TableViewRow TableView::FindRealizedRowForItem(winrt::IInspectable const& item)
{
    if (!item)
    {
        return nullptr;
    }

    if (auto repeater = m_rowsRepeater.get())
    {
        const auto childrenCount = winrt::VisualTreeHelper::GetChildrenCount(repeater);
        for (int32_t childIndex = 0; childIndex < childrenCount; ++childIndex)
        {
            if (auto row = winrt::VisualTreeHelper::GetChild(repeater, childIndex).try_as<winrt::TableViewRow>())
            {
                // ItemsRepeater keeps recycled containers parented in its pool, and a pooled row
                // still carries the DataContext of the item it last showed. Without this filter a
                // scrolled-away ghost can win the identity match below over the genuinely realized
                // row. GetElementIndex is >= 0 only for containers the repeater currently
                // considers realized.
                if (repeater.GetElementIndex(row) < 0)
                {
                    continue;
                }

                if (auto dataItem = UnwrapDataItem(row.DataContext());
                    dataItem && SameInspectableIdentity(dataItem, item))
                {
                    return row;
                }
            }
        }
    }

    return nullptr;
}
