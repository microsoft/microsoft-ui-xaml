// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewBeginningEditEventArgs.g.h"

class TableViewBeginningEditEventArgs :
    public winrt::implementation::TableViewBeginningEditEventArgsT<TableViewBeginningEditEventArgs>
{
public:
    TableViewBeginningEditEventArgs(
        winrt::IInspectable const& item,
        winrt::TableViewColumn const& column)
        : m_item(item)
        , m_column(column)
    {
    }

    winrt::IInspectable Item() { return m_item; }
    winrt::TableViewColumn Column() { return m_column; }
    bool Cancel() { return m_cancel; }
    void Cancel(bool value) { m_cancel = value; }

private:
    winrt::IInspectable m_item{ nullptr };
    winrt::TableViewColumn m_column{ nullptr };
    bool m_cancel{ false };
};
