// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewCellAutomationPeer.h"

// Border is sealed. A single-child Grid preserves cell chrome while allowing
// normal framework peer discovery to return the same peer as Grid.GetItem.
class TableViewCell :
    public ReferenceTracker<TableViewCell, winrt::Microsoft::UI::Xaml::Controls::GridT, winrt::composable>
{
public:
    TableViewCell(winrt::TableViewRow const& row, winrt::TableViewColumn const& column, int32_t columnIndex)
        : m_row(winrt::make_weak(row)), m_column(winrt::make_weak(column)), m_columnIndex(columnIndex)
    {
    }

    winrt::AutomationPeer OnCreateAutomationPeer()
    {
        return winrt::make<TableViewCellAutomationPeer>(
            get_strong().as<winrt::FrameworkElement>(), m_row.get(), m_column.get(), m_columnIndex);
    }

    static winrt::UIElement Child(winrt::Grid const& cell)
    {
        auto const children = cell.Children();
        return children.Size() ? children.GetAt(0) : nullptr;
    }

    static void Child(winrt::Grid const& cell, winrt::UIElement const& content)
    {
        auto const children = cell.Children();
        children.Clear();
        if (content)
        {
            children.Append(content);
        }
    }

private:
    winrt::weak_ref<winrt::TableViewRow> m_row;
    winrt::weak_ref<winrt::TableViewColumn> m_column;
    int32_t m_columnIndex;
};
