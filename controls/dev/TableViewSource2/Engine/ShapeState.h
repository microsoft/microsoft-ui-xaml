// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <winrt/Windows.Foundation.h>

#include "Engine/FenwickTree.h"

struct GroupNode;
struct SourceRecord;

// The shaped-node base and the engine state stored inline on every SourceRecord (declarations
// only). See docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Shaped tree
// representation" and "Engine state storage".
//
// A leaf header: only the standard library, WinRT and FenwickTree (itself a leaf).
// SourceRecord.h includes it, so Source/ depends on these headers and not on the engine.

// Which concrete type a ShapedNode is. Fixed at construction.
enum class ShapedNodeKind : uint8_t
{
    Record,
    Group,
};

// Common base of every node in the shaped tree: SourceRecord (a data row) and GroupNode (a group
// header, or the shaped root). Holds what ShapedTree maintains on every node, so its walks read the
// fields directly whatever the kind. A ShapedNode* is the shaped tree's node handle: one pointer,
// null for "no node", compared and hashed as a pointer.
//
// Not polymorphic (decided): no vtable, protected non-virtual destructor, never deleted or copied
// through this type. AsRecord / AsGroup check Kind, then static_cast.
//
// Deliberately has no parent field (decided). SourceRecord::Parent (source tree, SourceTable-owned,
// null for a top-level record) and GroupNode::Parent stay separate and typed, so a source walk can
// never step into a group. ShapedParent() derives the shaped parent from them.
//
// Size: Kind sits in the padding before VisibleDescendantCount. ShapeFlags stays in ShapeState
// (record-only); SourceRecord packs its small fields to absorb that struct's padding.
struct ShapedNode
{
    ShapedNodeKind const Kind;

    // Rows this node's own subtree contributes below it (group headers and data rows), given its
    // own and its descendants' expansion and ignoring its ancestors': 0 when collapsed. Valid for
    // every node in the shaped tree, shown or hidden (decided, P6, replaces F2), so expanding a
    // node never re-walks its subtree. Maintained by ShapedTree.
    uint32_t VisibleDescendantCount{ 0 };

    // Position in the node's shaped parent list (decided, P2): a group's Children, the shaped
    // root's Children, or a record's shaped child list (ShapedChildren, or SourceRecord::Children
    // when unshaped). Lets IndexOf ask the parent's ChildRows for the rows before this node in
    // O(log k) instead of searching the list, and gives PositionInSet in O(1). Structural, so
    // always valid while the node is in the shaped tree, shown or not; undefined otherwise; 0 on
    // the shaped root. Written by ShapedTree::Link / Unlink (surgical insert / remove / move,
    // including a source edit to an unshaped Children list) and by a rebuild, renumbering the later
    // siblings, O(k).
    uint32_t IndexInParent{ 0 };

    // Visible-row index over this node's shaped child list (ShapedChildCount / ShapedChildAt),
    // entry j = 1 + child j's VisibleDescendantCount (decided, P1). Total() equals
    // VisibleDescendantCount when expanded; on the shaped root it is the visible list's Size().
    // Written by ShapedTree; kept for every node in the shaped tree, shown or hidden, expanded or
    // not (P6), unless marked stale (P7: rebuilt on its next read).
    FenwickTree ChildRows;

    bool IsGroup() const noexcept { return Kind == ShapedNodeKind::Group; }

    // Null if this node is the other kind.
    SourceRecord* AsRecord() noexcept;
    SourceRecord const* AsRecord() const noexcept;
    GroupNode* AsGroup() noexcept;
    GroupNode const* AsGroup() const noexcept;

    // Parent in the shaped tree, O(1). Null only for the shaped root:
    //   group: GroupNode::Parent
    //   nested record: SourceRecord::Parent (source parent)
    //   top-level record: Shape.Group (bottom-level group, or the shaped root when ungrouped)
    // The one upward walk used by ShapedTree (IndexOf, IsShown, count propagation), set size,
    // parent identity and "go to parent". There is no stored visible list (P1).
    ShapedNode* ShapedParent() const noexcept;

    // Children this node has in the shaped tree, O(1). The single rule for expandability
    // (IsExpandable = ShapedChildCount() > 0), ChildCount on a record row, SizeOfSet (called on
    // ShapedParent()) and the expander chrome the owner re-reads (P9):
    //   group or shaped root: Children.size()
    //   record, ChildrenShaped set: Shape.ShapedChildren.size()
    //   record otherwise: SourceRecord::Children.size()
    // Not HasChildren(): under a filter that reads the source tree and stays true when every child
    // is filtered out. Not stored; derived on demand. IsGroup() is the only group test: a source
    // parent (ShapedChildCount() > 0) is expandable but is not a group.
    uint32_t ShapedChildCount() const noexcept;

