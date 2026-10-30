// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <winrt/Microsoft.UI.Xaml.Interop.h>
#include <winrt/Windows.Foundation.h>

#include "CollectionAccessor2.h"
#include "SourceRecord.h"

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
// (selectors, kept child accessors) are bound to host.TrackerOwner(), so Release() must run
// while that owner is alive (SourceTable::Clear() calls it first). ParentBy object keys are strong
// references (drift #73). Every app call (selectors,
// child enumeration) happens in Prepare(), CollectIncoming() or Reevaluate(), never in Release().

class SourceHierarchy;

// What a strategy may ask of its SourceTable. SourceTable implements it privately. Every member
// runs on the UI thread.
struct ISourceHierarchyHost
{
    // The tracker owner every app object a strategy holds is bound to. Null once detached.
    virtual ITrackerHandleManager const* TrackerOwner() const noexcept = 0;

    // Weak self for event lambdas (child collections). Locked only on the UI thread (SourceTable
    // Lifetime rule); an expired host drops the notification.
    virtual std::weak_ptr<ISourceHierarchyHost> WeakHost() noexcept = 0;

    // The current strategy, or nullptr when flat. A child-collection lambda checks it is still the
    // strategy that subscribed before routing a change to it.
    virtual SourceHierarchy* CurrentHierarchy() noexcept = 0;

    // Root-collection records, in source order.
    virtual std::vector<SourceRecordPtr> const& RootRecords() const noexcept = 0;

    // Records: create one (added to the lookups and, when live shaping is on, subscribed), or take
    // one out of the lookups before the caller destroys it. Release walks no children.
    virtual SourceRecordPtr CreateRecord(winrt::Windows::Foundation::IInspectable const& item) = 0;
    virtual void ReleaseRecord(SourceRecord* record) noexcept = 0;

    // SourceTable::ValidateIncoming: throws hresult_invalid_argument on a null, a duplicate within
    // items, or an item already in the table outside replacing and its subtree.
    virtual void ValidateIncoming(
        std::vector<winrt::Windows::Foundation::IInspectable> const& items,
        SourceRecord const* replacing) const = 0;

    // Shared tree operations (see SourceTable). SiblingsOf(nullptr) is the top level.
    virtual std::vector<SourceRecord*>& SiblingsOf(SourceRecord* parent) noexcept = 0;
    virtual void Reparent(SourceRecord* record, SourceRecord* newParent, uint32_t index) = 0;

    // Re-entrancy gate for child-collection notifications (#19); the same table as the root
    // collection (see SourceTable). isReset: a child Reset or a multi-item change reported as one.
    // True: apply positionally (an incremental change, table not frozen and not diverged). False:
    // the host has handled the notification and the strategy must not touch anything:
    //   - frozen, or held and (diverged or isReset): skipped (m_diverged and m_rereadDue set);
    //   - free and (diverged or isReset): the host re-reads now (Refresh()); a validation throw
    //     propagates into the app's Add / Insert / setter and the table stays diverged.
    virtual bool AdmitChange(bool isReset) = 0;

    // SourceTable::RaiseSourceChanged: raise the change inside a freeze, then destroy removed.
    virtual void RaiseSourceChanged(
        SourceChangeKind kind,
        SourceRecord* oldParent,
        SourceRecord* newParent,
        uint32_t oldIndex,
        uint32_t newIndex,
        std::vector<SourceRecord*> const& added,
        std::vector<SourceRecordPtr>& removed) = 0;

    // A child change was rejected (null/duplicate) after the child collection had already changed.
    // Decided (no subtree-scoped Reset): sets SourceTable::m_diverged (not m_rereadDue), exactly
    // as a rejected root change does, so the next notification (root or any child collection) is
    // handled as a whole-tree re-read (Refresh(): one Reset, unchanged records reused per feed)
    // until one validates. Raises nothing itself. Only for rejections: a child Reset or
    // multi-item change goes through AdmitChange(true).
    virtual void RequestReset() noexcept = 0;

    // A callable that throws hresult_wrong_thread when invoked off the table's bound thread (the
    // root collection's rule, SourceTable::ThrowIfOffBoundThread). Child-collection lambdas
    // capture it by value and call it before locking the host.
    virtual std::function<void()> MakeThreadCheck() const = 0;

