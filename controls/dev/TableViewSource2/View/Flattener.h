// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Engine/ShapedEdit.h"
#include "Engine/ShapedTree.h"
#include "View/FlatDelta.h"
#include "View/GroupExpansion.h"

// Shaped tree -> visible list (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Change pipeline".
//
// Keeps the visible list (m_rows) and every shown node's VisibleDescendantCount in step with the shaped
// tree, and turns each unit of work into one FlatDelta:
//   - Bulk:      OnRebuilt -> Rebuild -> IsReset.
//   - Surgical:  BeginCommit -> OnRemoving / OnInserted per ShapedEdit -> EndCommit.
//   - Expansion: Expand / Collapse (one node) or ExpandAll* / CollapseAll* (Reset). Never involves
//                the engine: shaped children are built eagerly, so no app code runs.
//
// m_rows holds the published state (decided, V4). A surgical unit of work records its splices,
// each with the rows it inserts, and leaves m_rows alone; ShapingManager::Publish applies them one
// at a time (ApplySplice), raising each event right after, so Size / GetAt match every event.
// Rebuild writes m_rows directly: a Reset is one event, so nothing sees a partial state.
//
// Expansion state: records use ShapeFlags::Expanded on their ShapeState; groups use the group
// expansion model, keyed by GroupNode::Identity (owned by ShapingManager, written here).
//
// Flat index (decided: linear sum): a node's index is its parent's index + 1 (0 for children of
// the shaped root, which is not shown) plus 1 + VisibleDescendantCount for each earlier sibling,
// repeated up the ancestors. O(depth x siblings). A Fenwick tree or a cached index can replace it
// later without changing the sink or the delta.
//
// Owns no app object. UI-thread affine.
class Flattener final : public IShapedEditSink
{
public:
    explicit Flattener(GroupExpansion& groupExpansion);

    Flattener(Flattener const&) = delete;
    Flattener& operator=(Flattener const&) = delete;

    // ---- Visible list (read) ----------------------------------------------------------------

    uint32_t Size() const noexcept;

    // The node shown at index (group header or data row). O(1). Reads m_rows, the published
    // state: during a commit, and between the events of a delta being published, it lags the tree.
    ShapedRef GetAt(uint32_t index) const;

    // Index of node in the visible list, or nullopt when an ancestor is collapsed. Public for
    // identity -> index (TryGetIndexForIdentity, IKeyIndexMapping::IndexFromKey), restore after a
    // Reset, and UIA / keyboard parent navigation. The visible list maps nullopt to -1 for WinRT.
    // Derived from the tree's counts, so it answers for the final state; the visible list checks
    // m_rows when it is called mid-publish (V4).
    std::optional<uint32_t> IndexOf(ShapedRef node) const;

    // Number of active GroupBy levels (0 when ungrouped). Feeds ShapedDepth().
    uint16_t GroupLevelCount() const noexcept;

    // ---- Publishing (V4) --------------------------------------------------------------------

    // Applies one splice of the delta being published to m_rows: erases RemovedCount rows at
    // Index, then inserts Inserted. ShapingManager::Publish calls it once per splice, in order,
    // each followed by that splice's event. Never dereferences a removed ref. Asserts that no
    // commit is open.
    void ApplySplice(FlatSplice const& splice);

    // ---- Bulk -------------------------------------------------------------------------------

    // Depth-first pass from root's children over shown nodes only: fills m_rows and their
    // VisibleDescendantCount, O(visible). Hidden counts are left undefined (F2).
    void Rebuild(GroupNode& root, uint16_t groupLevelCount);

    // ---- Surgical (IShapedEditSink) ---------------------------------------------------------

    // Starts a unit of work: clears m_pending and m_dirty.
    void BeginCommit();

    // 1. span = 1 + VisibleDescendantCount(node).
    // 2. If shown: append {i, span, {}} at i = FlatIndexOf(node). m_rows is not touched (V4).
    // 3. Subtract span up the ancestors, stopping at the first collapsed one.
    // 4. Mark the parent (ItemCount up the group chain for a top-level record or a group, never
    //    the shaped root; Expandable if its shaped child count is now 0).
    // 5. Drop the node and its subtree from m_dirty.
    // 6. A GroupNode: GroupExpansion::Erase for it and every group in its subtree (F4).
    void OnRemoving(ShapedEdit const& e) override;

    // Mirror of OnRemoving: if shown, compute the node's count through its expanded descendants
    // (F2), collect its shown rows in display order and append {i, 0, rows}; if hidden, no count
    // work; add span up the ancestors, mark the parent (ItemCount; Expandable if its count is now 1).
    void OnInserted(ShapedEdit const& e) override;

    // Rebuild, clear m_dirty (its nodes may be destroyed after this call), and the pending delta
    // becomes IsReset. A commit either rebuilds or edits, never both.
    void OnRebuilt(GroupNode& root, uint16_t groupLevelCount) override;

    // Resolves m_dirty: one RowStateEdit {FlatIndexOf(node), fields} per dirty node still shown.
    // Returns m_pending.
    FlatDelta EndCommit();

