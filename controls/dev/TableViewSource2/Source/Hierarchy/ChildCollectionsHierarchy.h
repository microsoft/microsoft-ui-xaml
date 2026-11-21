// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <winrt/Microsoft.UI.Xaml.Interop.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>

#include "Source/CollectionAccessor2.h"
#include "Source/SourceRecord.h"
#include "SourceHierarchy.h"

// ChildrenBy strategy; see Source/Hierarchy/SourceHierarchy.h.

// ChildrenBy(childrenSelector). Records() are the top level. Every other record is owned by its
// parent's ChildFeed (CollectionAccessor2 + event token + ordered records), one per non-null child
// collection, enumerated eagerly. A child added to any collection brings its whole subtree. Child
// collections are source: their changes are tracked whether or not live shaping is on, and go
// through the host's re-entrancy gate.
//
// Decided:
//   C1: teardown (DestroyChildFeed, Release) never calls host.ReleaseRecord; removal outside
//       teardown goes through TakeFeed, which does.
//   C2: a child-collection handler checks the bound thread, locks the host and compares
//       CurrentHierarchy() and Generation(); each entry point classifies the change against its
//       feed and calls host.AdmitChange(isReset); false means touch nothing.
//   C3: positional only for a single Add, a single matching Remove and a single Replace;
//       everything else is a whole-tree re-read. Commit() rebinds every feed.
//   C4: Reevaluate is a no-op (a children-property swap is picked up at Refresh).
//   A8: the selector's collections are enumerated through the static CollectionAccessor2::
//       Enumerate (no tracker handles); only a ChildFeed's accessor is kept.
//   H1: every subtree is read once (StagedNode) and built from that read.
//   H2: every tree walk is iterative (explicit stack); no recursion on tree depth.
//   H3: feeds subscribe and revoke through CollectionAccessor2::Subscribe / Unsubscribe.
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
        std::vector<winrt::Windows::Foundation::IInspectable>& items) override;
    void DiscardIncoming() noexcept override;
    void OnInserted(ISourceHierarchyHost& host, SourceRecord* record) override;
    void OnRemoving(ISourceHierarchyHost& host, SourceRecord* record, std::vector<SourceRecordPtr>& removed) override;
    void OnMoved(ISourceHierarchyHost& host, SourceRecord* record, uint32_t fromIndex, uint32_t toIndex) override;
    void ForEachChildRecord(std::function<void(SourceRecordPtr const&)> const& visit) const override;
    bool Reevaluate(ISourceHierarchyHost& host, SourceRecord* record, winrt::hstring const& propertyName) override;
    void TakeChildRecords(ISourceHierarchyHost& host, std::vector<SourceRecordPtr>& removed) override;
    void Release(ISourceHierarchyHost& host) noexcept override;

    // Entry points for the child-collection handlers, called after the handler checked the bound
    // thread, locked the host and found this strategy current (C2). Each classifies the change
    // against owner's feed and calls host.AdmitChange(isReset) itself; a positional change (C3)
    // is applied through InsertChild / RemoveChild / ReplaceChild, everything else (and every
    // IBindableObservableVector change, which carries no detail) re-reads the whole tree. Drains
    // the host's pending changes before rethrowing a selector error. A rejected incoming item
    // calls host.RequestReset() and rethrows (into the app's Add / Insert / setter).
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

    // One item of a subtree read (H1), pre-order: an item's children follow it, each followed by
    // its own subtree. Plain strong references: scratch state that lives only until the build (or
    // the throw), inside one verb, one re-read or one change.
    struct StagedNode
    {
        winrt::Windows::Foundation::IInspectable Item;
        winrt::Windows::Foundation::IInspectable ChildCollection;  // null: no feed
        uint32_t SubtreeSize{ 0 };                                 // descendants that follow
        SourceRecord* Match{ nullptr };                            // MatchForRefresh(): record kept for Item
    };

    // Appends item and its subtree to nodes, pre-order, iteratively (H2). Calls the selector once
    // per item and enumerates each non-null collection through the static Enumerate (A8). An
    // object is expanded once: seen holds the addresses already in the read (the caller adds
    // item's, and Prepare every root item's), so a collection that contains an ancestor ends.
    // With selectorError null (Prepare) a selector or enumeration throw propagates and a null or
    // repeated child throws hresult_invalid_argument. Otherwise the first throw is stored there and
    // the item becomes a leaf, and a null or repeated child is kept as a leaf for the caller's
    // ValidateIncoming to reject (C3, incremental).
    void ReadSubtree(
        winrt::Windows::Foundation::IInspectable const& item,
        std::vector<StagedNode>& nodes,
        std::unordered_set<ObjectAddress>& seen,
        std::exception_ptr* selectorError) const;

    // Reads item's subtree for an incremental insert (selector errors stored, not thrown).
    std::vector<StagedNode> ReadIncoming(
        winrt::Windows::Foundation::IInspectable const& item,
        std::exception_ptr& selectorError) const;

    // Appends nodes[first + 1 .. first + nodes[first].SubtreeSize] items (the descendants of
    // nodes[first]), pre-order.
    static void AppendStagedItems(
        std::vector<StagedNode> const& nodes,
        size_t first,
        std::vector<winrt::Windows::Foundation::IInspectable>& items);

    // Builds record's subtree from nodes[first] (whose Item is record->Item), iteratively (H2):
    // one feed per non-null ChildCollection (subscribed; an existing feed of the same owner is
    // rebound and its records pooled), each child reusing its Match from that pool or created
    // through the host, and linked directly (Parent, Children, Depth). Appends every record it
    // creates to added, pre-order. No app calls besides the subscriptions; may throw on allocation.
    void BuildChildFeed(
        ISourceHierarchyHost& host,
        SourceRecordPtr const& record,
        std::vector<StagedNode> const& nodes,
        size_t first,
        std::vector<SourceRecord*>& added);

    // Positional child edits (C3), after AdmitChange(false). InsertChild reads and validates item's
    // subtree, writes nothing and calls host.RequestReset() before rethrowing on rejection,
    // otherwise inserts, builds, links and raises one Add. ReplaceChild validates against the
    // leaving record and its subtree (same rejection rule), then RemoveChild + insert from the same
    // read (the view sees Remove + Add; the new row is collapsed). Both return the stored selector
    // error for the caller to rethrow after DrainPendingChanges.
    std::exception_ptr InsertChild(
        ISourceHierarchyHost& host,
        SourceRecordPtr const& owner,
        ChildFeed& feed,
        uint32_t index,
        winrt::Windows::Foundation::IInspectable const& item);
    void RemoveChild(ISourceHierarchyHost& host, SourceRecord* owner, ChildFeed& feed, uint32_t index);
    std::exception_ptr ReplaceChild(
        ISourceHierarchyHost& host,
        SourceRecordPtr const& owner,
        ChildFeed& feed,
        uint32_t index,
        winrt::Windows::Foundation::IInspectable const& item);

    // Inserts the already read and validated subtree nodes[0] at index in feed and raises the Add.
    void InsertStaged(
        ISourceHierarchyHost& host,
        SourceRecordPtr const& owner,
        ChildFeed& feed,
        uint32_t index,
        std::vector<StagedNode> const& nodes);

    // Non-teardown removal (C1): erases owner's feed and every feed below it, unsubscribes each,
    // and moves every record under owner out of the lookups (host.ReleaseRecord) into removed,
    // pre-order. Iterative (H2). No app calls.
    void TakeFeed(ISourceHierarchyHost& host, SourceRecord const* owner, std::vector<SourceRecordPtr>& removed);

    // Erases owner's feed only, unsubscribing through safe_get, without host.ReleaseRecord (C1).
    // For Commit() when owner's collection became null: MatchForRefresh() has already released
    // every record of that feed and erased the feeds below them (asserted).
    void DestroyChildFeed(SourceRecord const* owner) noexcept;

    // The handlers capture host.WeakHost(), host.Binding(), Generation() and the owner's weak_ptr.
    // A collection that refuses the handler stays unobserved.
    void SubscribeToChildChanges(ISourceHierarchyHost& host, ChildFeed& feed);

    // App delegate, tracked. Optional so Release() can destroy it while the tracker owner is alive.
    std::optional<tracker_ref<winrt::TableViewChildrenSelector2>> m_childrenSelector;

    // One feed per record with a non-null child collection.
    std::unordered_map<SourceRecord const*, std::unique_ptr<ChildFeed>> m_childFeeds;

    // Prepare(): every root item's subtree, root items in root order, each followed by its subtree.
    std::vector<StagedNode> m_staged;

    // CollectIncoming(): the incoming root item's subtree read (H1/H5), and the selector error
    // OnInserted() rethrows after the Add. Cleared by OnInserted() and DiscardIncoming().
    std::vector<StagedNode> m_incoming;
    std::exception_ptr m_incomingError;
};