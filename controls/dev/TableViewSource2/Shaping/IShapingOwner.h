// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <span>

#include "Source/SourceRecord.h"

// ShapingManager -> owner (TableView) channel (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Owner notifications".
//
// Decided (#18): one native interface (O1) instead of v1's separate std::function hooks. The owner
// (TableView, or a sink it owns) implements it and registers through TableViewSource2::TrySetOwner,
// which arbitrates (B2) and hands the accepted owner to ShapingManager::SetOwner.
// The source layer never names TableView (dependency rule); TableView may read SourceRecord.
//
// Every call is made on the UI thread from inside a ShapingManager commit, so inside the #19 apply
// scope: the source is frozen, and a call back into a shaping verb, Refresh or an expansion verb
// throws. Reading the view (Count, GetAt, IndexOf) and the row metadata provider is allowed.
//
// What replaced each v1 hook:
//   ShapingChanged(reorderOnly)  -> ShapingAppliedArgs::Changes (O5)
//   RowsReleased                 -> ShapingAppliedArgs::Retired (O3)
//   LiveShapingHold / Resume     -> TableViewSource2::BeginLiveShapingHold / EndLiveShapingHold (O4)
//   ProjectionChanged            -> none: the view is never swapped
//   SetOwningTableView           -> TableViewSource2::TrySetOwner, single owner (O7)
//   ReleaseOwningTableView       -> TableViewSource2::ReleaseOwner
//   Add / RemoveOwnerReleasedHandler -> TableViewSource2::Add / RemoveOwnerReleasedWaiter

// What a unit of work did to the visible list, for the owner (decided, #18 O5). It maps the flags
// to UIA StructureChanged: Order alone -> ChildrenReordered; Membership or Reset ->
// ChildrenInvalidated. Accumulated by ShapingManager from the IShapedRowsListener events (P8).
// Replaces v1 ShapingChanged(reorderOnly).
enum class ShapingChangeFlags : uint8_t
{
    None = 0x0,

    // The list was replaced by one Reset. Combined with Order (a sort-only reshape: same rows, new
    // order; set by ShapingManager from the spec Diff) or Membership (everything else).
    Reset = 0x1,

    // Rows entered or left the list (insert, remove, filter change, expand, collapse): any event
    // that is not a move, or move events whose row counts do not net to zero.
    Membership = 0x2,

    // Rows moved without the shown set changing: within a surgical commit, a node unlinked and
    // linked again (live re-sort, group-key change, reparent). Set by any isMove event.
    Order = 0x4,
};
DEFINE_ENUM_FLAG_OPERATORS(ShapingChangeFlags);

// Delivered once per unit of work, after its last collection event.
struct ShapingAppliedArgs
{
    // What happened to the visible list (#18 O5). The owner maps it to UIA StructureChanged.
    ShapingChangeFlags Changes{ ShapingChangeFlags::None };

    // Records that left the source in this unit of work (deleted, or gone in a source Reset),
    // whether or not they were shown (#18 O3). Valid only during the call: the records are
    // destroyed after the SourceTable handler returns. The owner purges selection, focus and
    // comparer ranks by Item() / Identity(). Records hidden by a filter or a collapse are not here:
    // their state survives so it returns when they are shown again.
    std::span<SourceRecord const* const> Retired;
};

struct IShapingOwner
{
    virtual ~IShapingOwner() = default;

    // A Reset is about to be raised (#18 O2, P5; called from ShapedTree::BeginReset, P8). The owner
    // must not read rows through the view: for a source Reset, SourceTable has already rewritten
    // source-owned child lists while the counts are still old. It captures its scroll-anchor
    // candidates from its realized row elements instead: key from the DataContext (a
    // GroupedEntry's Identity, or the record found by item through SourceTable, D4; an item no
    // longer present is skipped) and offset from the element's position. The repeater has not seen
    // the Reset yet, so the realized elements are still the old ones. The matching restore happens
    // in OnShapingApplied (Changes has Reset). Not called for surgical commits. Should not throw:
    // it runs inside the engine's write phase, so ShapingManager catches a throw, raises one Reset
    // and rethrows after the commit.
    virtual void OnResetting() = 0;

    // The unit of work is done: every row event or the Reset has been raised, and
    // ShapingEngine::ReleaseRetired has not run yet. Called when Changes is not None, Retired is
    // non-empty (a removed hidden record changes nothing visible but still purges), or a surgical
    // source change was applied with no row event (Changes is None, but a staying row's expander
    // may have changed: a first child linked under a collapsed parent).
    //
    // Chrome (decided, P9: replaces the States list of O5). Row events cover only rows that enter,
    // leave or move; a row that stays can still change its chrome (expander shown or hidden as its
    // first child comes or goes, a group header's item count, IsExpanded after a verb). The owner
    // therefore re-reads the chrome of every realized row here through the row metadata provider:
    // O(realized), no per-row bookkeeping in the engine. Skipped when Changes is Reset (every
    // realized row is re-bound anyway).
    virtual void OnShapingApplied(ShapingAppliedArgs const& args) = 0;
};
