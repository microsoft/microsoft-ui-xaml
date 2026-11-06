// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <vector>

#include <winrt/Windows.Foundation.h>

#include "Source/SourceRecord.h"
#include "ISourceHierarchyHost.h"

// Source-tree strategies (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "Source tree strategies".
//
// Decided (split hierarchy out of SourceTable, option A): SourceTable keeps everything that is
// always on (records, lookups, live shaping, the re-entrancy gate) and the shared tree operations
// (m_topLevel, SiblingsOf, Reparent, UpdateDepths, ResetTreeToFlat). What one hierarchy verb needs
// lives in a SourceHierarchy strategy that SourceTable owns as std::unique_ptr (null = flat):
// ParentKeyHierarchy for ParentBy, ChildCollectionsHierarchy for ChildrenBy. Setting or clearing a
// verb swaps the strategy on a live table, which is why this is a strategy and not a
// HierarchicalSourceTable subclass. Strategies reach back only through ISourceHierarchyHost.
//
// Lifetime and threading: a strategy lives on the UI thread inside its SourceTable. Its tracker_refs
// (selectors, object keys, child accessors) are bound to host.TrackerOwner(), so Release() must run
// while that owner is alive (SourceTable::Clear() calls it first). Every app call (selectors,
// child enumeration) happens in Prepare(), CollectIncoming() or Reevaluate(), never in Release().
//
// Files: ISourceHierarchyHost.h (host interface), SourceHierarchy.h (strategy base),
// ParentKeyHierarchy.h (ParentBy), ChildCollectionsHierarchy.h (ChildrenBy).

// One hierarchy verb's state and tree maintenance. Created by the verb, prepared, then committed;
// replaced or released by the next verb, ClearHierarchy or SourceTable::Clear.
//
// Two-phase build (rejection contract): Prepare() runs every app call and every check and writes
// only the strategy's own scratch state; a throw leaves the table and the current strategy as they
// were. Commit() then links every record through the host and cannot fail on app input (it may still
// throw on allocation). SourceTable releases the old strategy between the two.
//
// Under DeferRefresh the verb only constructs the strategy and parks it unprepared in
// SourceTable::m_pendingHierarchy; the re-read at the outermost Close prepares and commits it.
//
// Verb flow: Prepare(host, current root items); old->TakeChildRecords(host, removed);
// old->Release(host); ResetTreeToFlat(); Commit(host, added); one Reset.
//
// Refresh flow (decided: reuse, per-feed match). SourceTable::Refresh() re-reads the tree with the
// current strategy (or the parked one, which replaces it as in the verb flow):
//   1. Read: re-enumerate the root, ValidateItems(), then Prepare(host, rootItems). Every app call
//      and check; a throw leaves the table untouched.
//   2. Match: SourceTable::MatchRecords() matches the root; MatchForRefresh() matches each feed.
//      Every unmatched record leaves the lookups before anything is created, so an item that moved
//      to another parent (or between the root and a child collection) gets a new record instead of
//      colliding with its old one.
//   3. Link: SourceTable::CommitRecords() creates the unmatched root records (no strategy hooks),
//      ResetTreeToFlat(), then Commit() relinks every record, reusing the matched child records and
//      creating the rest.
//   4. One Reset: Added = records created, Removed = records that left (alive for the handler).
// A reused record keeps its pointer, Identity(), PropertyChanged subscription and expansion flag.
// A freshly constructed strategy has no feeds, so its Commit() creates every child record.
class SourceHierarchy
{
public:
    virtual ~SourceHierarchy() = default;

    SourceHierarchy(SourceHierarchy const&) = delete;
    SourceHierarchy& operator=(SourceHierarchy const&) = delete;

    // Evaluates the selectors over rootItems in root order (a verb passes the current root items,
    // Refresh() the re-enumerated ones; ChildrenBy also enumerates every child collection
    // recursively) and validates the whole tree, including duplicates across collections. Staged
    // state is keyed by root position, not by record: under Refresh() the root is not matched yet.
    // Throws hresult_invalid_argument with v1's messages; writes nothing outside the strategy.
    virtual void Prepare(
        ISourceHierarchyHost& host,
        std::vector<winrt::Windows::Foundation::IInspectable> const& rootItems) = 0;

