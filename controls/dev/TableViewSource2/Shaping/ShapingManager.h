// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include "pch.h"
#include "common.h"

#include "Source/SourceTable.h"
#include "Engine/ShapingEngine.h"
#include "Spec/ShapeSpec.h"
#include "Spec/ShapeDelegates.h"
#include "Spec/PathEvaluator.h"
#include "View/Flattener.h"
#include "View/GroupExpansion.h"

// The shaping coordinator (declarations only). See
// docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "6. Coordinator".
//
// Owns every shaping part behind TableViewSource2 and wires them together. TableViewSource2 stays
// a thin WinRT shell: it validates verb arguments and forwards here.
//
//   Owns:  SourceTable (records and source tree), the desired ShapeSpec, ShapeDelegates,
//          PathEvaluator, the ItemsSourceView handed to TableView; later the engine (applied spec,
//          shaped tree), the flattener and the visible list.
//   Runs:  verb -> Diff(applied, desired) -> engine apply -> flat delta -> publish to the
//          visible list. Local changes go engine -> IShapedEditSink (m_flattener) as ShapedEdits;
//          reshapes go through Reapply() and publish a stable Reset ("Change pipeline").
//   Hears: SourceTable::SourceChanged and ItemChanged; it is their only subscriber.
//
// Unlike v1 ShapedItemsSource it is plain C++, not the WinRT collection the repeater binds to.
// That collection (the visible list) is not declared yet; View() wraps it in an ItemsSourceView.
//
// Reference tracking: holds no app object itself. SourceTable, ShapeDelegates and m_view bind their
// tracker_refs to m_trackerOwner (the owning TableViewSource2). Clear() must run while that owner
// is alive; ~TableViewSource2() calls it.
//
// Threading: UI-thread affine, like SourceTable.
class ShapingManager
{
public:
    // Creates the SourceTable over `items` (SourceTable::Create, initial load) and installs this
    // object as its SourceChanged and ItemChanged handler. Must be called on the UI thread.
    ShapingManager(ITrackerHandleManager const* trackerOwner, winrt::IInspectable const& items);
    ~ShapingManager();

    ShapingManager(ShapingManager const&) = delete;
    ShapingManager& operator=(ShapingManager const&) = delete;

    // ---- Spec verbs: edit m_desired, then apply synchronously ------------------------------
    // Arguments are already validated by TableViewSource2. App-code exceptions propagate (see
    // "Misbehaving records"). Called while an apply runs: throws InvalidOperationException.

    void Filter(winrt::TableViewPredicate2 const& predicate);
    void GroupBy(winrt::TableViewKeySelector2 const& key, winrt::SortDirection2 groupDirection);
    void Sort(winrt::hstring const& sortMemberPath, winrt::SortDirection2 direction);
    void Sort(winrt::TableViewKeySelector2 const& key, winrt::SortDirection2 direction);
    void Sort(winrt::TableViewComparer2 const& comparer);
    void ClearFilter();
    void ClearGroupBy();
    void ClearSort();

    // ---- Hierarchy verbs: forward to SourceTable, which raises one Reset -------------------

    void ParentBy(winrt::TableViewKeySelector2 const& key, winrt::TableViewKeySelector2 const& parentKey);
    void ChildrenBy(winrt::TableViewChildrenSelector2 const& childrenSelector);
    void ClearHierarchy();

    // ---- Source -----------------------------------------------------------------------------

    // The only path that re-enumerates the source on request. Calls SourceTable::Refresh(), whose
    // Reset comes back through OnSourceChanged and runs Reapply() once.
    void Refresh();

    bool IsLiveShaping() const;
    void IsLiveShaping(bool value);

    // ---- Output -----------------------------------------------------------------------------

    // The ItemsSourceView TableView binds to (as v1 TableViewSource::GetItemsSourceView()). It
    // wraps the visible list, is created on first call and cached, and stays the same object for
    // this coordinator's lifetime: reshapes raise CollectionChanged on it, never replace it.
    // Returns nullptr after Clear().
    winrt::ItemsSourceView View();

    // ---- Lifetime ---------------------------------------------------------------------------

    // Teardown: Flattener::Clear() first (it holds raw record pointers), then ShapingEngine::Clear()
    // (drops groups and tracked keys), then SourceTable::Clear()
    // (detach, handlers nulled, records destroyed), ShapeDelegates::Clear(),
    // PathEvaluator::Clear(), m_view.reset(). Must run on the UI thread while m_trackerOwner is alive. Idempotent.
    void Clear();

private:
    // SourceTable callbacks.
    void OnSourceChanged(SourceChange const& change);
    // Live shaping (decided, E7: coalesce per dispatcher turn). Never shapes inside the app's
    // setter: adds (record, propertyName) to m_liveDirty (a second, different name for the same
    // record becomes the empty name, "re-read everything") and posts one DrainLiveChanges.
    // record == nullptr (a dropped off-thread change) sets m_liveAllDirty instead. Ignored when no
    // verb is active.
    void OnItemChanged(SourceRecord* record, winrt::hstring const& propertyName);

    // Posts DrainLiveChanges to the current DispatcherQueue unless one is already posted
    // (m_liveDrainPosted). The callback holds a weak reference to the owning TableViewSource2 and
    // returns if it is gone.
    void PostLiveDrain();

