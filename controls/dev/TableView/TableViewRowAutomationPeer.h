// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewRow.h"
#include "TableViewRowAutomationPeer.g.h"
#include "TableViewAutomationHelpers.h"

#include <vector>

class TableViewRowAutomationPeer :
    public ReferenceTracker<TableViewRowAutomationPeer, winrt::implementation::TableViewRowAutomationPeerT, winrt::IVirtualizedItemProvider>
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
    // "row i of n" on the same flat basis as IGridProvider.
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

    // IVirtualizedItemProvider — available only after the realized row has been recycled out.
    void Realize();

    // Single source of cell-peer identity for GetChildrenCore and Grid.GetItem; UIA compares
    // providers by identity.
    winrt::AutomationPeer GetOrCreateCellPeer(winrt::FrameworkElement const& cell);
    bool CanReuseForRowItem(winrt::TableViewRow const& row, winrt::TableView const& tableView);
    void TrackCurrentRowItem(winrt::TableViewRow const& row, winrt::TableView const& tableView);
    void DropCellPeerCache();

private:
    // The owning TableView, or null once the row has been recycled out of the tree.
    winrt::TableView GetOwningTableView();
    // This row's index in the owner's ItemsSource index space, or -1 when unrealized.
    int32_t GetRowIndex();
    bool IsVirtualized();
    void RealizeCore();
    winrt::IInspectable GetTrackedItem() const;
    int32_t GetTrackedItemIndex(winrt::TableView const& tableView);
    int32_t GetItemOccurrenceAtIndex(winrt::TableView const& tableView, winrt::IInspectable const& item, int32_t targetIndex);
    bool IsTrackedRow(winrt::TableViewRow const& row, winrt::TableView const& tableView);
    void TrackRowItem(winrt::TableViewRow const& row, winrt::TableView const& tableView);
    [[noreturn]] static void ThrowElementNotAvailable();
    // Joins the visible cells' display text in visual order.
    static std::wstring ComposeCellTexts(
        TableViewRow* rowImpl,
        winrt::Panel const& cellsHost,
        bool allowPeerCreation);

    // Dead cell entries stay cached until the next prune during child enumeration or peer lookup.
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
    winrt::weak_ref<winrt::TableView> m_lastOwningTable{ nullptr };
    TableViewTrackedItemIdentity m_item;
    int32_t m_lastKnownRowIndex{ -1 };
    int32_t m_trackedItemOccurrence{ -1 };
};
