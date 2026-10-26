// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>

#include "Engine/ShapedTree.h"

// Engine -> flattener change contract (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Change pipeline".
//
// Two paths:
//   - Surgical: a local change (source add/remove/replace/move, live-shaping filter, sort-key or
//     group-key change, reparent, orphan adoption) is reported as ShapedEdits.
//   - Bulk: a reshape (Reapply(): spec verbs, hierarchy verbs, SourceTable Reset, recovery, a
//     range above the threshold) rebuilds the shaped tree and reports OnRebuilt once.
//
// The engine calls the sink only in the commit phase, after every call into app code for the unit
// of work has succeeded ("Misbehaving records": compute, then commit). The sink therefore never
// sees a change that later throws, and it never calls app code itself.

enum class ShapedEditKind : uint8_t
{
    Insert,
    Remove,
};

// One structural change to the shaped tree. The node moves with its whole subtree.
//   - Move (reparent, live re-sort, group-key change): a Remove at the old location, then an
//     Insert at the new one.
//   - Group created or dropped: an Insert or Remove of the GroupNode (with its subtree).
//   - Replace from the source: a Remove of the old record, then an Insert of the new one.
struct ShapedEdit
{
    ShapedEditKind Kind{ ShapedEditKind::Insert };

    // Record or group header.
    ShapedRef Node;

    // Shaped parent at this location:     GroupNode (the shaped root at top level) or source parent
        // record; never null. For a Remove, the location the node is leaving; for an Insert, where it now is.
    ShapedRef Parent;

    // Position among Parent's shaped children at this location.
    uint32_t IndexInParent{ 0 };
};

// Implemented by the flattener. The engine holds it by reference and never sees the visible list.
struct IShapedEditSink
{
    virtual ~IShapedEditSink() = default;

    // Before the engine unlinks e.Node: the old tree is intact, so the sink can find the node's
    // flat index and span.
    virtual void OnRemoving(ShapedEdit const& e) = 0;

    // After the engine links e.Node: the new tree is in place.
    virtual void OnInserted(ShapedEdit const& e) = 0;

    // After a full re-apply has rebuilt the shaped tree. root is the engine's single stable root
    // (same address every call; only its children are swapped). groupLevelCount is the number of
    // active GroupBy levels (0 when ungrouped); it feeds ShapedDepth() and the bottom-level test.
    virtual void OnRebuilt(GroupNode& root, uint16_t groupLevelCount) = 0;
};
