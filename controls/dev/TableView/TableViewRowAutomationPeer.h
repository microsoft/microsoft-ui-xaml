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

    // A TableViewRow is a Control with no content of its own, so the base peer computes no name and
    // assistive technology announces a bare "data item". Compose the visible cell texts instead.
    hstring GetNameCore();

    // Rows are virtualized, so the UIA parent only ever exposes the realized window and a client
    // cannot infer "row i of n" from the children collection - the control has to supply it. Same
    // contract ListViewItemAutomationPeer satisfies. Under grouping these are group-relative,
    // matching ItemsControlAutomationPeer / NavigationView / TreeView.
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

    // Internal: hands back the peer this row has already published for a cell. Every path that
    // yields a cell provider (this peer's GetChildrenCore and TableViewAutomationPeer::GetItem)
    // must route through here - UIA compares providers by identity, so minting a fresh peer per
    // query makes grid addressing and tree navigation disagree about the same cell, and drops
    // Narrator focus whenever the tree is re-queried.
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

    // One peer per realized cell, keyed weakly so a cell dropped by a rebuild or a recycle releases
    // immediately. Rebuilt wholesale from the live cell list on every GetChildrenCore.
    struct CellPeerCacheEntry
    {
        winrt::weak_ref<winrt::FrameworkElement> cell{ nullptr };
        winrt::AutomationPeer peer{ nullptr };
    };

    std::vector<CellPeerCacheEntry> m_cellPeerCache;
};
