// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableView.h"
#include "TableViewAutomationPeer.g.h"

class TableViewAutomationPeer :
    public ReferenceTracker<TableViewAutomationPeer, winrt::implementation::TableViewAutomationPeerT>
{
public:
    TableViewAutomationPeer(winrt::TableView const& owner);

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();

    // IGridProvider / ITableProvider expose visible logical columns.
    // Headers remain semantic cell names even when the header strip is hidden.
    int32_t RowCount();
    int32_t ColumnCount();
    winrt::IRawElementProviderSimple GetItem(int32_t row, int32_t column);

    // ITableProvider — TableView has column headers but no row-header concept.
    winrt::RowOrColumnMajor RowOrColumnMajor();
    winrt::com_array<winrt::IRawElementProviderSimple> GetRowHeaders();
    winrt::com_array<winrt::IRawElementProviderSimple> GetColumnHeaders();

    // ISelectionProvider — row-scoped, single selection.
    winrt::com_array<winrt::IRawElementProviderSimple> GetSelection();
    bool CanSelectMultiple();
    bool IsSelectionRequired();

    // IItemContainerProvider — lets AT clients enumerate beyond realized rows.
    winrt::IRawElementProviderSimple FindItemByProperty(
        winrt::IRawElementProviderSimple const& startAfter,
        winrt::AutomationProperty const& property,
        winrt::IInspectable const& value);

private:
    com_ptr<TableView> GetImpl();
};
