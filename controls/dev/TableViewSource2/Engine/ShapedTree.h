// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <winrt/Windows.Foundation.h>

#include "Engine/FenwickTree.h"
#include "Engine/GroupExpansion.h"
#include "Source/SourceRecord.h"

// Shaped tree for TableViewSource2 (declarations only): the node types and the ShapedTree class
// that owns the shaped root and serves the visible list from it. See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Shaped tree
// representation" and "Reading rows by position".
//
// The shaped tree does not copy the source tree. Only group headers are new nodes (GroupNode); rows
// are references (ShapedRef) to existing SourceRecords.
//
//   shaped root (GroupNode, never displayed)
//     ├─ GroupNode  (GroupLevel 0)          only while GroupBy is active
//     │    ├─ SourceRecord (top-level)      filtered and sorted; its source subtree follows it
//     │    └─ ...
//     └─ ...
//   Without grouping the shaped root's Children are the filtered and sorted top-level records.
//
// Rules:
//   - The shaped root is always a GroupNode; there is no grouped/ungrouped mode. It is never in the
//     visible list (ShapedTree starts at its Children).
//   - Groups bucket SourceTable::TopLevel() only. A record's source descendants are reached through
//     SourceRecord::Children (or the filtered/sorted children in the record's engine ShapeState),
//     never through a group.
//   - Applying or removing GroupBy rebuilds GroupNodes (O(n) bucketing); the source tree is
//     untouched. Engine state for records lives in each record's ShapeState slot.
//   - ShapedRef and GroupNode stay native: the engine, ShapedTree, the WinRT visible list and the
//     row metadata provider. TableView sees items and the TableViewRowInfo / TableViewGroupInfo
//     derived from them.
//   - UI-thread affine, like SourceTable. Each GroupNode is heap-allocated and owned by its parent
//     group (GroupNode::Subgroups; top-level groups by the shaped root), so GroupNode* and
//     ShapedRef stay valid until the engine drops the group, and dropping a group frees its
//     subtree.

struct GroupNode;

// One slot in a shaped child list: a GroupNode or a SourceRecord, distinguished by the low pointer
// bit (1 = group; both pointees are at least 2-byte aligned). Non-owning; one pointer wide.
// IsGroup() is the only group test: a source parent (ShapedChildCount() > 0) is expandable but is
// not a group.
struct ShapedRef
{
    ShapedRef() noexcept = default;

    static ShapedRef FromRecord(SourceRecord* record) noexcept;
    static ShapedRef FromGroup(GroupNode* group) noexcept;

    bool IsNull() const noexcept;
    bool IsGroup() const noexcept;

    // Null if this slot holds the other kind.
    GroupNode* Group() const noexcept;
    SourceRecord* Record() const noexcept;

    // Parent in the shaped tree, O(1). Null only for the shaped root (and a null ref):
    //   group: GroupNode::Parent
    //   nested record: Record()->Parent (source parent)
    //   top-level record: Record()->Shape.Group (bottom-level group, or the shaped root when
    //   ungrouped)
    // The one upward walk used by ShapedTree (IndexOf, IsShown, count propagation), set size,
    // parent identity and "go to parent". There is no stored visible list (P1).
    ShapedRef ShapedParent() const noexcept;

    // Children this node has in the shaped tree, O(1). The single rule for expandability
    // (IsExpandable = ShapedChildCount() > 0), ChildCount on a record row, SizeOfSet (called on
    // ShapedParent()) and the expander chrome the owner re-reads (P9):
    //   group or shaped root: Group()->Children.size()
    //   record, ChildrenShaped set: Record()->Shape.ShapedChildren.size()
    //   record otherwise: Record()->Children.size()
    // Not Record()->HasChildren(): under a filter that reads the source tree and stays true when
    // every child is filtered out. Not stored; derived on demand.
    uint32_t ShapedChildCount() const noexcept;

    // Same node (same tagged pointer).
    bool operator==(ShapedRef const& other) const noexcept;
    bool operator!=(ShapedRef const& other) const noexcept;

private:
    friend struct ShapedRefHash;