    // The posted callback, and the synchronous flush before a verb. One commit, one Publish:
    //   m_liveAllDirty           -> Reapply();
    //   one entry                -> m_engine.ApplyItemChanged (surgical);
    //   more than c_bulkThreshold -> m_engine.ApplyItemsChanged (re-read dirty records only, then
    //                               one structural rebuild and a stable Reset).
    // Empties m_liveDirty and clears both flags first. Skipped while the owner holds live shaping
    // (an in-progress cell edit, v1 m_liveShapingHold); the hook is declared with #18 (owner
    // notifications), and releasing the hold calls PostLiveDrain.
    void DrainLiveChanges();

    // Applies Diff(applied, desired); then Retain()s the delegates and paths both specs use.
    void ApplyDesiredSpec();

    // Rebuilds the shaped tree from SourceTable::Records() with the desired spec. Never reads
    // the source. Used for a SourceTable Reset, a Diff that falls back to a full rebuild, and
    // dropped-ItemChanged recovery.
    void Reapply();

    // The single path from a FlatDelta to consumers, whatever produced it (an engine commit,
    // Expand / Collapse, or an expand-all verb). Decided (V4):
    //   - Surgical: for each splice in order, Flattener::ApplySplice, then the visible list raises
    //     a ranged Remove (if RemovedCount) and a ranged Add (if Inserted is non-empty). Item lists
    //     are size-only placeholders; Replace is not used.
    //   - Bulk: one Reset.
    //   - Then RowStateEdits to the owner, then ShapingEngine::ReleaseRetired.
    // Runs inside the #19 apply scope: a handler that calls a verb throws. An empty delta raises
    // nothing.
    void Publish(FlatDelta const& delta);

    ITrackerHandleManager const* m_trackerOwner{ nullptr };

    // Shared because consumers may hold it past this object's lifetime; Clear() detaches it.
    std::shared_ptr<SourceTable> m_sourceTable;

    // What the verbs have declared. The applied copy lives in the engine (Phase 4).
    ShapeSpec m_desired;
    ShapeDelegates m_delegates;
    PathEvaluator m_paths;

    // Cached result of View(). std::optional because ~tracker_ref dereferences the owner; Clear()
    // resets it while the owner is alive.
    std::optional<tracker_ref<winrt::ItemsSourceView>> m_view;

    // A source notification with more than this many items (a range event, or an insert adopting
    // several orphans under ParentBy) goes through Reapply() and one stable Reset instead of
    // ShapedEdits. Descendants carried by an item do not count. 1 matches v1 ShapedItemsSource.
    static constexpr uint32_t c_bulkThreshold = 1;

    // Re-entrancy guard: a verb or Refresh while an apply runs throws.
    bool m_isApplying{ false };

    // Source change during an apply (decided, #19: freeze, then one deferred Reset). Every engine
    // entry point and its Publish run inside one scope: m_isApplying, SourceTable::BeginFreeze,
    // Flattener::BeginCommit, the engine call, EndCommit -> Publish, EndFreeze. A source change
    // raised by app code inside that scope (a predicate, comparer or key selector, or a binding,
    // converter or element factory during Publish) is not applied to the table, so records and
    // links never change under the engine or the flattener. When EndFreeze reports a pending
    // change, ResyncSource runs SourceTable::Refresh(), whose Reset goes to Reapply().
    // It runs synchronously once; it is posted instead when the scope ended by an exception, or
    // when the resync's own apply was disturbed again (one resync per turn, so an app that mutates
    // on every callback cannot loop).
    void ResyncSource(bool post);
    bool m_resyncPosted{ false };

    // Live-shaping changes pending for the next DrainLiveChanges (E7). Keys are raw record
    // pointers: OnSourceChanged erases removed records and their descendants before the engine
    // applies the change, a SourceTable Reset or any Reapply empties the map (it re-reads every
    // record), and a verb drains it synchronously before applying, so a verb never runs over stale
    // keys. Clear() empties it, so a drain posted before teardown is a no-op.
    std::unordered_map<SourceRecord*, winrt::hstring> m_liveDirty;
    bool m_liveAllDirty{ false };     // ItemChanged(nullptr): the drain does Reapply()
    bool m_liveDrainPosted{ false };

    // Group expansion, keyed by GroupNode::Identity. Read and written by m_flattener. Pruned
    // (decided, F4): after every engine commit whose delta is IsReset, this calls
    // RetainOnly(m_engine.GroupsByIdentity()); the flattener Erases on a surgical group drop;
    // cleared when GroupBy gets a new selector. Declared before m_flattener, which references it.
    GroupExpansion m_groupExpansion;

    // Shaped tree -> visible list; the engine's IShapedEditSink.
    Flattener m_flattener{ m_groupExpansion };

    // Source tree + applied spec -> shaped tree. Declared after m_flattener, its IShapedEditSink.
    // Every entry point is bracketed here: m_flattener.BeginCommit(), the engine call,
    // EndCommit() -> Publish, with a scope that calls AbortCommit() if the call throws.
    // Built in the constructor's member-init list after m_sourceTable is created (it binds to
    // *m_sourceTable), with m_trackerOwner, m_delegates, m_paths and m_flattener.
    ShapingEngine m_engine;

    // Not declared yet: the visible list (the WinRT collection).
};
