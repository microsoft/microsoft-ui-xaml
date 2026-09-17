// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewSortingEventArgs.g.h"

class TableViewSortingEventArgs :
    public winrt::implementation::TableViewSortingEventArgsT<TableViewSortingEventArgs>
{
public:
    TableViewSortingEventArgs(
        winrt::TableViewColumn const& column,
        winrt::SortDirection direction)
        : m_column(column)
        , m_direction(direction)
    {
    }

    winrt::TableViewColumn Column() { return m_column; }
    winrt::SortDirection Direction() { return m_direction; }
    bool Cancel() { return m_cancel; }
    void Cancel(bool value) { m_cancel = value; }

private:
    winrt::TableViewColumn m_column{ nullptr };
    winrt::SortDirection m_direction{ winrt::SortDirection::None };
    bool m_cancel{ false };
};