    static constexpr uintptr_t c_groupTag = 1;

    uintptr_t m_bits{ 0 };
};

// A group header in the shaped tree, or the shaped root. Group headers are engine-owned and exist
// only while GroupBy is active (all are dropped when it is cleared); the shaped root is
// ShapedTree's and lives as long as the engine. Surfaced to consumers as TableViewGroupInfo (Key, ItemCount, Level = GroupLevel).
struct GroupNode
{
    // The GroupBy selector's result for this bucket; empty on the shaped root. Tracked through the
    // owning TableViewSource2 (an object key may reference the VM; see SourceTable's Reference
    // tracking). Optional so the engine can drop it while the owner is alive.
    std::optional<tracker_ref<winrt::Windows::Foundation::IInspectable>> Key;

    // Stable identity: the group key path from the top level down, one length-prefixed segment
    // per level (the parent's Identity plus this Key's ValueKey::ToObjectLookupKey form:
    // "value:<typed value>", "object:<IUnknown address>" or "null:"). Survives a regroup, unlike
    // GroupNode*, so the expansion model keys group expansion on it. Empty on the shaped root.
    // Starts with the reserved "g|" marker (decided, V3), written once for a top-level group and
    // inherited by nested ones, so it doubles as the visible list's header key without colliding
    // with record identities.
    // Not the header text (that is StringifyGroupKey, culture-formatted and possibly colliding).
    winrt::hstring Identity;

    // Containing group; nullptr only on the shaped root.
    GroupNode* Parent{ nullptr };

    // Level among group headers: top-level groups = 0. Also the group's ShapedDepth(). Unused on
    // the shaped root.
    uint16_t GroupLevel{ 0 };

    // Subgroups (all IsGroup()), or, for a bottom-level group, top-level records after filter and
    // sort. Never mixed. Non-owning and in display order: the subgroups are owned by Subgroups. On the shaped root: top-level groups, or top-level records when ungrouped.
    // Bottom-level is not stored or tested here: the engine knows it from its group-level count
    // (GroupLevel + 1 == count; the shaped root when count == 0). Children cannot tell an empty
    // group's kind.
    std::vector<ShapedRef> Children;

    // Owns this group's subgroups (decided: parent owns subgroups). Same set as the group entries
    // of Children, in no particular order; empty for a bottom-level group. Destroying a group
    // destroys its subtree. A surgical prune unlinks the group (ShapedTree::Unlink, which raises its
    // event) before moving it from here to the engine's retired list, destroyed when the commit
    // ends (P8). A rebuild builds the new set aside, swaps it in under the root between
    // ShapedTree::BeginReset and EndReset, and only then destroys the old set.
    std::vector<std::unique_ptr<GroupNode>> Subgroups;

    // Bucketed top-level records after filter (descendants not counted, as v1 GroupItemCount).
    // Children.size() for a bottom-level group; the sum over subgroups otherwise.
    uint32_t ItemCount{ 0 };

    // Rows this node's own subtree contributes below it (headers and data rows), given its own and
    // its descendants' expansion and ignoring its ancestors': 0 when collapsed. Lets ShapedTree
    // find a node's visible position and report only the affected range. Maintained by
    // ShapedTree; valid for every node in the shaped tree, shown or hidden (decided, P6).
    // For source parents the same count lives in the record's ShapeState slot.
    uint32_t VisibleDescendantCount{ 0 };

    // Visible-row index over Children, entry j = 1 + child j's VisibleDescendantCount (decided,
    // P1). Total() equals VisibleDescendantCount when expanded; on the shaped root it is the
    // visible list's Size(). Written by ShapedTree; kept for every group, shown or hidden,
    // expanded or not (P6), unless marked stale (P7; rebuilt on its next read).
    FenwickTree ChildRows;

    // Position in the parent's Children (decided, P2); 0 on the shaped root. Written by
    // ShapedTree::Link / Unlink (renumbering the later siblings) and by a rebuild. Valid while the
    // group is in the tree, shown or not.
    uint32_t IndexInParent{ 0 };

