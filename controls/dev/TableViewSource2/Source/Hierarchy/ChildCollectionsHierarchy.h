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

#include "Source/CollectionAccessor2.h"
#include "Source/SourceRecord.h"
#include "SourceHierarchy.h"

// ChildrenBy strategy (declarations only); see Source/Hierarchy/SourceHierarchy.h.

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
