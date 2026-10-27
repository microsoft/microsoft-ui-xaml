// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <vector>

#include "Engine/ShapedTree.h"

// Flattener -> visible list and owner (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Change pipeline".
//
// One FlatDelta per unit of work (a commit, an expand or collapse). Either a stable Reset (bulk) or
// an ordered list of splices plus the row-state refreshes that follow them (surgical).

// A contiguous range of the visible list replaced. Published one at a time (decided, V4): a
// commit records splices and leaves m_rows alone; ShapingManager::Publish then applies each splice
// to m_rows (Flattener::ApplySplice) and raises its event right away, a Remove of RemovedCount
// rows, then an Add of Inserted.size() rows, at Index (the repeater has no Move). So Size / GetAt
// match every event a consumer sees.
// The flattener emits one-sided splices only ({i, n, {}} or {i, 0, rows}) and never merges them
// into a Replace (decided, #18 O6): the repeater treats Replace as Remove + Add anyway, and an item
// changing in place keeps its row (the record refreshes; chrome goes through a RowStateEdit).
struct FlatSplice
{
    // In the list as it stands after the earlier splices of the same delta.
    uint32_t Index{ 0 };
    uint32_t RemovedCount{ 0 };

    // The rows inserted at Index, in display order. Captured when the splice is recorded, because
    // the tree may change again later in the same commit. A ref may name a group that a later
    // splice of the same delta removes; the engine keeps pruned groups alive until the delta is
    // published (ShapingEngine::ReleaseRetired).
    std::vector<ShapedRef> Inserted;
};

// Row chrome that changed without the row moving. Derived by the flattener from the edits and its
// own toggles; the engine never emits these. Position in set and size of set are not here: every
// later sibling would shift, so UIA reads them on demand.
enum class RowStateField : uint8_t
{
    None = 0x0,

    // Group header count text (GroupNode::ItemCount changed). Groups only: a record's child count
    // is read on demand (UIA); its visible chrome reacts only to the 0 <-> 1 change (Expandable).
    ItemCount = 0x1,

    // ShapedChildCount() went 0 <-> 1, so the expander appears or disappears.
    Expandable = 0x2,

    // The node was expanded or collapsed.
    Expanded = 0x4,
};
DEFINE_ENUM_FLAG_OPERATORS(RowStateField);

// Not a collection change: goes to the owner (TableView), which refreshes the chrome of the realized
// row at Index. Unrealized rows read the state when realized.
struct RowStateEdit
{
    // In the final list (after every splice of the delta).
    uint32_t Index{ 0 };
    RowStateField Fields{ RowStateField::None };
};

// What a published unit of work did to the visible list, for the owner (decided, #18 O5). It maps
// the flags to UIA StructureChanged: Order alone -> ChildrenReordered; Membership or Reset ->
// ChildrenInvalidated; RowState alone -> no structure event. Replaces v1 ShapingChanged(reorderOnly).
enum class ShapingChangeFlags : uint8_t
{
    None = 0x0,

    // The list was replaced by one Reset. Combined with Order (a sort-only reshape: same rows, new
    // order; set by ShapingManager from the spec Diff) or Membership (everything else).
    Reset = 0x1,

    // Rows entered or left the list (insert, remove, filter change, expand, collapse).
    Membership = 0x2,

    // Rows moved without the shown set changing: within a surgical commit, a node removed and
    // inserted again (live re-sort, group-key change, reparent).
    Order = 0x4,

    // States is non-empty.
    RowState = 0x8,
};
DEFINE_ENUM_FLAG_OPERATORS(ShapingChangeFlags);

struct FlatDelta
{
    // Bulk: publish one stable Reset (the visible list implements IKeyIndexMapping, so the repeater
    // reuses elements by key). Splices and States are empty. Flattener::Rebuild staged the new rows
    // during the commit; ShapingManager::Publish calls IShapingOwner::OnResetting (old rows still
    // readable), then Flattener::ApplyReset swaps them in, then raises the Reset (#18 O2).
    bool IsReset{ false };

    // Set by the flattener (Membership, Order, RowState, Reset); ShapingManager adds Order to a
    // sort-only Reset. None for an empty delta.
    ShapingChangeFlags Changes{ ShapingChangeFlags::None };

    // Surgical: applied to m_rows and published one at a time, in order (V4).
    std::vector<FlatSplice> Splices;

    // Surgical: delivered after all splices.
    std::vector<RowStateEdit> States;

    bool IsEmpty() const noexcept { return !IsReset && Splices.empty() && States.empty(); }
};