    // The header's row value in the visible list (decided, V2): an internal GroupHeaderRow token
    // holding Key and Identity only, created on the first ShapedCollectionView::GetAtCore and reused after, so
    // the header's DataContext is stable. Null until then and on the shaped root. Mutable: the
    // visible list fills it from a const read.
    mutable winrt::Windows::Foundation::IInspectable HeaderRow{ nullptr };

    bool IsRoot() const noexcept { return Parent == nullptr; }
};

// Hash of the tagged pointer, for ShapedRef-keyed maps.
struct ShapedRefHash
{
    size_t operator()(ShapedRef const& ref) const noexcept;
};

static_assert(alignof(SourceRecord) >= 2, "ShapedRef uses the low pointer bit as the group tag");
static_assert(alignof(GroupNode) >= 2, "ShapedRef uses the low pointer bit as the group tag");
static_assert(sizeof(ShapedRef) == sizeof(void*), "ShapedRef must stay one pointer wide");

// Row events from ShapedTree (decided, P8: inline, replaces P4's FlatDelta and publish overlay).
// Raised as each change is made, so a handler that reads the tree sees exactly the list the event
// describes; nothing is recorded or replayed. Implemented by ShapingManager, which forwards to
// ShapedCollectionView and accumulates ShapingChangeFlags. noexcept because the tree calls it from
// the engine's write phase: the implementation catches a handler's throw (see ShapingManager).
struct IShapedRowsListener
{
    virtual ~IShapedRowsListener() = default;

    // count rows at index left the visible list; the tree already reflects it. isMove: the node is
    // being moved (the engine will Link it again in the same commit).
    virtual void OnRowsRemoved(uint32_t index, uint32_t count, bool isMove) noexcept = 0;

    // count rows were inserted at index; the tree already reflects it. isMove as above.
    virtual void OnRowsInserted(uint32_t index, uint32_t count, bool isMove) noexcept = 0;

    // A reset is about to replace the list (BeginReset, before the engine swaps the new tree in).
    // Not a safe point to read the tree: for a source Reset, SourceTable has already rewritten
    // source-owned child lists. The owner captures scroll anchors from its realized elements
    // (#18 O2, P5).
    virtual void OnRowsResetting() noexcept = 0;

    // The reset is done (EndReset): raise one stable Reset.
    virtual void OnRowsReset() noexcept = 0;
};

// The shaped tree as a whole: owns the shaped root and serves the visible list from it (decided,
// P10: replaces the Flattener; as the v6 prototype's NodeTree6). There is no stored visible list
// (P1): every read walks the tree through the counts on its nodes.
//   - Structure: the engine makes every change to a shaped child list through Link / Unlink (one
//     call at the topmost node that enters or leaves, F1) or BeginReset / EndReset (a rebuild).
//     Each keeps IndexInParent (P2), the counts (P6) and the stale bits (P7) correct and raises its
//     event right away (P8).
//   - Reads: Size, GetAt, IndexOf and the per-row helpers. A one-entry cursor (P3) serves the
//     repeater's sequential reads.
//   - Expansion: Expand / Collapse / ExpandSubtree / Reveal / SetAll*. Never involves the engine:
//     shaped children are built eagerly, so no app code runs.
//
// Counts. Each node keeps VisibleDescendantCount (rows below it, 0 when collapsed) and a FenwickTree
// (ChildRows) over its children's 1 + VisibleDescendantCount; both are valid for every shaped node,
// shown or hidden (P6), ChildRows unless stale (P7).
//   - GetAt(i): walk down from the root; at each level ChildRows.Find picks the child whose rows
//     cover the remaining offset. O(depth x log k).
//   - IndexOf(node): walk up ShapedParent; at each level add ChildRows.PrefixSum(IndexInParent),
//     plus 1 for the parent's own row unless it is the root. O(depth x log k).
//   - A count change updates one entry per ancestor level up to and including the first collapsed
//     ancestor (whose own count stays 0), shown or not: O(depth x log k). Expand / Collapse read
//     ChildRows.Total() instead of re-walking the subtree, and a moved node keeps its count.
//   - A membership or order change marks that list's ChildRows stale; it is rebuilt on its next
//     read (CurrentChildRows), O(k). A hidden list stays stale until read, so edits into a
//     collapsed group cost no O(k) per commit.
//
// Readable between calls (P8). Every public call leaves the tree consistent, because a listener may
// read it from inside the event. The engine therefore links a new chain (E8, E9) or a new group's
// subtree while it is detached (plain writes, no events), then Links its topmost node once; removal
// mirrors it. A node that is not linked is never reached by a read.
//
// Expansion state: records use ShapeFlags::Expanded (and OverlayCollapsed, F5) on their
// ShapeState; groups use GroupExpansion, keyed by GroupNode::Identity (owned by ShapingManager).
//
// Owns no app object. Calls no app code itself; its listener may. UI-thread affine.
class ShapedTree final
{
public:
    explicit ShapedTree(GroupExpansion& groupExpansion);

