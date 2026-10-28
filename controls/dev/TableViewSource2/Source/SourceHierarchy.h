// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
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
// (selectors, object keys, child accessors) are bound to host.TrackerOwner(), so Release() must run
// while that owner is alive (SourceTable::Clear() calls it first). Every app call (selectors,
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

    // Re-entrancy gate for child-collection notifications (#19). False while frozen or dispatching:
    // the host has set m_pendingReset and the strategy must not touch anything.
    virtual bool AdmitChange() noexcept = 0;

    // SourceTable::RaiseSourceChanged: raise (or queue) the change, then destroy removed.
    virtual void RaiseSourceChanged(
        SourceChangeKind kind,
        SourceRecord* oldParent,
        SourceRecord* newParent,
        uint32_t oldIndex,
        uint32_t newIndex,
        std::vector<SourceRecord*> const& added,
        std::vector<SourceRecordPtr>& removed) = 0;

    // A child change was rejected (null/duplicate) after the child collection had already changed.
    // Decided (no subtree-scoped Reset): sets SourceTable::m_outOfSync, exactly as a rejected root
    // change does, so the next notification (root or any child collection) is handled as a
    // whole-tree re-read (Refresh(): one Reset, unchanged records reused per feed) until one
    // validates. Raises nothing itself.
    virtual void RequestReset() noexcept = 0;

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

    // Root-collection edits, after m_records changed and before the SourceChange is raised. Each
    // fills the change's tree fields (parents, sibling indexes) and may add Moves to raise after it
    // (ParentBy orphan adoption; see the open batching item in SourceTable).
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

// ParentBy(keySelector, parentKeySelector). Every item is in the root collection; the tree is an
// index over Records(). Key rules follow v1 (Hierarchical-Data-Rows-Parent-Key-Design.md section 3):
// value keys compare by value, object keys by reference; a null/empty parent key or an orphan makes
// a top-level record; a null/empty key, a duplicate key, a self-parent or a cycle (found by counting
// records reachable from the top level) throws before any write.
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
    void OnInserted(ISourceHierarchyHost& host, SourceRecord* record) override;
    void OnRemoving(ISourceHierarchyHost& host, SourceRecord* record) noexcept override;
    void OnMoved(ISourceHierarchyHost& host, SourceRecord* record, uint32_t fromIndex, uint32_t toIndex) override;
    bool Reevaluate(ISourceHierarchyHost& host, SourceRecord* record, winrt::hstring const& propertyName) override;
    void TakeChildRecords(ISourceHierarchyHost& host, std::vector<SourceRecordPtr>& removed) override;
    void Release(ISourceHierarchyHost& host) noexcept override;

private:
    // Lookup form of a selector result (v1 MakeNodeKey): value types by value, objects by
    // reference. Empty for null.
    static winrt::hstring MakeNodeKey(winrt::Windows::Foundation::IInspectable const& key);

    // Counts records reachable from the top level of the prepared links; any unreached record is
    // on or below a cycle. Throws.
    void ValidateNoCycles(ISourceHierarchyHost const& host) const;

    // App delegates, tracked (see SourceTable Reference tracking). Optional so Release() can
    // destroy them while the tracker owner is alive.
    std::optional<tracker_ref<winrt::TableViewKeySelector>> m_keySelector;
    std::optional<tracker_ref<winrt::TableViewKeySelector>> m_parentKeySelector;

    // Each record's cached keys. Object keys are held while the tree is current so their addresses
    // cannot be reused (v1 section 3.1).
    struct NodeKeys
    {
        winrt::hstring Key;
        winrt::hstring ParentKey;
        // Object keys only (value keys need only Key); tracked like Item.
        std::optional<tracker_ref<winrt::Windows::Foundation::IInspectable>> KeyObject;
    };

    // Node key -> record, and record -> keys. Prepare() stages by root position (under Refresh()
    // the records are not matched yet); Commit() resolves each position through
    // host.RootRecords() and rebuilds these.
    std::unordered_map<winrt::hstring, SourceRecord*> m_keyIndex;
    std::unordered_map<SourceRecord const*, NodeKeys> m_nodeKeys;
    std::unordered_map<winrt::hstring, uint32_t> m_stagedKeyIndex;  // node key -> root position
    std::vector<NodeKeys> m_stagedNodeKeys;                         // by root position
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
    void OnInserted(ISourceHierarchyHost& host, SourceRecord* record) override;
    void OnRemoving(ISourceHierarchyHost& host, SourceRecord* record) noexcept override;
    void OnMoved(ISourceHierarchyHost& host, SourceRecord* record, uint32_t fromIndex, uint32_t toIndex) override;
    bool Reevaluate(ISourceHierarchyHost& host, SourceRecord* record, winrt::hstring const& propertyName) override;
    void TakeChildRecords(ISourceHierarchyHost& host, std::vector<SourceRecordPtr>& removed) override;
    void Release(ISourceHierarchyHost& host) noexcept override;

    // Entry points for the child-collection lambdas, after they locked the host, checked
    // host.AdmitChange() and that host.CurrentHierarchy() == this. A rejected change calls
    // host.RequestReset() and rethrows (into the app's Add / Insert / setter).
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
    // their subtrees, and links them through the host. Callers validate the whole subtree first.
    void BuildChildFeed(ISourceHierarchyHost& host, SourceRecord* record);

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
