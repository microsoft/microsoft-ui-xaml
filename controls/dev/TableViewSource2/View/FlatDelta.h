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
// Today the flattener emits one-sided splices only ({i, n, {}} or {i, 0, rows}) and never merges
// them; merging an adjacent remove + insert into a Replace is revisited with owner notifications.
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

struct FlatDelta
{
    // Bulk: publish one stable Reset (the visible list implements IKeyIndexMapping, so the repeater
    // reuses elements by key). Splices and States are empty. Flattener::Rebuild already wrote
    // m_rows during the commit: one event, so no intermediate state can be observed.
    bool IsReset{ false };

    // Surgical: applied to m_rows and published one at a time, in order (V4).
    std::vector<FlatSplice> Splices;

    // Surgical: delivered after all splices.
    std::vector<RowStateEdit> States;

    bool IsEmpty() const noexcept { return !IsReset && Splices.empty() && States.empty(); }
};