    // The j-th child in the shaped child list, by the same three cases as ShapedChildCount. O(1).
    ShapedNode* ShapedChildAt(uint32_t j) const noexcept;

protected:
    explicit ShapedNode(ShapedNodeKind kind) noexcept : Kind(kind) {}
    ~ShapedNode() = default;

    ShapedNode(ShapedNode const&) = delete;
    ShapedNode& operator=(ShapedNode const&) = delete;
};

// Record-only engine state, stored inline on every SourceRecord (SourceRecord::Shape). The
// per-node counts and position live on the ShapedNode base instead, shared with GroupNode.
//
// Inline, not a lazily allocated unique_ptr: the large parts are vectors, which allocate nothing
// until used, so inline costs only the fixed fields per record and avoids a heap allocation per
// record when shaping starts, a pointer hop and null check on every access, and a rule for when to
// free. Revisit if the fixed size grows.
//
// Written only by the engine (and its ShapedTree for Expanded and OverlayCollapsed). The source
// tree links (SourceRecord::Parent/Children) are never written here. UI-thread affine.
enum class ShapeFlags : uint8_t
{
    None = 0x0,

    // Passes every filter predicate (true when no filter is active).
    PassesFilter = 0x1,

    // There is no "has a passing descendant" flag (decided, E8): a record failing the filter stays
    // in the shaped tree as an ancestor of a match exactly when its ShapedChildren is non-empty.
    // See ShapingEngine::InShapedParent.

    // ShapedChildren is current. When clear, the shaped children are SourceRecord::Children as-is
    // (no filter and no sort active). Never depends on expansion: shaping is eager.
    ChildrenShaped = 0x2,

    // SortKeys is current for the applied spec. Cleared on ItemChanged and on a ShapeDelta::Sort.
    SortKeysValid = 0x4,

    // The record's rows are expanded. Records keep their own expansion (they survive reshapes and,
    // through the Refresh() match, Resets, including child records); only groups need the
    // expansion model. Default clear (collapsed).
    // Written and read by ShapedTree (Expand / Collapse, SetAllRowsExpanded).
    Expanded = 0x8,

    // Filter auto-expand overlay (decided, F5). A context row (in the shaped tree, failing the
    // filter) shows expanded unless this is set; toggling a context row writes only this bit, so
    // Expanded keeps the user's state for when the filter is cleared. Written by ShapedTree;
    // cleared by the engine's full Filter stage (filter added, changed or removed, Reapply), not by
    // a live re-read or a surgical edit. Ignored on a record that passes the filter.
    OverlayCollapsed = 0x10,

    // Transient tie-run mark for surgical placement (decided, E5). Set by
    // ShapingEngine::PlacementIndex on the shaped siblings whose keys tie with the record being
    // placed, read by its backward walk over the source siblings, and cleared before it returns.
    // Never set outside that call.
    PlacementMark = 0x20,
};
DEFINE_ENUM_FLAG_OPERATORS(ShapeFlags);

struct ShapeState
{
    ShapeFlags Flags{ ShapeFlags::None };

    // One cached key per SortSpec of the applied spec, in level order. Path and Key levels hold the
    // value read once per record (PathEvaluator / ShapeDelegates); Comparer levels hold nothing (the
    // comparer is called with the items; their slot stays null). A null slot reads as a null key.
    // Strong references, like SourceRecord::Item (Header-Changes drift #73): a key can reference
    // the app's objects, which then stay alive while the record holds the key.
    // Valid only while SortKeysValid is set.
    std::vector<winrt::Windows::Foundation::IInspectable> SortKeys;

    // Source children after filter and sort, in shaped order. Non-owning. Valid only while
    // ChildrenShaped is set. Built eagerly for every parent whenever a filter or sort is active,
    // expanded or not, so expansion never runs app code and every edit has a shaped index.
    // Holds exactly the children for which ShapingEngine::InShapedParent is true, so under a
    // filter a non-empty list is what keeps a failing record visible as an ancestor of a match.
    std::vector<SourceRecord*> ShapedChildren;

    // Top-level records only: the GroupNode holding this record in the shaped tree, i.e. its shaped
    // parent: the bottom-level group while GroupBy is active, the shaped root otherwise. Null for
    // nested records (their shaped parent is SourceRecord::Parent) and for a top-level record not in
    // the shaped tree (filtered out). The source tree has no group links (a top-level record's
    // Parent is null), so this is the only upward link from a record to a GroupNode. Non-owning.
    // Written by the engine when it places or moves a top-level record (bucketing, re-bucketing, a
    // GroupBy change); read by the engine to find the old group on remove or a group-key change,
    // and through ShapedNode::ShapedParent() by ShapedTree's ancestor walks. Replaces a
    // record -> group map. The group key is not cached here: it lives on GroupNode::Key.
    GroupNode* Group{ nullptr };
};
