// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>

#include "TableViewRowInfo.h"

class ShapedRowQueries;
struct IShapedExpansion;

// TableViewSource2's row metadata provider (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Row metadata".
//
// An adapter (decided, R1): implements the existing ITableViewRowMetadataProvider, so TableView
// consumes v2 exactly as it consumes v1 TableViewSource::GetRowMetadata(). TableViewSource2 returns
// it as a shared_ptr. It holds no shaping logic: reads forward to ShapedRowQueries, and the four
// expansion actions to IShapedExpansion (implemented by ShapingManager, which runs them inside its
// commit scope). It cannot reach the verbs, Refresh or Clear.
//
// Stateless: no CollectionChanged subscriptions and no caches (v1 RowMetadataProvider keeps three
// event tokens, an identity -> index map and a header-index list, all rebuilt on change). Every
// answer is read live from the shaped tree, which matches the last raised event (P8).
//
// Key space: the row key (V3), ShapedRowQueries::KeyAt, the same string as
// ShapedCollectionView::KeyFromIndex. Records:
// SourceRecord::Identity() ("0x" + address); groups: GroupNode::Identity ("g|" ...). v1's "group:"
// and "node:" prefixes are not used.
//
// Lifetime (decided, R4: the D2 pattern): ShapingManager creates it on the first
// TableViewSource2::GetRowMetadata() and caches it; ShapingManager::Clear() calls Detach() right
// after ShapedCollectionView::Detach(). TableView may hold it longer; once detached, GetRowInfo
// returns a default TableViewRowInfo, GetIdentity an empty string, and every other member false or
// a no-op.
//
// UI-thread affine.
class ShapedRowMetadata final : public ITableViewRowMetadataProvider
{
public:
    ShapedRowMetadata(ShapedRowQueries const& rows, IShapedExpansion& expansion) noexcept;

    // ---- Per row ----------------------------------------------------------------------------

    // ShapedRowQueries::RowInfo(index): built on demand from RowAt(index), never stored (field
    // table in the spec). Out of range or detached: a default TableViewRowInfo.
    TableViewRowInfo GetRowInfo(int32_t index) override;

    // ShapedRowQueries::KeyAt(index), the row key (V3) shared with ShapedCollectionView. Empty
    // when out of range or detached.
    winrt::hstring GetIdentity(int32_t index) override;

    // ShapedRowQueries::IndexOfKey (V3/D4). False when unknown, hidden or detached.
    bool TryGetIndexForIdentity(winrt::hstring const& identity, int32_t& index) override;

    // ---- Expansion (R2) ---------------------------------------------------------------------
    // Each resolves the key with ShapedRowQueries::TryResolveKey and calls
    // IShapedExpansion::SetExpanded with true / false / nullopt (toggle). Returns whether the state
    // changed; false for an unknown key, a node that is not expandable, or detached. Throws while
    // an apply runs (#19), like a verb.

    bool Expand(winrt::hstring const& key) override;
    bool Collapse(winrt::hstring const& key) override;
    bool Toggle(winrt::hstring const& key) override;

    // IShapedExpansion::SetAllExpanded(ExpansionAxis::Groups | Rows, expanded).
    void ExpandAllGroups() override;
    void CollapseAllGroups() override;
    void ExpandAllRows() override;
    void CollapseAllRows() override;

    // IShapedExpansion::ExpandSubtree. No-op for a group key, an unknown key, or a source without a
    // hierarchy (the interface's contract).
    void ExpandSubtree(winrt::hstring const& key) override;

    // TryResolveKey, then IShapedExpansion::Reveal (#20). The node's visible index, or -1 for an
    // unknown key, a node not in the shaped tree, or detached. Throws while an apply runs (#19).
    int32_t Reveal(winrt::hstring const& key) override;

    // ---- Shape ------------------------------------------------------------------------------

    // ShapedRowQueries::IsHierarchical(): the source tree has a hierarchy (ParentBy / ChildrenBy).
    // TableView uses it to reserve chevron width on leaf roots.
    bool IsHierarchicalSource() const override;

    // v1 meaning kept (decided, R3): 1-based position among the data rows between two adjacent
    // header bands, and that band's size. Computed, not scanned:
    // ShapedRowQueries::TryGetGroupMembership.
    // False when rows is not this source's ShapedCollectionView (compared by identity), the source
    // is not grouped, index is a header or out of range, or detached.
    bool TryGetGroupMembership(
        winrt::Windows::Foundation::IInspectable const& rows,
        int32_t index,
        int32_t& positionInGroup,
        int32_t& groupSize) override;

    // ---- Lifetime ---------------------------------------------------------------------------

    // Nulls m_rows and m_expansion. Called by ShapingManager::Clear() right after
    // ShapedCollectionView::Detach(). Idempotent.
    void Detach() noexcept;

private:
    // Non-owning (R4). Both null after Detach(); both point into the same ShapingManager.
    ShapedRowQueries const* m_rows{ nullptr };
    IShapedExpansion* m_expansion{ nullptr };
};
