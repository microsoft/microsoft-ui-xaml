// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <optional>

#include "TableViewRowInfo.h"
#include "Source/SourceTable.h"
#include "Engine/ShapedTree.h"
#include "Engine/ShapingEngine.h"

// Read-only queries over the shaped rows (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Row queries".
//
// Every read the adapters make, in one place: the visible list (ShapedCollectionView) and the
// row-metadata reads (ShapedRowMetadata). It also answers ShapingManager::IsItemInShapedTree, the
// same lookup family. It reads the engine (and its ShapedTree) and the SourceTable; it never writes,
// opens a commit scope or raises an event. Actions (expand, collapse, reveal) are not here: they
// need the commit scope, so they stay on ShapingManager behind IShapedExpansion.
//
// Why a separate object (decided): these reads hold real logic (the "g|" key dispatch, building
// TableViewRowInfo, the group-membership walk) but no coordination, so they do not belong in the
// coordinator. Each adapter holds a pointer to exactly what it uses instead of the whole
// ShapingManager.
//
// Reads the tree as it is, which is the list the last event described (P8): inside an event the
// tree already reflects the change, so no read needs a mapping.
//
// Lifetime: owned by value by ShapingManager (m_rows), declared after m_engine and bound to it and
// to *m_sourceTable in the member-init list. The adapters hold a raw pointer, nulled by their
// Detach() in ShapingManager::Clear() before the engine and the SourceTable are cleared. After
// Clear() every query reports "not found" (RowCount 0).
//
// UI-thread affine.
class ShapedRowQueries final
{
public:
    ShapedRowQueries(ShapingEngine const& engine, SourceTable const& source) noexcept;

    ShapedRowQueries(ShapedRowQueries const&) = delete;
    ShapedRowQueries& operator=(ShapedRowQueries const&) = delete;

    // ---- Visible list (ShapedCollectionView; RowAt and IndexOfKey also ShapedRowMetadata) ---

    // m_engine.Tree().Size().
    uint32_t RowCount() const noexcept;

    // m_engine.Tree().GetAt(index); the caller maps it to the row value and key. Throws E_BOUNDS.
    ShapedNode* RowAt(uint32_t index) const;

    // Row key -> flat index (V3): TryResolveKey, then ShapedTree::IndexOf. nullopt when unknown or
    // hidden.
    std::optional<uint32_t> IndexOfKey(winrt::hstring const& key) const;

    // Row value -> flat index: a GroupedEntry (TryGetGroupedEntry) by its Identity, else
    // SourceTable::TryFindByItem.
    // Then ShapedTree::IndexOf. nullopt when unknown or hidden.
    std::optional<uint32_t> IndexOfItem(winrt::IInspectable const& value) const;

    // ---- Row metadata reads (ShapedRowMetadata) --------------------------------------------

    // TableViewRowInfo for RowAt(index), built on demand from the node (spec field table):
    // Kind, Level (ShapedDepth), IsExpandable, IsExpanded (ShapedTree::IsExpanded), ChildCount,
    // ParentIdentity, PositionInSet (IndexInParent + 1, O(1), P2), SizeOfSet. Never stored.
    // Throws E_BOUNDS.
    TableViewRowInfo RowInfo(uint32_t index) const;

    // Row key -> node (V3/D4 dispatch). A "g|" key: m_engine.GroupsByIdentity(). Otherwise a
    // record key: SourceRecord::TryParseIdentity, then SourceTable::TryFindByAddress (always on, so
    // it works without the identity index, D4). Shown or not. nullptr when unknown.
    ShapedNode* TryResolveKey(winrt::hstring const& key) const;

    // The source tree has a hierarchy (SourceTable::HasHierarchy()).
    bool IsHierarchical() const noexcept;

    // R3, v1 meaning: index is a data row; walk ShapedParent up to its bottom-level group
    // (O(depth)); position = index - IndexOf(group) (1-based), size =
    // ShapedTree::ShownDescendantCount(group). False when ungrouped, a header, or out of range.
    bool TryGetGroupMembership(uint32_t index, uint32_t& positionInGroup, uint32_t& groupSize) const;

    // ---- Item query (ShapingManager::IsItemInShapedTree) -----------------------------------

    // True when item (by object identity) is a record in the shaped tree, shown or not:
    // SourceTable::TryFindByItem, then ShapingEngine::InShapedTree. See
    // ShapingManager::IsItemInShapedTree for the full contract.
    bool IsItemInShapedTree(winrt::IInspectable const& item) const;

private:
    ShapingEngine const& m_engine;
    SourceTable const& m_source;
};
