// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <winrt/Windows.Foundation.h>

#include "Source/SourceRecord.h"

// Shaped tree node types for TableViewSource2 (declarations only).
// See docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Shaped tree
// representation".
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
//     visible list (the flattener starts at its Children).
//   - Groups bucket SourceTable::TopLevel() only. A record's source descendants are reached through
//     SourceRecord::Children (or the filtered/sorted children in the record's engine ShapeState),
//     never through a group.
//   - Applying or removing GroupBy rebuilds GroupNodes (O(n) bucketing); the source tree is
//     untouched. Engine state for records lives in each record's ShapeState slot.
//   - ShapedRef and GroupNode stay native: the engine, the flattener (whose visible list is a
//     vector of ShapedRef), the WinRT visible list and the row metadata provider. TableView sees
//     items and the TableViewRowInfo / TableViewGroupInfo derived from them.
//   - UI-thread affine, like SourceTable. GroupNodes are owned by the engine in a container with
//     stable addresses, so GroupNode* and ShapedRef stay valid until the engine drops the group.

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
    // The one upward walk used by the flattener (IndexOf, IsShown, count propagation), set size,
    // parent identity and "go to parent". Not stored in the visible list.
    ShapedRef ShapedParent() const noexcept;

    // Children this node has in the shaped tree, O(1). The single rule for expandability
    // (IsExpandable = ShapedChildCount() > 0), ChildCount on a record row, SizeOfSet (called on
    // ShapedParent()) and the flattener's Expandable state (count went 0 <-> 1):
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

// A group header in the shaped tree, or the shaped root. Engine-owned. Group headers exist only
// while GroupBy is active (all are dropped when it is cleared); the shaped root lives as long as
// the engine. Surfaced to consumers as TableViewGroupInfo (Key, ItemCount, Level = GroupLevel).
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
    // Not the header text (that is StringifyGroupKey, culture-formatted and possibly colliding).
    winrt::hstring Identity;

    // Containing group; nullptr only on the shaped root.
    GroupNode* Parent{ nullptr };

    // Level among group headers: top-level groups = 0. Also the group's ShapedDepth(). Unused on
    // the shaped root.
    uint16_t GroupLevel{ 0 };

    // Subgroups (all IsGroup()), or, for a bottom-level group, top-level records after filter and
    // sort. Never mixed. On the shaped root: top-level groups, or top-level records when ungrouped.
    // Bottom-level is not stored or tested here: the engine knows it from its group-level count
    // (GroupLevel + 1 == count; the shaped root when count == 0). Children cannot tell an empty
    // group's kind.
    std::vector<ShapedRef> Children;

    // Bucketed top-level records after filter (descendants not counted, as v1 GroupItemCount).
    // Children.size() for a bottom-level group; the sum over subgroups otherwise.
    uint32_t ItemCount{ 0 };

    // Rows this node's own subtree contributes below it (headers and data rows), given its own and
    // its descendants' expansion and ignoring its ancestors': 0 when collapsed. Lets the flattener
    // find a node's visible position and splice only the affected range. Maintained by the
    // flattener.
    // For source parents the same count lives in the record's ShapeState slot.
    uint32_t VisibleDescendantCount{ 0 };

    bool IsRoot() const noexcept { return Parent == nullptr; }
};

// Hash of the tagged pointer, for ShapedRef-keyed maps (the flattener's dirty set).
struct ShapedRefHash
{
    size_t operator()(ShapedRef const& ref) const noexcept;
};

static_assert(alignof(SourceRecord) >= 2, "ShapedRef uses the low pointer bit as the group tag");
static_assert(alignof(GroupNode) >= 2, "ShapedRef uses the low pointer bit as the group tag");
static_assert(sizeof(ShapedRef) == sizeof(void*), "ShapedRef must stay one pointer wide");
