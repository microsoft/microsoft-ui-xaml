// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewRow.h"
#include "TableViewRowAutomationPeer.g.h"

#include <vector>

class TableViewRowAutomationPeer :
    public ReferenceTracker<TableViewRowAutomationPeer, winrt::implementation::TableViewRowAutomationPeerT>
{
public:
    TableViewRowAutomationPeer(winrt::TableViewRow const& owner);

    // IAutomationPeerOverrides
    hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);

    // A TableViewRow is a Control with no content of its own, so the base peer computes no name
    // and AT announces a bare "data item". Compose the visible cell texts instead.
    hstring GetNameCore();

    // Rows are virtualized: UIA only ever sees the realized window, so the control has to supply
    // "row i of n". Group-relative when the source is grouped.
    int32_t GetPositionInSetCore();
    int32_t GetSizeOfSetCore();

    // Expose direct cell wrappers only to avoid deep, costly UIA subtree walks.
    winrt::IVector<winrt::AutomationPeer> GetChildrenCore();

    // ISelectionItemProvider — the row reports state the owning TableView holds; it never keeps
    // its own copy, because rows are recycled.
    bool IsSelected();
    winrt::IRawElementProviderSimple SelectionContainer();
    void AddToSelection();
    void RemoveFromSelection();
    void Select();

    // Single source of cell-peer identity: GetChildrenCore and TableViewAutomationPeer::GetItem
    // both route through here. UIA compares providers by identity, so a fresh peer per query makes
    // grid addressing and tree navigation disagree and drops Narrator focus on every re-query.
    winrt::AutomationPeer GetOrCreateCellPeer(
        winrt::FrameworkElement const& cell,
        winrt::TableViewColumn const& column,
        int32_t visibleColumnIndex);

private:
    // The owning TableView, or null once the row has been recycled out of the tree.
    winrt::TableView GetOwningTableView();
    // This row's index in the owner's ItemsSource index space, or -1 when unrealized.
    int32_t GetRowIndex();
    // 1-based position within the owning group and that group's item count; false when ungrouped.
    bool TryGetGroupPosition(int32_t rowIndex, int32_t& positionInGroup, int32_t& sizeOfGroup);

    // One peer per realized cell, keyed weakly so a recycled or rebuilt cell releases immediately.
    // tracker_ref is the convention for a strong WinRT ref owned by a ReferenceTracker type.
    struct CellPeerCacheEntry
    {
        CellPeerCacheEntry(
            ITrackerHandleManager const* owner,
            winrt::FrameworkElement const& cellElement,
            winrt::AutomationPeer const& cellPeer)
            : cell(winrt::make_weak(cellElement))
            , peer(owner, cellPeer)
        {
        }

        winrt::weak_ref<winrt::FrameworkElement> cell{ nullptr };
        tracker_ref<winrt::AutomationPeer> peer;
    };

    std::vector<CellPeerCacheEntry> m_cellPeerCache;
};
