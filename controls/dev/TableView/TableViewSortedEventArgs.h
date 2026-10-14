// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewSortedEventArgs.g.h"

class TableViewSortedEventArgs :
    public winrt::implementation::TableViewSortedEventArgsT<TableViewSortedEventArgs>
{
public:
    TableViewSortedEventArgs(
        winrt::TableViewColumn const& column,
        winrt::SortDirection direction)
        : m_column(column)
        , m_direction(direction)
    {
    }

    winrt::TableViewColumn Column() { return m_column; }
    winrt::SortDirection Direction() { return m_direction; }

private:
    winrt::TableViewColumn m_column{ nullptr };
    winrt::SortDirection m_direction{ winrt::SortDirection::None };
};