    // Refresh() only, between Prepare() and Commit(). rootMatches[i] is the existing root record
    // kept at prepared root position i, or nullptr when position i gets a new record. ChildrenBy
    // matches each kept record's feed against the staged children of the same position, with the
    // root rules (raw pointer at the same position, then raw pointer or address among the
    // unmatched); there is no matching across feeds. Every unmatched child record, with its
    // subtree, and every feed not reached from a kept record (its owner left) is unsubscribed as
    // needed and moved out of the lookups (host.ReleaseRecord) into removed. ParentBy: nothing
    // (every record is a root record). No app calls.
    virtual void MatchForRefresh(
        ISourceHierarchyHost& host,
        std::vector<SourceRecord*> const& rootMatches,
        std::vector<SourceRecordPtr>& removed) = 0;

    // Links every record from the prepared state, with host.RootRecords() now in prepared root
    // order, rewriting Parent / Children / Depth on every record. ChildrenBy reconciles each feed:
    // reuses the records MatchForRefresh() kept, creates the rest through the host, swaps the
    // accessor and subscription when the child collection object changed, and drops a feed whose
    // collection is now null. Appends every record it creates to added (pre-order), then drops
    // the scratch state. No app calls. The caller raises the one Reset.
    virtual void Commit(ISourceHierarchyHost& host, std::vector<SourceRecord*>& added) = 0;

    // Appends the items an incoming root item brings, after the item itself, for validation
    // before anything is created. ParentBy: nothing. ChildrenBy: its whole subtree, pre-order.
    virtual void CollectIncoming(
        winrt::Windows::Foundation::IInspectable const& item,
        std::vector<winrt::Windows::Foundation::IInspectable>& items) const = 0;

    // Root-collection edits, after m_records changed and before the SourceChange is raised. Each
    // fills the change's tree fields (parents, sibling indexes) and may add Moves to raise after it
    // (ParentBy orphan adoption; see SourceChanged in SourceTable).
    //   OnInserted: ParentBy keys the record, links it under its parent (or top level) and adopts
    //     orphans whose parent key matches; ChildrenBy builds its feed and subtree.
    //   OnRemoving: ParentBy unkeys it and moves its children to the top level; ChildrenBy destroys
    //     its feed and subtree (records released through the host).
    //   OnMoved: a root move. ParentBy: no tree change unless the record is top-level (its sibling
    //     index follows source order). ChildrenBy: a top-level move.
    // A replace is OnRemoving then OnInserted.
    virtual void OnInserted(ISourceHierarchyHost& host, SourceRecord* record) = 0;
    virtual void OnRemoving(ISourceHierarchyHost& host, SourceRecord* record) noexcept = 0;
    virtual void OnMoved(ISourceHierarchyHost& host, SourceRecord* record, uint32_t fromIndex, uint32_t toIndex) = 0;

    // Live shaping saw PropertyChanged on record: re-evaluate its key and parent key (reparent) or
    // its children property (swap the feed) and raise the resulting SourceChange through the host.
    // Returns true if the tree changed. Called before ItemChanged is forwarded.
    virtual bool Reevaluate(ISourceHierarchyHost& host, SourceRecord* record, winrt::hstring const& propertyName) = 0;

    // Before Release() when the strategy is replaced or cleared (a verb, or Refresh() committing a
    // parked strategy): unsubscribes every feed and moves every child record out of the lookups
    // (host.ReleaseRecord) into removed, so the Reset can report them while they are alive.
    // ParentBy: nothing. No app calls.
    virtual void TakeChildRecords(ISourceHierarchyHost& host, std::vector<SourceRecordPtr>& removed) = 0;

    // Unlinks nothing (the caller resets the tree to flat), but unsubscribes and destroys every
    // child feed still held and its records (through host.ReleaseRecord), and drops the selectors
    // and keys. Teardown-safe: no app calls, revokes through safe_get. Idempotent.
    virtual void Release(ISourceHierarchyHost& host) noexcept = 0;

protected:
    SourceHierarchy() = default;
};
