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
// of rows — header, items, header, items — held in an ordinary IObservableVector. That vector
// can then be wrapped in an ItemsSourceView and handed to the ItemsRepeater by the control. 
class GroupedSourceAdapter : public std::enable_shared_from_this<GroupedSourceAdapter>
{
public:
    // UI-thread-affine: construct on a UI thread with a DispatcherQueue. Source notifications are
    // required to arrive on this thread and are applied synchronously; the queue is captured only
    // to assert that affinity in chk. Constructing off a UI thread throws RPC_E_WRONG_THREAD.
    GroupedSourceAdapter();
    ~GroupedSourceAdapter();

    // The flat row axis the repeater consumes: a single ItemsSourceView wrapping the materialized
    // observable vector. It is created ONCE over m_entries (whose object identity is stable for the
    // adapter's lifetime — Rebuild ReplaceAll's its contents and a single-group toggle splices them
    // in place, but the vector instance never changes), so the identity a consumer captures stays
    // valid across rebuilds — the same contract the computed adapter's Entries() had. Because the
    // wrapped object is a plain IObservableVector, the repeater reaches it through
    // InspectingDataSource, which reports no key mapping — this is the container-preservation cost
    // of this design.
    winrt::ItemsSourceView Entries() const { return m_entriesView; }

    winrt::IInspectable Source() const { return m_source; }
    void Source(winrt::IInspectable const& value);
    void DetachSourceQuietly();

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
    void OnExpansionChanged(ShapingHelpers::RowExpansionModel::Change const& change);

    // Incremental expand/collapse of a SINGLE group: flips the group's header slot and splices just
    // that group's data rows into/out of the flat projection, instead of dropping and re-realizing
    // every container via a full Rebuild + Reset. Returns false when the group/header can't be
    // resolved incrementally (mid-reshape, header not materialized, count desync); the caller then
    // falls back to Rebuild, which is authoritative. Bulk changes (ExpandAll/CollapseAll, baseline
    // moves) never take this path — they stay a single Reset.
    bool TryApplyExpansionSplice(winrt::hstring const& intentKey, bool expand);

    // A single row inserted into or removed from one group: re-mints that group's header with the
    // new count and splices the row, instead of a full Rebuild + Reset. Returns false, having changed
    // nothing, when the change cannot be resolved that way; the caller then rebuilds.
    void OnGroupItemsChanged(
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const& items,
        winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    bool TryApplyGroupItemsSplice(
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const& items,
        winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    void EnsureGroupHeaderSlots();

    void AttachToSource();
    void DetachFromSource();
    void SubscribeToGroup(winrt::IInspectable const& group);
    void UnsubscribeFromAllGroups();

    static winrt::hstring GetGroupIntentKey(winrt::IInspectable const& group);

    winrt::IInspectable m_source{ nullptr };

    // THE flat projection. Materialized in full on every Rebuild via a single ReplaceAll; a single-
    // group expand/collapse splices that group's rows in place (SetAt/InsertAt/RemoveAt).
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_entries{
        winrt::single_threaded_observable_vector<winrt::IInspectable>() };

    // One ItemsSourceView over m_entries, created once (m_entries identity is stable for life).
    winrt::ItemsSourceView m_entriesView{ nullptr };

    // Expand/collapse intent, keyed by group identity string so it survives reshapes.
    ShapingHelpers::RowExpansionModel m_expansion;

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
        winrt::IInspectable ItemsForRevocation{ nullptr };
        winrt::event_token CollectionToken{};
        winrt::event_token Token{};
        winrt::event_token BindableToken{};
    };
    std::vector<InnerGroupSubscription> m_innerSubscriptions;

    // Where each group's header sits in m_entries, in group order, so a change inside one group is
    // located without scanning every row. Recomputed from m_entries after an expansion splice.
    struct GroupHeaderSlot
    {
        winrt::IInspectable Group{ nullptr };
        winrt::IInspectable Items{ nullptr };
        // Canonical IUnknown of Items, which keeps it alive.
        void* ItemsIdentity{};
        uint32_t EntryIndex{};
    };
    std::vector<GroupHeaderSlot> m_groupHeaderSlots;
    bool m_groupHeaderSlotsValid{ false };

    // Simple re-entrancy guard: a change notification raised synchronously inside ReplaceAll must
    // not start a nested Rebuild. The re-entrant request is remembered and run once after unwind.
    // Plain bool, not atomic: the adapter is UI-thread-affine and every mutation asserts that.
    bool m_rebuildInFlight{ false };
    bool m_pendingRebuild{ false };
};

using GroupedSourceAdapterPtr = std::shared_ptr<GroupedSourceAdapter>;
