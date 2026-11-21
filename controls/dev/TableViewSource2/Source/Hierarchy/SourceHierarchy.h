// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

#include <winrt/Windows.Foundation.h>

#include "Source/SourceRecord.h"
#include "ISourceHierarchyHost.h"

// Source-tree strategy base and shared tree helpers. See
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
    //
    // Read once (decided, H1/H5): ChildrenBy keeps what it read (each item's child collection and
    // children) as the incoming stage, replacing any earlier one, and OnInserted() builds from it
    // without calling the selector again. A selector throw is caught here and makes that item a
    // leaf; OnInserted() rethrows it after the Add. Between this call and OnInserted() SourceTable
    // calls DiscardIncoming() on every throw, and RemoveAt (OnRemoving) leaves the stage alone, so
    // a hierarchical ReplaceAt validates and inserts from the same read.
    virtual void CollectIncoming(
        winrt::Windows::Foundation::IInspectable const& item,
        std::vector<winrt::Windows::Foundation::IInspectable>& items) = 0;

    // Drops the incoming stage (and the app references it holds). ParentBy: nothing. Idempotent.
    virtual void DiscardIncoming() noexcept = 0;

    // Root-collection edits (decided, A1: the strategy owns the hierarchical edit). SourceTable
    // has updated m_records and the lookups; the strategy links or unlinks the record and raises
    // the change through the host, then what follows from it (ParentBy: orphan Moves, or the one
    // bulk Reset when an insert adopts more than one orphan; spec Orphan adoption, Bulk
    // threshold). A selector or key error never undoes the edit (spec Misbehaving records): it is
    // rethrown after everything was raised. A strategy that faults part way through its own raise
    // sequence calls host.RequestReset() before rethrowing.
    //   OnInserted: record is in RootRecords() and the lookups but not linked. ParentBy keys it,
    //     links it under its parent (or the top level) in source order, raises the Add (with no
    //     Add for an excluded record) and adopts the top-level orphans waiting for its key.
    //     ChildrenBy links it at the top level, builds its feed and subtree from the incoming
    //     stage (which must be for record->Item) and raises the Add.
    //   OnRemoving: record has left RootRecords() and the lookups but is still linked; removed
    //     holds it. ParentBy unkeys it, moves each child to the top level (one Move each, while
    //     record is still in the shaped tree), unlinks it and raises the Remove (nothing for an
    //     excluded record: its Remove was raised when it was excluded). ChildrenBy appends its
    //     subtree to removed (pre-order), unlinks it and raises the Remove.
    //   OnMoved: a root move, after m_records changed. The record keeps its parent; the strategy
    //     re-places it among its siblings and SourceTable raises a Move when its sibling index
    //     changed. An unlinked (excluded) record is left alone.
    // A Replace under a hierarchy is RemoveAt then InsertAt (decided, D1), so no hook covers it.
    virtual void OnInserted(ISourceHierarchyHost& host, SourceRecord* record) = 0;
    virtual void OnRemoving(ISourceHierarchyHost& host, SourceRecord* record, std::vector<SourceRecordPtr>& removed) = 0;
    virtual void OnMoved(ISourceHierarchyHost& host, SourceRecord* record, uint32_t fromIndex, uint32_t toIndex) = 0;

    // Visits every record the strategy owns outside RootRecords() (ChildrenBy child records), for
    // SourceTable::SetLiveShaping. ParentBy: none. No app calls.
    virtual void ForEachChildRecord(std::function<void(SourceRecordPtr const&)> const& visit) const = 0;

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
    // child feed still held and its records, and drops the selectors, keys and any stage.
    // Teardown-safe: no app calls, revokes through safe_get, never calls host.ReleaseRecord
    // (decided, C1: that reads Item; every caller has already taken the records out of the
    // lookups, or SourceTable::Clear() clears them wholesale). Idempotent.
    virtual void Release(ISourceHierarchyHost& host) noexcept = 0;

    // Unique per strategy (decided, A4): a stale child-collection handler compares it, so a new
    // strategy allocated at a freed one's address is never mistaken for it.
    uint64_t Generation() const noexcept { return m_generation; }

protected:
    SourceHierarchy() noexcept : m_generation(++s_lastGeneration) {}

private:
    // Atomic: each UI thread (one per window) creates strategies for its own tables.
    static inline std::atomic<uint64_t> s_lastGeneration{ 0 };
    uint64_t const m_generation;
};

// Appends record and its source subtree, pre-order (SourceChange::Added for an insert).
void AppendSourceSubtree(SourceRecord* record, std::vector<SourceRecord*>& out);

// record's index in siblings, or siblings.size() when it is not there.
uint32_t IndexInSourceSiblings(std::vector<SourceRecord*> const& siblings, SourceRecord const* record) noexcept;