    // SourceTable::DrainPendingChanges: the end of a child notification handler that applied a
    // change (never after AdmitChange returned false).
    virtual void DrainPendingChanges() = 0;

protected:
    ~ISourceHierarchyHost() = default;
};

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

    // Root-collection edits, after m_records changed. Decided (drift #8): every tree change other
    // than the edit itself is raised by the strategy as its own SourceChange (a Move, or the bulk
    // Reset when an insert adopts more than one orphan: spec Orphan adoption, Bulk threshold).
    // A raise can throw (listener fault); the caller posts the owed re-read and rethrows.
    //   OnInserted: record is in RootRecords() but not yet linked. ParentBy keys it and links it
    //     under its parent (or the top level) in source order, and remembers the top-level
    //     orphans waiting for its key; ChildrenBy links it at the top level and builds its feed
    //     and subtree. Returns false when the record stays unlinked (ParentBy: excluded, spec
    //     Misbehaving records), or when it adopted more than one orphan and raised the one bulk
    //     Reset itself: the caller raises no Add. A selector or key failure is captured
    //     in error (the first one only); the caller raises the change and then rethrows it.
    //   AfterInserted: after the caller raised the Add. ParentBy adopts the single remembered
    //     orphan as a Move from the top level to record. ChildrenBy: nothing.
    //   OnRemoving: record has left RootRecords() and the lookups but is still linked. ParentBy
    //     unkeys it and moves each of its children to the top level (one Move each, raised while
    //     record is still in the shaped tree); ChildrenBy moves its subtree out of the lookups
    //     into removed (pre-order, after record). Returns false when record was not linked
    //     (ParentBy: excluded): the caller raises no Remove. Otherwise the caller then reads
    //     record's position, unlinks it and raises the Remove.
    //   OnMoved: a root move, after m_records changed. The record keeps its parent; its sibling
    //     index follows source order. An unlinked (excluded) record is left alone.
    // A Replace under a hierarchy is RemoveAt then InsertAt (drift #16), so no hook covers it.
    virtual bool OnInserted(ISourceHierarchyHost& host, SourceRecord* record, std::exception_ptr& error) = 0;
    virtual void AfterInserted(ISourceHierarchyHost& host, SourceRecord* record) = 0;
    virtual bool OnRemoving(ISourceHierarchyHost& host, SourceRecord* record, std::vector<SourceRecordPtr>& removed) = 0;
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
    // child feed still held and its records (through host.ReleaseRecord), and drops the selectors
    // and keys. Teardown-safe: no app calls, revokes through safe_get. Idempotent.
    virtual void Release(ISourceHierarchyHost& host) noexcept = 0;

    uint64_t Generation() const noexcept { return m_generation; }

protected:
    SourceHierarchy() : m_generation(++s_lastGeneration) {}

private:
    // Unique per strategy: a stale event handler compares it, so a new strategy allocated at a
    // freed one's address is never mistaken for it. UI thread only.
    static inline uint64_t s_lastGeneration = 0;
    uint64_t const m_generation;
};

// Appends record and its source subtree, pre-order (SourceChange::Added for an insert).
void AppendSourceSubtree(SourceRecord* record, std::vector<SourceRecord*>& out);

// record's index in siblings, or siblings.size() when it is not there.
uint32_t IndexInSourceSiblings(std::vector<SourceRecord*> const& siblings, SourceRecord const* record) noexcept;

// ParentBy(keySelector, parentKeySelector). Every item is in the root collection; the tree is an
// index over Records(). Key rules follow v1 (Hierarchical-Data-Rows-Parent-Key-Design.md section 3):
// value keys compare by value, object keys by reference; a null/empty parent key or an orphan makes
// a top-level record. At the verb a null/empty key, a duplicate key, a self-parent or a cycle (found
// by counting records reachable from the top level) throws before any write; a Refresh() re-read
// and live edits are tolerant (drift #9, #11).
class ParentKeyHierarchy final : public SourceHierarchy
{
public:
    ParentKeyHierarchy(
        ITrackerHandleManager const* owner,
        winrt::TableViewKeySelector const& keySelector,
        winrt::TableViewKeySelector const& parentKeySelector);

