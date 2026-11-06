// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewCellEditEndedEventArgs.g.h"

class TableViewCellEditEndedEventArgs :
    public winrt::implementation::TableViewCellEditEndedEventArgsT<TableViewCellEditEndedEventArgs>
{
public:
    TableViewCellEditEndedEventArgs(
        winrt::IInspectable const& item,
        winrt::TableViewColumn const& column,
        winrt::TableViewEditAction editAction)
        : m_item(item)
        , m_column(column)
        , m_editAction(editAction)
    {
    }

    winrt::IInspectable Item() { return m_item; }
    winrt::TableViewColumn Column() { return m_column; }
    winrt::TableViewEditAction EditAction() { return m_editAction; }

private:
    winrt::IInspectable m_item{ nullptr };
    winrt::TableViewColumn m_column{ nullptr };
    winrt::TableViewEditAction m_editAction{ winrt::TableViewEditAction::Commit };
};
