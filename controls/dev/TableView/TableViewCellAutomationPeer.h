// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableView.h"
#include "TableViewCellAutomationPeer.g.h"
#include <optional>

// UIA peer for a realized TableView cell; supplies the cell name and grid/table item coordinates.
// Weak row/column refs avoid extending recycled rows or removed columns.
class TableViewCellAutomationPeer :
    public ReferenceTracker<TableViewCellAutomationPeer, winrt::implementation::TableViewCellAutomationPeerT, winrt::IVirtualizedItemProvider>
{
public:
    TableViewCellAutomationPeer(
        winrt::FrameworkElement const& cell,
        winrt::TableViewRow const& row,
        winrt::TableViewColumn const& column,
        int32_t columnIndex);

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    hstring GetClassNameCore();
    hstring GetNameCore();
    hstring GetHelpTextCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
    hstring GetLocalizedControlTypeCore();

    // Body navigation is two-level, so a cell is a tab stop only while the cursor is drilled into
    // its row (TableViewRow::SetCellLevelInternal). The base peer derives both of these from
    // IsTabStop, which would make a cell report itself unfocusable - and make SetFocus() throw
    // "Target element cannot receive focus" - whenever the cursor happened to be at row level.
    // A cell is ALWAYS a valid focus target for automation; the gate is a Tab-order policy, not a
    // focusability one, so these two restate that and SetFocusCore drills the row in first.
    bool IsKeyboardFocusableCore();
    void SetFocusCore();

    // IGridItemProvider — per-cell coordinates in the owning TableView.
    int32_t Row();
    int32_t Column();
    int32_t RowSpan();
    int32_t ColumnSpan();
    winrt::IRawElementProviderSimple ContainingGrid();

    // ITableItemProvider — returns this cell's column header; rows have no headers.
    winrt::com_array<winrt::IRawElementProviderSimple> GetRowHeaderItems();
    winrt::com_array<winrt::IRawElementProviderSimple> GetColumnHeaderItems();

    // IValueProvider — lets assistive technology read the cell text and set it without a pointer.
    // SetValue drives the same public edit lifecycle a user does (BeginEdit / write / CommitEdit),
    // so BeginningEdit / CellEditEnding handlers and validation all still run.
    winrt::hstring Value();
    bool IsReadOnly();
    void SetValue(winrt::hstring const& value);

    // IVirtualizedItemProvider — available only after the owning row has been recycled out.
    void Realize();

    winrt::hstring ReadNameForEdit();
    void BeginEditName();
    void EndEditName();
    void ResetEditName();
    void UpdateNameItem(winrt::IInspectable const& item);

private:
    // True when this cell is editable AND its column produces a TextBox editor, which is the only
    // editor SetValue can drive today.
    bool SupportsValuePattern();

    // Resolves the row index from the owning ItemsRepeater.
    int32_t GetRowIndex();
    bool IsVirtualized();
    void RealizeCore();
    winrt::UIElement GetRealizedCellFromRow(winrt::TableViewRow const& row);
    winrt::TableView GetTrackedTableForRow(winrt::TableViewRow const& row);
    winrt::IInspectable GetTrackedItem() const;
    int32_t GetTrackedItemIndex(winrt::TableView const& tableView);
    bool IsTrackedRow(winrt::TableViewRow const& row, winrt::TableView const& tableView);
    void TrackRowItem(winrt::TableViewRow const& row, winrt::TableView const& tableView);
    [[noreturn]] static void ThrowElementNotAvailable();

    // Resolves the column's stringified Header.
    winrt::hstring GetColumnHeaderText();

    // Resolves displayed text from the TextBlock or content peer name.
    winrt::hstring GetCellValueText();
    winrt::hstring ReadDisplayName(winrt::FrameworkElement const& display = nullptr);
    void QueueFinalName(uint64_t generation);

    winrt::weak_ref<winrt::TableViewRow> m_row{ nullptr };
    winrt::weak_ref<winrt::TableViewColumn> m_column{ nullptr };
    winrt::weak_ref<winrt::TableView> m_lastOwningTable{ nullptr };
    winrt::weak_ref<winrt::IInspectable> m_item{ nullptr };
    // Construction-time fallback only; Column() recomputes from the live cell host.
    int32_t m_columnIndex{ -1 };
    int32_t m_lastKnownRowIndex{ -1 };
    tracker_ref<winrt::IInspectable> m_nameItem{ this };
    std::optional<winrt::hstring> m_lastName;
    // Holds the pre-edit Name only; read paths must not write it or live cell names freeze.
    std::optional<winrt::hstring> m_editName;
    uint64_t m_nameGeneration{ 0 };
    winrt::FrameworkElement::LayoutUpdated_revoker m_nameLayoutUpdatedRevoker{};
};