    void Prepare(
        ISourceHierarchyHost& host,
        std::vector<winrt::Windows::Foundation::IInspectable> const& rootItems) override;
    void MatchForRefresh(
        ISourceHierarchyHost& host,
        std::vector<SourceRecord*> const& rootMatches,
        std::vector<SourceRecordPtr>& removed) override;
    void Commit(ISourceHierarchyHost& host, std::vector<SourceRecord*>& added) override;
    void CollectIncoming(
        winrt::Windows::Foundation::IInspectable const& item,
        std::vector<winrt::Windows::Foundation::IInspectable>& items) const override;
    bool OnInserted(ISourceHierarchyHost& host, SourceRecord* record, std::exception_ptr& error) override;
    void AfterInserted(ISourceHierarchyHost& host, SourceRecord* record) override;
    bool OnRemoving(ISourceHierarchyHost& host, SourceRecord* record, std::vector<SourceRecordPtr>& removed) override;
    void OnMoved(ISourceHierarchyHost& host, SourceRecord* record, uint32_t fromIndex, uint32_t toIndex) override;
    bool Reevaluate(ISourceHierarchyHost& host, SourceRecord* record, winrt::hstring const& propertyName) override;
    void TakeChildRecords(ISourceHierarchyHost& host, std::vector<SourceRecordPtr>& removed) override;
    void Release(ISourceHierarchyHost& host) noexcept override;
    void ForEachChildRecord(std::function<void(SourceRecordPtr const&)> const& visit) const override;

private:
    // Lookup form of a selector result (v1 MakeNodeKey): value types by value, objects by
    // reference. Empty for null.
    static winrt::hstring MakeNodeKey(winrt::Windows::Foundation::IInspectable const& key);

    // Counts records reachable from the top level of the prepared links; any unreached record is
    // on or below a cycle. Throws.
    void ValidateNoCycles(ISourceHierarchyHost const& host) const;

    // Each record's cached keys. Object keys are held while the tree is current so their addresses
    // cannot be reused (v1 section 3.1).
    struct NodeKeys
    {
        winrt::hstring Key;
        winrt::hstring ParentKey;
        // Object keys only (value keys need only Key); a strong reference like Item (drift #73).
        winrt::Windows::Foundation::IInspectable KeyObject{ nullptr };
    };

    // Placement after the verb (spec Misbehaving records; rulings 5 and 6). Each raises its own
    // SourceChange through the host.
    //   Keys: computes record's keys tolerantly. Returns false (record to be excluded) when the
    //     key selector throws, the key is null or empty, or another record holds it; a parent-key
    //     selector throw leaves ParentKey empty (top level), a self-parent stays keyed (FindParent
    //     puts it at the top level). The first error goes to error.
    //   PlaceParent: the parent record goes under (the holder of its parent key, or nullptr). When
    //     that would close a cycle (the parent is record or below it, or below one of the orphans
    //     record is about to adopt), record is forced to the top level (m_forcedTop) and the
    //     cycle error goes to error (spec: the edited record moves to the top level).
    //   MoveTo: relinks record (with its subtree) under newParent in source order and raises one
    //     Move; nothing when its place does not change.
    //   Exclude / Include: unlinks record (its children already moved to the top level) and
    //     raises its Remove; links it where PlaceParent says, raises its Add, and adopts the
    //     orphans waiting for its key.
    //   WaitingFor / Adopt: top-level records whose parent key is key; one is moved (Move), more
    //     than one are relinked and raised as one Reset (Bulk threshold).
    //   OrphanChildren: each child of record moves to the top level (one Move each, drift #10).
    //   RestoreForcedTop: a record forced to the top level moves to its proper parent once that
    //     no longer makes a cycle.
    //   Unkey: drops record's keys (and its key-index entry when it holds it).
    bool Keys(ISourceHierarchyHost& host, SourceRecord const* record, NodeKeys& keys, std::exception_ptr& error) const;
    SourceRecord* PlaceParent(SourceRecord* record, std::vector<SourceRecord*> const& orphans, std::exception_ptr& error);
    void MoveTo(ISourceHierarchyHost& host, SourceRecord* record, SourceRecord* newParent);
    void Exclude(ISourceHierarchyHost& host, SourceRecord* record);
    void Include(ISourceHierarchyHost& host, SourceRecord* record, std::exception_ptr& error);
    std::vector<SourceRecord*> WaitingFor(ISourceHierarchyHost& host, winrt::hstring const& key, SourceRecord const* record) const;
    void Adopt(ISourceHierarchyHost& host, SourceRecord* record, std::vector<SourceRecord*> const& orphans);
    void OrphanChildren(ISourceHierarchyHost& host, SourceRecord* record);
    void RestoreForcedTop(ISourceHierarchyHost& host);
    void Unkey(SourceRecord const* record);

