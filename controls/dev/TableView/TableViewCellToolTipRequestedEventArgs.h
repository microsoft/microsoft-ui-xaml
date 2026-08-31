// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewCellToolTipRequestedEventArgs.g.h"

class TableViewCellToolTipRequestedEventArgs :
    public winrt::implementation::TableViewCellToolTipRequestedEventArgsT<TableViewCellToolTipRequestedEventArgs>
{
public:
    TableViewCellToolTipRequestedEventArgs(
        winrt::IInspectable const& item,
        winrt::TableViewColumn const& column)
        : m_column(column)
        , m_item(item)
    {
    }

    winrt::IInspectable Item() { return m_item; }
    winrt::TableViewColumn Column() { return m_column; }
    winrt::IInspectable Content() { return m_content; }
    void Content(winrt::IInspectable const& value) { m_content = value; }
    winrt::hstring AutomationHelpText() { return m_automationHelpText; }
    void AutomationHelpText(winrt::hstring const& value) { m_automationHelpText = value; }

private:
    winrt::TableViewColumn m_column{ nullptr };
    winrt::IInspectable m_item{ nullptr };
    winrt::IInspectable m_content{ nullptr };
    winrt::hstring m_automationHelpText{};
};