    ShapedTree(ShapedTree const&) = delete;
    ShapedTree& operator=(ShapedTree const&) = delete;

    // Set once by ShapingManager; null means events go nowhere (before the first View()).
    void SetListener(IShapedRowsListener* listener) noexcept;

    // ---- Root -------------------------------------------------------------------------------

    // Stable for the tree's lifetime (decided): a rebuild swaps its Children and Subgroups. Owns
    // the top-level groups through Subgroups, and so (transitively) every GroupNode (E2).
    GroupNode& Root() noexcept;
    GroupNode const& Root() const noexcept;

    // Number of active GroupBy levels (0 when ungrouped). Feeds ShapedDepth() and the bottom-level
    // test. Set by EndReset.
    uint16_t GroupLevelCount() const noexcept;

    // ---- Visible list (read) ----------------------------------------------------------------

    // Root().ChildRows.Total(); 0 when empty.
    uint32_t Size() const noexcept;

    // The node shown at index (group header or data row). Precondition: index < Size().
    //   1. Cursor hit: index == m_cursorIndex -> m_cursorNode; index == m_cursorIndex +/- 1 ->
    //      NextShown / PreviousShown of it, O(1) amortized for a forward scan.
    //   2. Otherwise walk down: node = root, r = index; repeat { (c, before) =
    //      CurrentChildRows(node).Find(r); child = the c-th shaped child; r -= before; if r == 0
    //      return child; r -= 1; node = child }. O(depth x log k).
    // Stores the result in the cursor (mutable).
    ShapedRef GetAt(uint32_t index) const;

    // Index of node in the visible list, or nullopt when an ancestor is collapsed. One upward walk
    // that checks IsShown and sums FlatIndexOf's terms together, O(depth x log k). The cursor is
    // not consulted. The view maps nullopt to -1 for WinRT.
    std::optional<uint32_t> IndexOf(ShapedRef node) const;

    // Every ancestor of node is expanded. O(depth).
    bool IsShown(ShapedRef node) const;

    // Effective expansion. Group: GroupExpansion. Record: context row (fails the filter, in the
    // shaped tree) ? !OverlayCollapsed : Expanded (F5). Feeds TableViewRowInfo::IsExpanded.
    bool IsExpanded(ShapedRef node) const;

    // Number of visible rows below node: its VisibleDescendantCount (valid shown or hidden, P6).
    // For a bottom-level group this is the size of its header band (R3).
    uint32_t ShownDescendantCount(ShapedRef node) const;

    // ---- Structure (engine write phase only) ------------------------------------------------
    //
    // noexcept: the engine's write phase cannot throw. Vector capacity is reserved in its compute
    // phase (E4). The engine still owns group lifetime (Subgroups, its identity map, the retired
    // list); these calls own position, counts and events.

    // node is now at index in parent's shaped child list. For a list the engine owns (a group's
    // Children, a record's ShapedChildren) Link inserts it; for a source-owned list (ChildrenShaped
    // clear) SourceTable already has. Then: renumber IndexInParent from index on, mark the parent's
    // ChildRows stale, set node's counts (isMove: kept from before its Unlink; otherwise
    // ComputeSubtreeCounts, O(subtree)), PropagateCount(parent, +span) through the first collapsed
    // ancestor, invalidate the cursor, and if node is shown raise OnRowsInserted(IndexOf(node),
    // span, isMove). span = 1 + VisibleDescendantCount(node).
    void Link(ShapedRef node, ShapedRef parent, uint32_t index, bool isMove) noexcept;

