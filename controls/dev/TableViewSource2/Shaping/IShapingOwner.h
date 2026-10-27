// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <span>

#include "Source/SourceRecord.h"
#include "View/FlatDelta.h"

// ShapingManager -> owner (TableView) channel (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Owner notifications".
//
// Decided (#18): one native interface (O1) instead of v1's separate std::function hooks. The owner
// (TableView, or a sink it owns) implements it and registers through TableViewSource2::SetOwner.
// The source layer never names TableView (dependency rule); TableView may read SourceRecord.
//
// Every call is made on the UI thread from inside ShapingManager::Publish, so inside the #19 apply
// scope: the source is frozen, and a call back into a shaping verb, Refresh or an expansion verb
// throws. Reading the view (Count, GetAt, IndexOf) and the row metadata provider is allowed.
//
// What replaced each v1 hook:
//   ShapingChanged(reorderOnly)  -> ShapingAppliedArgs::Changes (O5)
//   RowsReleased                 -> ShapingAppliedArgs::Retired (O3)
//   LiveShapingHold / Resume     -> TableViewSource2::BeginLiveShapingHold / EndLiveShapingHold (O4)
//   ProjectionChanged            -> none: the view is never swapped
//   SetOwningTableView           -> TableViewSource2::SetOwner, single owner (O7)

// Delivered once per published unit of work, after its last collection event.
struct ShapingAppliedArgs
{
    // What happened to the visible list (#18 O5). The owner maps it to UIA StructureChanged.
    ShapingChangeFlags Changes{ ShapingChangeFlags::None };

    // Rows whose chrome changed without moving, at final indexes (after every splice). Empty for a
    // Reset: every realized row is re-bound anyway.
    std::span<RowStateEdit const> States;

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

    // A Reset is about to be raised (#18 O2). The view still serves the old rows, so the owner
    // captures its scroll-anchor candidates (key and offset of realized rows). The matching
    // restore happens in OnShapingApplied (Changes has Reset). Not called for surgical deltas.
    virtual void OnResetting() = 0;

    // The unit of work is fully published: every splice event or the Reset has been raised, and
    // ShapingEngine::ReleaseRetired has not run yet. Called when the delta is non-empty or
    // Retired is non-empty (a removed hidden record changes nothing visible but still purges).
    virtual void OnShapingApplied(ShapingAppliedArgs const& args) = 0;
};
