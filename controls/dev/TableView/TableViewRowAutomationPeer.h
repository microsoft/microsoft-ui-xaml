// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TableViewRow.h"
#include "TableViewRowAutomationPeer.g.h"

class TableViewRowAutomationPeer :
    public ReferenceTracker<TableViewRowAutomationPeer, winrt::implementation::TableViewRowAutomationPeerT>
{
public:
    TableViewRowAutomationPeer(winrt::TableViewRow const& owner);

    // IAutomationPeerOverrides
    hstring GetClassNameCore();
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);

    // Expose direct cell wrappers only to avoid deep, costly UIA subtree walks.
    winrt::IVector<winrt::AutomationPeer> GetChildrenCore();

    // ISelectionItemProvider — the row reports state the owning TableView holds; it never keeps
    // its own copy, because rows are recycled.
    bool IsSelected();
    winrt::IRawElementProviderSimple SelectionContainer();
    void AddToSelection();
    void RemoveFromSelection();
    void Select();

    // IExpandCollapseProvider — hierarchical rows only. A leaf reports LeafNode rather than the
    // pattern being withdrawn, matching TableViewGroupHeaderAutomationPeer.
    winrt::ExpandCollapseState ExpandCollapseState();
    void Expand();
    void Collapse();

    // IAutomationPeerOverrides3. Level is the row's 1-based tree depth; position and set size are
    // reported within the SIBLING set, not the flat row axis, so a screen reader announces
    // "2 of 3" for the second child of a node rather than its offset into the whole table.
    int32_t GetLevelCore();
    int32_t GetPositionInSetCore();
    int32_t GetSizeOfSetCore();

    // Called by the owning row when its expansion state changes, so a connected client is not
    // left reading a stale ExpandCollapseState. Mirrors
    // TableViewGroupHeaderAutomationPeer::RaiseExpandCollapseAutomationEvent.
    void RaiseExpandCollapseAutomationEvent(winrt::ExpandCollapseState oldState, winrt::ExpandCollapseState newState);

private:
    // The owning TableView, or null once the row has been recycled out of the tree.
    winrt::TableView GetOwningTableView();
    // This row's index in the owner's ItemsSource index space, or -1 when unrealized.
    int32_t GetRowIndex();
    // The owning row, or null when this peer has outlived it.
    winrt::TableViewRow GetRow() const;
    // True when this row belongs to a tree projection, i.e. Level is 1-based rather than 0.
    bool IsHierarchicalRow() const;
    // Directional expansion, passed through to the owner unresolved so it stays idempotent.
    void SetExpansion(bool expand);
    // Position and size within this row's sibling set, read from the row's hierarchy descriptor.
    // False when the row is not a realized tree row, in which case UIA gets 0 ("unknown").
    bool TryGetSiblingPosition(int32_t& positionInSet, int32_t& sizeOfSet);
};