    // Mirror of Link: index = IndexOf(node) and span, taken first (only the siblings before node
    // matter, so this holds even when SourceTable has already erased node from a source-owned
    // list); erase from an engine-owned list; renumber; mark the parent's ChildRows stale;
    // PropagateCount(parent, -span); for a group, GroupExpansion::Erase for it and every group in
    // its subtree unless isMove (F4); invalidate the cursor; raise OnRowsRemoved if it was shown.
    // node's own counts are left as they are (a move keeps them).
    void Unlink(ShapedRef node, bool isMove) noexcept;

    // Bracket a rebuild. BeginReset raises OnRowsResetting before anything is swapped. The engine
    // then swaps the new Children / Subgroups in under the root (writing IndexInParent as it
    // builds), and EndReset sets groupLevelCount, recomputes every count (ComputeSubtreeCounts from
    // the root: post-order over every shaped node, O(n), P6), invalidates the cursor and raises
    // OnRowsReset. A commit either resets or links, never both.
    void BeginReset() noexcept;
    void EndReset(uint16_t groupLevelCount) noexcept;

    // ---- Expansion --------------------------------------------------------------------------
    //
    // Called by ShapingManager inside the #19 apply scope (source frozen), never during an engine
    // commit. Each returns whether anything changed.

    // Sets the flag (ShapeFlags::Expanded, or GroupExpansion), sets the node's count to
    // CurrentChildRows(node).Total() (children's counts are already valid; O(k) only if stale) and
    // propagates the change through the first collapsed ancestor, O(depth x log k). If shown,
    // raises OnRowsInserted(IndexOf(node) + 1, count). Hidden: the same flag and count work and no
    // event; the rows appear when the ancestor expands. False if already expanded or not
    // expandable.
    bool Expand(ShapedRef node);

    // Reverse of Expand: the count drops to 0, ChildRows is kept for the next Expand, ancestors are
    // updated, and if shown OnRowsRemoved is raised. O(depth x log k). Expand / Collapse of a
    // context row (F5) write only ShapeFlags::OverlayCollapsed.
    bool Collapse(ShapedRef node);

    // Expands a record and every record below it. Sets the flags first, then
    // ComputeSubtreeCounts(node), O(subtree), propagates, and if shown raises one OnRowsInserted of
    // the newly revealed rows after node. Records only: the manager never passes a group.
    bool ExpandSubtree(ShapedRef node);

    // Makes node shown by expanding its collapsed ancestors (decided, #20), bottom-up: every one but
    // the topmost is hidden (flag and count only), then the topmost as Expand does: one event for
    // every revealed row. Node itself is not expanded. False when already shown. Precondition: node
    // is in the shaped tree (ShapingEngine::InShapedTree). O(depth x log k), plus O(k) per stale
    // list on the path.
    bool Reveal(ShapedRef node);

    // One verb per axis. Each sets the state, then recomputes every count and raises one reset
    // (OnRowsResetting, then OnRowsReset).
    // Groups: GroupExpansion::SetAll (moves the baseline). O(1) + O(n).
    void SetAllGroupsExpanded(bool expanded);
    // Records: set or clear ShapeFlags::Expanded on every record, and clear or set
    // OverlayCollapsed on every context row (F5). O(n).
    void SetAllRowsExpanded(bool expanded);

    // ---- Lifetime ---------------------------------------------------------------------------

    // Teardown: empties the root's Children and Subgroups (destroying every group), clears the
    // cursor and the listener. Raises nothing. Called from ShapingEngine::Clear(), before
    // SourceTable::Clear() destroys the records the tree points to. Idempotent.
    void Clear() noexcept;

private:
    // Writes the bit IsExpanded reads: OverlayCollapsed for a context row, else Expanded or
    // GroupExpansion.
    void SetExpanded(ShapedRef node, bool expanded);

