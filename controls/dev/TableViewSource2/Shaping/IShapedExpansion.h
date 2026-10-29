// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <optional>

struct ShapedNode;

// The expansion actions ShapedRowMetadata drives (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Row metadata".
//
// Implemented privately by ShapingManager: every action runs inside its commit scope (#19: throws
// while an apply runs), and the tree raises its events as it changes (P8). Split from the reads
// (ShapedRowQueries) so the row metadata adapter can drive expansion without seeing the rest of the
// coordinator. Typed, not keyed (decided, R2): the adapter resolves a key with
// ShapedRowQueries::TryResolveKey first; keys stay a TableView-interface concern.
//
// UI-thread affine.

// Which kind of node an expand-all / collapse-all acts on (R2). Opening every group and expanding
// every tree node are different user actions (ITableViewRowMetadataProvider has one verb per axis).
enum class ExpansionAxis : uint8_t
{
    Groups,   // group headers: GroupExpansion baseline
    Rows,     // records: ShapeFlags::Expanded / OverlayCollapsed
};

struct IShapedExpansion
{
    // Expand (true), collapse (false) or toggle (nullopt) one node: ShapedTree::Expand / Collapse.
    // Returns whether the state changed; false when node is not expandable. Per-item expand is
    // internal only (decided, #20): a public per-item API is a TableView API item (Phase 6) and
    // will resolve to a ShapedNode* and call this.
    virtual bool SetExpanded(ShapedNode* node, std::optional<bool> desired) = 0;

    // ShapedTree::SetAllGroupsExpanded / SetAllRowsExpanded for the axis (one Reset).
    virtual void SetAllExpanded(ExpansionAxis axis, bool expanded) = 0;

    // ShapedTree::ExpandSubtree. No-op for a group or a source without a hierarchy.
    virtual void ExpandSubtree(ShapedNode* node) = 0;

    // Reveal (decided, #20; internal, public API deferred to Phase 6): nullopt, with nothing
    // changed, when ShapingEngine::InShapedTree(node) is false (filtered out). Otherwise
    // ShapedTree::Reveal (one insert event, or nothing when already shown), and returns
    // ShapedTree::IndexOf(node). Callers: a future ScrollIntoView(item), selection by item, jumping
    // to a filter match.
    virtual std::optional<uint32_t> Reveal(ShapedNode* node) = 0;

protected:
    // Never deleted through this interface.
    ~IShapedExpansion() = default;
};
