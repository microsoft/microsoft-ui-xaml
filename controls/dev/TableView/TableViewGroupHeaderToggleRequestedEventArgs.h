// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TableViewGroupHeaderToggleRequestedEventArgs.g.h"

class TableViewGroupHeaderToggleRequestedEventArgs :
    public winrt::implementation::TableViewGroupHeaderToggleRequestedEventArgsT<TableViewGroupHeaderToggleRequestedEventArgs>
{
public:
    explicit TableViewGroupHeaderToggleRequestedEventArgs(winrt::IInspectable const& groupKey)
        : m_groupKey(groupKey)
    {
    }

    winrt::IInspectable GroupKey() { return m_groupKey; }

private:
    winrt::IInspectable m_groupKey{ nullptr };
};