    // App delegates, tracked (see SourceTable Reference tracking). Optional so Release() can
    // destroy them while the tracker owner is alive.
    std::optional<tracker_ref<winrt::TableViewKeySelector>> m_keySelector;
    std::optional<tracker_ref<winrt::TableViewKeySelector>> m_parentKeySelector;

    // Node key -> record, and record -> keys. Prepare() stages by root position (under Refresh()
    // the records are not matched yet); Commit() resolves each position through
    // host.RootRecords() and rebuilds these.
    std::unordered_map<winrt::hstring, SourceRecord*> m_keyIndex;
    std::unordered_map<SourceRecord const*, NodeKeys> m_nodeKeys;
    std::unordered_map<winrt::hstring, uint32_t> m_stagedKeyIndex;  // node key -> root position
    std::vector<NodeKeys> m_stagedNodeKeys;                         // by root position

    // Records in RootRecords() that are in no sibling list: no usable key (null or empty key, a
    // key another record holds, or a key selector that threw). They behave as filtered out (spec
    // Misbehaving records) and have no children (children attach to the key's holder). Re-checked
    // on PropertyChanged, Refresh() and the next verb.
    std::unordered_set<SourceRecord const*> m_excluded;

    // Records placed at the top level to break a cycle (a key edit, or a re-read that found one).
    std::unordered_set<SourceRecord*> m_forcedTop;

    // The orphans OnInserted found waiting for the new record's key; AfterInserted adopts them.
    std::vector<SourceRecord*> m_pendingAdoptions;

    // False until the first Commit: Prepare is strict at the verb (and for a parked verb), and
    // tolerant when Refresh() re-reads the committed strategy (drift #9).
    bool m_committed{ false };
};

// ChildrenBy(childrenSelector). Records() are the top level. Every other record is owned by its
// parent's ChildFeed (CollectionAccessor2 + event token + ordered records), one per non-null child
// collection, enumerated eagerly and recursively. A child added to any collection brings its whole
// subtree. Child collections are source: their changes are tracked whether or not live shaping is
// on, and go through the host's re-entrancy gate.
class ChildCollectionsHierarchy final : public SourceHierarchy
{
public:
    ChildCollectionsHierarchy(
        ITrackerHandleManager const* owner,
        winrt::TableViewChildrenSelector2 const& childrenSelector);

    void Prepare(
        ISourceHierarchyHost& host,
        std::vector<winrt::Windows::Foundation::IInspectable> const& rootItems) override;
    void MatchForRefresh(
        ISourceHierarchyHost& host,
        std::vector<SourceRecord*> const& rootMatches,
        std::vector<SourceRecordPtr>& removed) override;
    void Commit(ISourceHierarchyHost& host, std::vector<SourceRecord*>& added) override;
    void CollectIncoming(
        winrt::Windows::Foundation::IInspectable const& item,
        std::vector<winrt::Windows::Foundation::IInspectable>& items) const override;
    bool OnInserted(ISourceHierarchyHost& host, SourceRecord* record, std::exception_ptr& error) override;
    void AfterInserted(ISourceHierarchyHost& host, SourceRecord* record) override;
    bool OnRemoving(ISourceHierarchyHost& host, SourceRecord* record, std::vector<SourceRecordPtr>& removed) override;
    void OnMoved(ISourceHierarchyHost& host, SourceRecord* record, uint32_t fromIndex, uint32_t toIndex) override;
    bool Reevaluate(ISourceHierarchyHost& host, SourceRecord* record, winrt::hstring const& propertyName) override;
    void TakeChildRecords(ISourceHierarchyHost& host, std::vector<SourceRecordPtr>& removed) override;
    void Release(ISourceHierarchyHost& host) noexcept override;
    void ForEachChildRecord(std::function<void(SourceRecordPtr const&)> const& visit) const override;

