// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewColumnReorderedEventArgs.g.h"

class TableViewColumnReorderedEventArgs :
    public winrt::implementation::TableViewColumnReorderedEventArgsT<TableViewColumnReorderedEventArgs>
{
public:
    TableViewColumnReorderedEventArgs(winrt::TableViewColumn const& column, int32_t oldIndex, int32_t newIndex)
        : m_column(column)
        , m_oldIndex(oldIndex)
        , m_newIndex(newIndex)
    {
    }

    winrt::TableViewColumn Column() { return m_column; }
    int32_t OldIndex() { return m_oldIndex; }
    int32_t NewIndex() { return m_newIndex; }

private:
    winrt::TableViewColumn m_column{ nullptr };
    int32_t m_oldIndex{ -1 };
    int32_t m_newIndex{ -1 };
};
