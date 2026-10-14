// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewColumnHeaderAutomationPeer.h"

// Keep the existing Grid layout/input behavior, but attach the header peer to the
// actual focus and hit-test target. This implementation-only type needs no WinRT API.
class TableViewHeaderCell :
    public ReferenceTracker<TableViewHeaderCell, winrt::Microsoft::UI::Xaml::Controls::GridT, winrt::composable>
{
public:
    TableViewHeaderCell(winrt::TableView const& table, winrt::TableViewColumn const& column)
        : m_table(winrt::make_weak(table)), m_column(winrt::make_weak(column))
    {
    }

    winrt::hstring GetRuntimeClassName() const
    {
        return winrt::hstring_name_of<winrt::Grid>();
    }

    winrt::AutomationPeer OnCreateAutomationPeer()
    {
        return winrt::make<TableViewColumnHeaderAutomationPeer>(
            get_strong().as<winrt::FrameworkElement>(), m_table.get(), m_column.get());
    }

private:
    winrt::weak_ref<winrt::TableView> m_table;
    winrt::weak_ref<winrt::TableViewColumn> m_column;
};
