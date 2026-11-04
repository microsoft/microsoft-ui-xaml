// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewCellEditEndingEventArgs.g.h"

class TableViewCellEditEndingEventArgs :
    public winrt::implementation::TableViewCellEditEndingEventArgsT<TableViewCellEditEndingEventArgs>
{
public:
    TableViewCellEditEndingEventArgs(
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

    // Set to true to keep the edit open. Read synchronously once the event returns: there is no
    // deferral in this release, so a handler must decide before it returns.
    bool Cancel() { return m_cancel; }
    void Cancel(bool value) { m_cancel = value; }

private:
    winrt::IInspectable m_item{ nullptr };
    winrt::TableViewColumn m_column{ nullptr };
    winrt::TableViewEditAction m_editAction{ winrt::TableViewEditAction::Commit };
    bool m_cancel{ false };
};