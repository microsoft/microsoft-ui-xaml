// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <winrt/Windows.Foundation.h>

#include "tracker_ref.h"
#include "Engine/FenwickTree.h"

struct GroupNode;
struct SourceRecord;

// Engine state stored inline on every SourceRecord (declarations only).
// See docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Engine state storage".
//
// A leaf header: only the standard library, WinRT, tracker_ref and FenwickTree (itself a leaf).
// SourceRecord.h includes it, so Source/ depends on these headers and not on the engine.
//
// Inline, not a lazily allocated unique_ptr: the large parts are vectors, which allocate nothing
// until used, so inline costs only the fixed fields per record and avoids a heap allocation per
// record when shaping starts, a pointer hop and null check on every access, and a rule for when to
// free. Revisit if the fixed size grows.
//
// Written only by the engine (and its ShapedTree for IndexInParent, VisibleDescendantCount,
// ChildRows, Expanded and OverlayCollapsed). The source tree links
// (SourceRecord::Parent/Children) are never written here. UI-thread affine.
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

    // Rows this record's own subtree contributes below it, given its own and its descendants'
    // expansion and ignoring its ancestors': 0 when collapsed. Valid for every record in the shaped
    // tree, shown or hidden (decided, P6, replaces F2), so expanding a node never re-walks its
    // subtree. Same meaning as GroupNode::VisibleDescendantCount. Maintained by ShapedTree.
    uint32_t VisibleDescendantCount{ 0 };

    // Position in the record's shaped parent list (decided, P2): its source parent's shaped child
    // list, its bottom-level group's Children, or the shaped root's Children. Lets IndexOf ask the
    // parent's ChildRows for the rows before this record in O(log k) instead of searching the
    // list, and gives PositionInSet in O(1). Structural, so always valid while the record is in the
    // shaped tree, shown or not; undefined otherwise. Written by ShapedTree::Link / Unlink
    // (surgical insert / remove / move, including a source edit to an unshaped Children list) and
    // by a rebuild, renumbering the later siblings, O(k).
    uint32_t IndexInParent{ 0 };

    // Visible-row index over this record's shaped child list (ShapedChildren when ChildrenShaped is
    // set, SourceRecord::Children otherwise), entry j = 1 + child j's VisibleDescendantCount
    // (decided, P1). Total() equals VisibleDescendantCount when expanded. Written by ShapedTree;
    // kept for every record in the shaped tree, shown or hidden, expanded or not (P6), unless
    // marked stale (P7: rebuilt on its next read).
    FenwickTree ChildRows;

    // One cached key per SortSpec of the applied spec, in level order. Path and Key levels hold the
    // value read once per record (PathEvaluator / ShapeDelegates); Comparer levels hold nothing (the
    // comparer is called with the items). Optional so the engine can destroy keys while the owner
    // is alive; tracked like SourceRecord::Item because a key can reference the app's objects.
    // Valid only while SortKeysValid is set.
    std::vector<std::optional<tracker_ref<winrt::Windows::Foundation::IInspectable>>> SortKeys;

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
    // and through ShapedRef::ShapedParent() by ShapedTree's ancestor walks. Replaces a
    // record -> group map. The group key is not cached here: it lives on GroupNode::Key.
    GroupNode* Group{ nullptr };
};
