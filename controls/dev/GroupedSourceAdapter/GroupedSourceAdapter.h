// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "pch.h"
#include "common.h"

#include <winrt/Microsoft.UI.Dispatching.h>

#include "RowExpansionModel.h"

// Single-responsibility grouped adapter.
//
// The adapter has ONE job: take the shaped grouped source and MATERIALIZE it into one flat list
// of rows — header, items, header, items — held in an ordinary IObservableVector. That vector is
// then wrapped in an ItemsSourceView and handed to the ItemsRepeater. There is no
// HierarchicalItemsSourceView, no GroupRunTable, no computed/virtualized axis, and no per-row key
// surface.
//
// What it deliberately drops relative to a computed adapter, and why the drop is the
// whole point:
//
//   * No lazy row minting. Rebuild walks every group and pushes every visible row up front, so a
//     grouped bind allocates one header object per group plus (for expanded groups) nothing per
//     data row — data rows are the raw app items, so they cost only the vector slot. Header count
//     is O(groups); the memory that scared the original change (a wrapper per data row) is already
//     gone because data rows are no longer wrapped.
//
//   * No fast paths. Every source change, every inner-group change, and every expand/collapse is a
//     full Rebuild that ends in one ReplaceAll — i.e. one Reset into the repeater. That is the
//     simplification: the ranged-splice machinery (TryApplyExpansionSplice,
//     TryApplyIncrementalInnerGroupChangeIncc, the run table that computed the affected range) is
//     gone. The cost is that a toggle or a single-item edit drops and re-realizes every container,
//     and the scroll/selection/focus that rode on them. Acceptable only if v1 scale is small.
//
//   * No IKeyIndexMapping / KeyFromIndex. Because Entries() is a plain IObservableVector, the
//     repeater reaches it through InspectingDataSource, which reports HasKeyIndexMapping only for a
//     source that implements IKeyIndexMapping. This adapter does not, so container preservation
//     across a reshape falls back to index-based reuse — a sort or filter re-templates rows rather
//     than key-rematching them. (The computed view implemented IItemsSourceView directly precisely
//     to keep that surface.)
//
// What it KEEPS, because these are orthogonal to how rows are stored:
//
//   * Expansion INTENT (RowExpansionModel), keyed by the group's stable string identity so it
//     survives group objects being re-minted by each reshape. A change here just triggers Rebuild.
//   * Source and inner-group subscription with UI-thread affinity and GC-safe (weak) callbacks.
//   * Empty groups are preserved (app-authored structure), matching the current adapter.
class GroupedSourceAdapter : public std::enable_shared_from_this<GroupedSourceAdapter>
{
public:
    // UI-thread-affine: construct on a UI thread with a DispatcherQueue. Source notifications are
    // required to arrive on this thread and are applied synchronously; the queue is captured only
    // to assert that affinity in chk. Constructing off a UI thread throws RPC_E_WRONG_THREAD.
    GroupedSourceAdapter();
    ~GroupedSourceAdapter();

    // The flat row axis the repeater consumes: a single ItemsSourceView wrapping the materialized
    // observable vector. It is created ONCE over m_entries (which is stable for the adapter's
    // lifetime and only ever ReplaceAll'd), so the identity a consumer captures stays valid across
    // rebuilds — the same contract the computed adapter's Entries() had. Because the wrapped object
    // is a plain IObservableVector, the repeater reaches it through InspectingDataSource, which
    // reports no key mapping — this is the container-preservation cost of this design.
    winrt::ItemsSourceView Entries() const { return m_entriesView; }

    winrt::IInspectable Source() const { return m_source; }
    void Source(winrt::IInspectable const& value);
    void DetachSourceQuietly();

    bool IncludeGroupHeaders() const { return m_includeGroupHeaders; }
    void IncludeGroupHeaders(bool value);

    // Per-group expand / collapse intent. The group OBJECT is translated to the model's stable
    // string key here; the model raises Changed, which rebuilds.
    bool IsGroupExpanded(winrt::IInspectable const& group);
    void SetGroupExpanded(winrt::IInspectable const& group, bool isExpanded);
    void ExpandAll();
    void CollapseAll();

    // The live group object for a declared key, so a control holding only a key can address a group.
    winrt::IInspectable ResolveLiveGroupByIdentity(winrt::hstring const& groupKey) const;

private:
    void Rebuild();
    bool OnUiThread() const;
    void AssertRebuildOnUiThread() const;
    void OnExpansionChanged(TabularShapingHelpers::RowExpansionModel::Change const& change);

    void AttachToSource();
    void DetachFromSource();
    void SubscribeToGroup(winrt::IInspectable const& group);
    void UnsubscribeFromAllGroups();

    static winrt::hstring GetGroupIntentKey(winrt::IInspectable const& group);

    winrt::IInspectable m_source{ nullptr };
    bool m_includeGroupHeaders{ true };

    // THE flat projection. Materialized in full on every Rebuild via a single ReplaceAll.
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_entries{
        winrt::single_threaded_observable_vector<winrt::IInspectable>() };

    // One ItemsSourceView over m_entries, created once (m_entries identity is stable for life).
    winrt::ItemsSourceView m_entriesView{ nullptr };

    // Expand/collapse intent, keyed by group identity string so it survives reshapes.
    TabularShapingHelpers::RowExpansionModel m_expansion;

    // Key->group map of the last Rebuild's live groups, for ResolveLiveGroupByIdentity.
    std::unordered_map<winrt::hstring, winrt::IInspectable> m_liveGroupsByIdentityKey;
    std::unordered_set<winrt::hstring> m_ambiguousLiveGroupIdentityKeys;

    // Affinity assertion only (chk); the adapter never marshals. Teardown runs on the owning UI
    // thread because every strong owner (TableViewSource / TableView) is a ReferenceTracker whose
    // final_release marshals destruction there, so no revoke thread guard is needed.
    winrt::weak_ref<winrt::Microsoft::UI::Dispatching::DispatcherQueue> m_uiQueue{ nullptr };

    winrt::IInspectable m_attachedSourceForRevocation{ nullptr };
    winrt::event_token m_outerCollectionChangedToken{};
    winrt::event_token m_outerVectorChangedToken{};
    winrt::event_token m_outerBindableVectorChangedToken{};

    struct InnerGroupSubscription
    {
        winrt::IInspectable GroupForRevocation{ nullptr };
        winrt::event_token CollectionToken{};
        winrt::event_token Token{};
        winrt::event_token BindableToken{};
    };
    std::vector<InnerGroupSubscription> m_innerSubscriptions;

    // Simple re-entrancy guard: a change notification raised synchronously inside ReplaceAll must
    // not start a nested Rebuild. The re-entrant request is remembered and run once after unwind.
    // Plain bool, not atomic: the adapter is UI-thread-affine and every mutation asserts that.
    bool m_rebuildInFlight{ false };
    bool m_pendingRebuild{ false };
};

using GroupedSourceAdapterPtr = std::shared_ptr<GroupedSourceAdapter>;
