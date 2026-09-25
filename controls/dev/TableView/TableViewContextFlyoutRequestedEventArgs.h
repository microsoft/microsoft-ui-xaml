// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewContextFlyoutRequestedEventArgs.g.h"

class TableViewContextFlyoutRequestedEventArgs :
    public winrt::implementation::TableViewContextFlyoutRequestedEventArgsT<TableViewContextFlyoutRequestedEventArgs>
{
public:
    TableViewContextFlyoutRequestedEventArgs(
        winrt::IInspectable const& item,
        winrt::TableViewColumn const& column,
        bool isHeader,
        winrt::FlyoutBase const& flyout)
        : m_item(item)
        , m_column(column)
        , m_isHeader(isHeader)
        , m_flyout(flyout)
    {
    }

    winrt::IInspectable Item() { return m_item; }
    winrt::TableViewColumn Column() { return m_column; }
    bool IsHeader() { return m_isHeader; }
    winrt::FlyoutBase ContextFlyout() { return m_flyout; }
    void ContextFlyout(winrt::FlyoutBase const& value) { m_flyout = value; }
    bool Handled() { return m_handled; }
    void Handled(bool value) { m_handled = value; }

private:
    winrt::IInspectable m_item{ nullptr };
    winrt::TableViewColumn m_column{ nullptr };
    bool m_isHeader{ false };
    winrt::FlyoutBase m_flyout{ nullptr };
    bool m_handled{ false };
};