    // Closes a unit of work whose engine compute phase threw: clears m_pending, m_dirty and
    // m_inCommit. Called by ShapingManager's commit scope. Nothing to undo: the engine calls the
    // sink only from its write phase, which cannot throw, so no splice was recorded.
    void AbortCommit() noexcept;

    // ---- Expansion --------------------------------------------------------------------------
    //
    // Each call is its own unit of work: it brackets itself with BeginCommit / EndCommit and returns
    // the delta, which ShapingManager::Publish sends out exactly like an engine commit. Asserts if
    // called while a commit is open, so expansion never merges into an engine delta. The manager
    // runs the call and its Publish inside the #19 apply scope (source frozen).

    // Sets the flag (ShapeFlags::Expanded, or the group model). If shown: recomputes the node's
    // count recursively through its expanded descendants (O(revealed rows), F2), records a splice
    // of the revealed rows at FlatIndexOf(node) + 1, propagates up, marks the node Expanded. No-op if already
    // expanded or not expandable. Hidden node (an ancestor collapsed): flag only, empty delta; the
    // rows appear when the ancestor expands. Revealing a node is a separate verb.
    FlatDelta Expand(ShapedRef node);

    // Reverse of Expand: the count drops to 0 and a removal splice is recorded.
    // Expand / Collapse of a context row (F5) write only ShapeFlags::OverlayCollapsed.
    FlatDelta Collapse(ShapedRef node);

    // One verb per axis: opening every group and expanding every tree node are different user
    // actions. All act on m_root, then Rebuild; the delta is IsReset.
    // Groups: move the group model's baseline (GroupExpansion::SetAll). O(1) + Rebuild.
    FlatDelta ExpandAllGroups();
    FlatDelta CollapseAllGroups();
    // Records: set or clear ShapeFlags::Expanded on every record, and clear or set
    // OverlayCollapsed on every context row (F5). O(n) + Rebuild.
    FlatDelta ExpandAllRows();
    FlatDelta CollapseAllRows();

    // ---- Lifetime ---------------------------------------------------------------------------

    // Teardown: empties m_rows, m_dirty and m_pending and nulls m_root. ShapingManager::Clear()
    // calls it before SourceTable::Clear() destroys the records those entries point to. Afterwards
    // Size() is 0. Idempotent.
    void Clear();

private:
    // Every ancestor of node is expanded. O(depth). Private: outside callers use IndexOf.
    bool IsShown(ShapedRef node) const;

    // Effective expansion. Group: GroupExpansion. Record: context row (fails the filter, in the
    // shaped tree) ? !OverlayCollapsed : Expanded (F5).
    bool IsExpanded(ShapedRef node) const;
    // Writes the bit IsExpanded reads: OverlayCollapsed for a context row, else Expanded or the
    // group model.
    void SetExpanded(ShapedRef node, bool expanded);

    // The node's own VisibleDescendantCount (GroupNode or the record's ShapeState).
    uint32_t& VisibleDescendantCount(ShapedRef node) const;

    // Adds delta to each ancestor's count, stopping at the first collapsed ancestor.
    void PropagateCount(ShapedRef parent, int64_t delta);

    // Precondition: IsShown(node).
    uint32_t FlatIndexOf(ShapedRef node) const;

    void MarkDirty(ShapedRef node, RowStateField fields);

    // Visible list: one ShapedRef per visible row (decided: no wrapper struct). A group header's
    // ref IsGroup(); a data row's ref is a SourceRecord. Only visible rows have entries: a
    // collapsed node's descendants have none, and the shaped root is never one. GetAt is O(1).
    // Not stored per row, because each is O(1) to derive or needed only rarely:
    //   - shaped parent: ShapedParent(), O(1);
    //   - set size: ShapedParent().ShapedChildCount();
    //   - position in set: searched among the shaped parent's children on demand (O(siblings),
    //     UIA only). Storing it would make every insert or remove rewrite all later siblings;
    //   - owning group: header -> GroupNode::Parent; record -> top-level ancestor -> Shape.Group;
    //   - shaped depth, TableViewRowInfo: built on demand by the row metadata provider.
    // The published state (V4): written only by Rebuild, ApplySplice and Clear.
    std::vector<ShapedRef> m_rows;

    // The delta being built by the current unit of work.
    FlatDelta m_pending;

    // Nodes whose row state changed during the current commit. Nodes, not indexes: later splices
    // shift indexes, so they are resolved in EndCommit. Keyed by node so repeated marks merge into
    // one edit, and a node removed mid-commit is erased before it is destroyed.
    std::unordered_map<ShapedRef, RowStateField, ShapedRefHash> m_dirty;

    // Group expansion, keyed by GroupNode::Identity. Owned by ShapingManager.
    GroupExpansion& m_groupExpansion;

    // The engine's shaped root: owned by the engine for its whole lifetime, never replaced (a
    // rebuild swaps its children). Bound on the first OnRebuilt; later calls assert the same root.
    // Null before the first rebuild and after Clear().
    GroupNode* m_root{ nullptr };
    uint16_t m_groupLevelCount{ 0 };

    // Between BeginCommit and EndCommit. Expansion calls assert it is false.
    bool m_inCommit{ false };
};