    // The node's own VisibleDescendantCount (GroupNode or the record's ShapeState).
    uint32_t& VisibleDescendantCount(ShapedRef node) const;

    // Walks up from parent: adds delta to the entry for the child below it in that ancestor's
    // ChildRows (Add at the child's IndexInParent; skipped if the list is stale, P7), then, if the
    // ancestor is expanded, to its own count and continues; a collapsed ancestor ends the walk (its
    // own count stays 0). Shown or not (P6). O(depth x log k).
    void PropagateCount(ShapedRef parent, int64_t delta) noexcept;

    // Precondition: IsShown(node). Sum up the ancestors of
    // CurrentChildRows(parent).PrefixSum(IndexInParent(n)) + (parent is the root ? 0 : 1).
    uint32_t FlatIndexOf(ShapedRef node) const;

    // The node's ChildRows (GroupNode or the record's ShapeState).
    FenwickTree& ChildRows(ShapedRef node) const;

    // The node's IndexInParent (GroupNode or the record's ShapeState) (P2).
    static uint32_t IndexInParent(ShapedRef node) noexcept;

    // The j-th child in node's shaped child list: Children for a group or the root, ShapedChildren
    // or SourceRecord::Children for a record (as ShapedChildCount). O(1).
    static ShapedRef ShapedChildAt(ShapedRef node, uint32_t j) noexcept;

    // Rewrites IndexInParent for parent's children from index on. O(k - index).
    static void Renumber(ShapedRef parent, uint32_t index) noexcept;

    // ChildRows(node).Build over its shaped children, entry j = 1 + VisibleDescendantCount of
    // child j, clearing the stale bit. O(k).
    void RebuildChildRows(ShapedRef node) const;

    // ChildRows(node), rebuilt first if stale (P7). Every read of a list's ChildRows goes through
    // it, so a list edited several times in one commit is rebuilt at most once per read in between,
    // O(k) each. Const because GetAt is: it touches only the stale bit and the tree's storage,
    // logically unchanged.
    FenwickTree& CurrentChildRows(ShapedRef node) const;

    // Post-order over node's subtree (every shaped node, shown or not): sets each
    // VisibleDescendantCount from its children's and its expansion, and builds ChildRows for every
    // expanded node, leaving a collapsed node's list stale (P7). O(subtree). Used by EndReset,
    // Link of a new node, ExpandSubtree and SetAll* (P6).
    void ComputeSubtreeCounts(ShapedRef node) noexcept;

    // Display-order neighbours among shown nodes, for the cursor (P3). Next: the first shaped
    // child if node is expanded and has one, else the next sibling (IndexInParent + 1) of node or
    // of its nearest ancestor that has one. Previous: the previous sibling's last shown
    // descendant (descend through last children while expanded), else the parent. Null past either
    // end (the root is never returned).
    ShapedRef NextShown(ShapedRef node) const noexcept;
    ShapedRef PreviousShown(ShapedRef node) const noexcept;

    // Clears the cursor. Called by every entry point that changes the tree or a count (P3).
    void InvalidateCursor() const noexcept;

    // Raise helpers: no-ops while m_listener is null.
    void RaiseRemoved(uint32_t index, uint32_t count, bool isMove) const noexcept;
    void RaiseInserted(uint32_t index, uint32_t count, bool isMove) const noexcept;

    // Owned for the engine's lifetime, never replaced. Clear() empties it before SourceTable
    // destroys the records it points to.
    GroupNode m_root;
    uint16_t m_groupLevelCount{ 0 };

    // Group expansion, keyed by GroupNode::Identity. Owned by ShapingManager.
    GroupExpansion& m_groupExpansion;

    // Not owned; ShapingManager.
    IShapedRowsListener* m_listener{ nullptr };

    // Last GetAt answer (decided, P3: a single cursor, invalidated on any tree or count change rather
    // than adjusted). Mutable: GetAt is a const read. m_cursorNode null means empty.
    mutable ShapedRef m_cursorNode;
    mutable uint32_t m_cursorIndex{ 0 };
};
