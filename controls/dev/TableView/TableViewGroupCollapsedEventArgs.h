// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewGroupCollapsedEventArgs.g.h"

class TableViewGroupCollapsedEventArgs :
    public winrt::implementation::TableViewGroupCollapsedEventArgsT<TableViewGroupCollapsedEventArgs>
{
public:
    explicit TableViewGroupCollapsedEventArgs(winrt::IInspectable const& groupKey)
        : m_groupKey(groupKey)
    {
    }

    winrt::IInspectable GroupKey() { return m_groupKey; }

private:
    winrt::IInspectable m_groupKey{ nullptr };
};