    // Entry points for the child-collection lambdas, after they locked the host, checked
    // host.AdmitChange(isReset) and that host.CurrentHierarchy() == this. isReset is true for a
    // Reset, a multi-item change, and every IBindableObservableVector change (it carries no
    // detail). A rejected change calls host.RequestReset() and rethrows (into the app's Add /
    // Insert / setter).
    void OnChildCollectionChanged(ISourceHierarchyHost& host, SourceRecordWeakPtr const& owner, winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    void OnChildVectorChanged(ISourceHierarchyHost& host, SourceRecordWeakPtr const& owner, winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    void OnChildBindableVectorChanged(ISourceHierarchyHost& host, SourceRecordWeakPtr const& owner);

private:
    // One tracked child collection.
    struct ChildFeed
    {
        // The record whose children this feed holds. Weak; see SourceTable Lifetime.
        SourceRecordWeakPtr Owner;

        // Optional so Release() can destroy its tracker_refs while the tracker owner is alive.
        std::optional<CollectionAccessor2> Accessor;
        winrt::event_token EventToken{};

        // Sole strong references to this collection's records, in Owner->Children order.
        std::vector<SourceRecordPtr> Records;
    };

    // Evaluates the selector for item and, recursively, its descendants, enumerating each non-null
    // child collection into items (pre-order) without creating anything.
    void CollectSubtreeItems(
        winrt::Windows::Foundation::IInspectable const& item,
        std::vector<winrt::Windows::Foundation::IInspectable>& items) const;

    // Creates record's feed (if its collection is non-null), its child records and, recursively,
    // their subtrees, and links them through the host. Callers validate the whole subtree first
    // (CollectIncoming + ValidateIncoming). The selector runs again here (drift #15). A selector
    // that throws makes record a leaf (spec Misbehaving records); an item that now fails
    // validation is skipped and host.RequestReset() is called. The first error is kept in error
    // for the caller to rethrow.
    void BuildChildFeed(ISourceHierarchyHost& host, SourceRecordPtr const& record, std::exception_ptr& error);

    // Moves owner's feed out: unsubscribes it and moves its records and their subtrees out of
    // the lookups (host.ReleaseRecord) into removed, pre-order. No app calls.
    void TakeFeed(ISourceHierarchyHost& host, SourceRecord const* owner, std::vector<SourceRecordPtr>& removed);

    // One validated child Add / Remove in feed (owned by owner), raised as Add / Remove with
    // NewParent / OldParent = owner. InsertChild returns the first selector error (the tree is
    // consistent); the caller drains pending changes and then rethrows it.
    std::exception_ptr InsertChild(
        ISourceHierarchyHost& host,
        SourceRecordPtr const& owner,
        ChildFeed& feed,
        uint32_t index,
        winrt::Windows::Foundation::IInspectable const& item);
    void RemoveChild(ISourceHierarchyHost& host, SourceRecord* owner, ChildFeed& feed, uint32_t index);

    // Unsubscribes (safe_get), resets the accessor, releases the feed's records and their
    // subtrees through the host and destroys them. Teardown-safe.
    void DestroyChildFeed(ISourceHierarchyHost& host, SourceRecord const* owner) noexcept;

    // The lambda captures host.WeakHost() and the owner's weak_ptr.
    void SubscribeToChildChanges(ISourceHierarchyHost& host, ChildFeed& feed);
    void UnsubscribeFromChildChanges(ChildFeed& feed) noexcept;

    // App delegate, tracked. Optional so Release() can destroy it while the tracker owner is alive.
    std::optional<tracker_ref<winrt::TableViewChildrenSelector2>> m_childrenSelector;

    // One feed per record with a non-null child collection.
    std::unordered_map<SourceRecord const*, std::unique_ptr<ChildFeed>> m_childFeeds;

    // Prepare(): every item below the root items, pre-order, already validated, with the child
    // collection it came from, so Commit() creates and subscribes without calling app code again.
    // Plain strong references: scratch state that lives only between Prepare() and Commit() (or the
    // throw), inside one verb or one re-read.
    struct StagedNode
    {
        winrt::Windows::Foundation::IInspectable Item;
        winrt::Windows::Foundation::IInspectable ChildCollection;  // null: no feed
        uint32_t ChildCount{ 0 };                                  // nodes that follow, one level down
        SourceRecord* Match{ nullptr };                            // MatchForRefresh(): record kept for Item
    };
    std::vector<StagedNode> m_staged;
};
