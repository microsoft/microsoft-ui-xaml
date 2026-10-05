// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "pch.h"
#include "common.h"

#include "ShapingPipeline.h"
#include "ShapingHelpers.h"
#include "RowIdentity.h"
#include "ParentKeyIndex.h"

class GroupedSourceAdapter;
class HierarchicalSourceAdapter;
class ShapedGroup;

// Layer 2 of the shaping stack: the LIVE PROJECTION.
//
// Layer 1 decides what a shape is and how to apply it to a vector of items. This class owns the
// shape that currently EXISTS -- the projected rows, the group buckets behind them, the identity
// index that makes a change locatable, and the subscriptions that keep all of it true as the
// underlying source mutates. It is the difference between "sort these items" and "stay sorted".
//
// It deliberately knows nothing about a control. It produces vectors and reports what kind of
// projection it produced; an owner above decides what a row means, how to present it, and what
// to cache against it. Everything that used to make this logic control-specific -- constructing
// an ItemsSourceView, minting a row-metadata provider, notifying a TableView that its cached
// projection went stale -- is now delivered through the three handlers below, so the same engine
// can back any consumer.
//
// Threading: UI-thread-affine after construction, the same contract every XAML items source has
// (ItemsRepeater's InspectingDataSource makes no thread check either). Shaping verbs, source
// notifications and projection mutation must all run on the owning thread. A source that raises
// change notifications from a background thread is app misuse and is not supported; no attempt is
// made to marshal. Marshaling was tried and removed -- deferring to a rebuild that re-reads the
// app's collection from the UI thread leaves the app's own collection racing anyway, so it bought
// an illusion of safety while forcing a silent drop path and an exception swallow.
class ShapedItemsSource : public std::enable_shared_from_this<ShapedItemsSource>
{
public:
    // What the last rebuild actually produced -- the EFFECTIVE shape, not the requested one.
    // Consumers read this to decide how to interpret a row, so it must never report intent.
    //
    // Only ONE shaping request degrades: a source with no usable ROW identity degrades to
    // Unshaped, a plain 1:1 mirror (RebuildUnshapedRows). A GROUPING request does NOT degrade --
    // an unresolvable, unstable or colliding group identity throws hresult_invalid_argument out of
    // RebuildGroupedRows instead of quietly producing Flat. The asymmetry is deliberate: a row
    // identity the engine cannot derive is a property of the app's data that the app may not be
    // able to change, and an unshaped mirror still shows every row; a bad group identity comes
    // from the GroupBy(...) selector the app just wrote, and silently rendering ungrouped is a bug
    // an app ships without ever noticing.
    enum class ProjectionKind
    {
        // No projection has been built yet.
        None,
        // A 1:1 mirror of the source, no shaping applied.
        Unshaped,
        // Filtered and/or sorted rows -- raw items, no group headers.
        Flat,
        // Group headers interleaved with their items, produced through the group adapter.
        Grouped,
        // A tree: every visible row is a real data row, produced through the hierarchy adapter.
        // Unlike Grouped there are no synthetic header rows -- a parent IS a data row.
        Hierarchical,
        // Both axes in force. The ROOTS are bucketed into groups; each root still expands into its
        // own subtree beneath it, and a descendant is never re-bucketed away from its parent. So
        // group headers appear at depth 0 only, and every row under one is a real data row at its
        // real depth -- the two axes compose rather than compete.
        GroupedHierarchical,
    };

    explicit ShapedItemsSource(winrt::IInspectable const& source);
    ~ShapedItemsSource();

    // Subscribes to the source and builds the first projection. Separate from the constructor so
    // the owner can install its handlers first and therefore observe the very first projection.
    void Start();

    // Fired whenever the projection vector has been replaced or re-shaped, i.e. whenever an owner
    // that caches anything derived from it must re-derive. Always fired on the UI thread.
    void SetProjectionRebuiltHandler(std::function<void()> handler) { m_projectionRebuilt = std::move(handler); }

    // Fired only when the projection KIND changed, which is the case where an owner's cached view
    // and row metadata describe a shape that no longer exists.
    void SetShapeSwappedHandler(std::function<void()> handler) { m_shapeSwapped = std::move(handler); }

    // Fired after a shaping verb rewrote the projection. `reorderOnly` distinguishes a pure
    // re-order, which preserves membership, from a change that may have altered which rows exist.
    void SetShapingChangedHandler(std::function<void(bool reorderOnly)> handler) { m_shapingChanged = std::move(handler); }

    // -- shaping verbs --------------------------------------------------------------------
    // Filters are conjunctive. The untokenized overloads are the single-filter shorthand; the
    // tokenized ones let independent filter sources (a column filter and a search box, say) be
    // declared and retracted without knowing about each other.
    void SetFilter(ShapingHelpers::Predicate const& predicate);
    void SetFilter(winrt::hstring const& axisToken, ShapingHelpers::Predicate const& predicate);
    void ClearFilter();
    void ClearFilter(winrt::hstring const& axisToken);
    void SetGroup(
        ShapingHelpers::KeySelector const& key,
        RowIdentity::IdentitySelector const& groupIdentitySelector);
    void ClearGroup();

    // Declares the source a HIERARCHY through a self-referencing parent/child relation over the flat
    // rows: `key` yields each item's unique identity and `parentKey` the identity of its parent (null
    // or empty for a root; a key that matches no item also makes a root). The index is rebuilt from
    // the whole source on every refresh, and invalid data (duplicate / null key, self-parent, cycle)
    // throws E_INVALIDARG with the previous projection intact. Re-declaring or clearing the relation
    // resets expansion intent: a different relation is a different tree.
    //
    // Composes with grouping. When both are in force the GROUP key is applied to the ROOTS: the
    // top level is bucketed under headers, and each root still expands into its own subtree. A
    // descendant is never pulled out from under its parent to join a bucket, because its depth --
    // and therefore the tree itself -- would not survive it.
    void SetParent(ShapingHelpers::KeySelector key, ShapingHelpers::KeySelector parentKey);
    void ClearParentBy();
    void SetSort(
        winrt::hstring const& previousAxisToken,
        winrt::hstring const& axisToken,
        ShapingHelpers::KeySelector const& key,
        winrt::Windows::Foundation::IUnknown const& keyIdentity,
        winrt::hstring const& sortMemberPath,
        winrt::SortDirection direction);
    void ClearSorts();
    void ClearSort(winrt::hstring const& axisToken);
    // Drops every sort axis except axisToken. Lets a consumer that owns ONE axis assert itself as
    // the only sort without having to know the tokens of axes it did not declare.
    void ClearSortsExcept(winrt::hstring const& axisToken);

    // What an active sort axis looks like from outside the engine. Enough for a consumer to tell
    // an axis it declared from one it did not, and to say which property a foreign axis sorts on.
    struct ActiveSortAxisInfo
    {
        winrt::hstring AxisToken;
        // Empty when the axis was declared with a delegate no property path expresses.
        winrt::hstring SortMemberPath;
        winrt::SortDirection Direction{ winrt::SortDirection::None };
    };

    // The active sort axes in precedence order (index 0 is the primary sort). An untokenized axis
    // reports an empty token, so a consumer can tell "an axis I do not own exists" from "only mine
    // exists".
    std::vector<ActiveSortAxisInfo> ActiveSortAxisInfos() const;

    // -- projection -----------------------------------------------------------------------
    // Name this engine uses to prefix caller-facing diagnostics. Layer 2 must not hardcode a
    // layer-4 type name, but the messages are contractual for apps that already ship against
    // TableViewSource, so the consumer supplies its own name instead of the text changing.
    void DiagnosticName(winrt::hstring const& value) { m_diagnosticName = value; }
    winrt::hstring const& DiagnosticName() const noexcept { return m_diagnosticName; }
    ProjectionKind Kind() const noexcept { return m_kind; }
    bool IsProjectedAsGrouped() const noexcept { return m_projectedAsGrouped; }
    // The flat shaped row vector: the presented row axis for flat and unshaped projections. Under
    // a Grouped projection the presented row axis is the GroupedSourceAdapter's computed
    // ItemsSourceView instead, but this vector is still maintained as the flat shaped projection
    // (group order, headers excluded) so Rows() stays coherent regardless of grouping.
    winrt::IObservableVector<winrt::IInspectable> Rows() const noexcept { return m_rows; }
    std::shared_ptr<GroupedSourceAdapter> GroupedAdapter() const noexcept { return m_groupedAdapter; }
    std::shared_ptr<HierarchicalSourceAdapter> HierarchicalAdapter() const noexcept { return m_hierarchicalAdapter; }
    // The selector every identity consumer must use. Derives identity from each item's object
    // identity, so shaping never depends on the app having a unique domain key.
    ShapingHelpers::KeySelector const& IdentitySelector() const noexcept { return EffectiveIdentitySelector(); }

    void Refresh();

private:
    void SubscribeToSourceCollectionChanges();
    void UnsubscribeFromSourceCollectionChanges();
    void OnSourceCollectionChanged();
    void OnSourceCollectionChanged(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    void OnSourceVectorChanged(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    void ApplyIncrementalChange(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    void ApplyIncrementalVectorChange(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    bool TryApplyIncrementalSortedChange(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    void ApplyShapingChange();
    bool TryApplyShapingDeltaInPlace(ShapingHelpers::ShapingDelta const& delta);
    void InvalidateShapingState();
    static std::vector<winrt::IInspectable> Materialize(winrt::IInspectable const& source);
    void ApplyFilter(std::vector<winrt::IInspectable>& rows) const { m_pipeline.ApplyFilter(rows); }
    void ApplySort(std::vector<winrt::IInspectable>& rows, int32_t afterOrder = -1, int32_t beforeOrder = -1) const { m_pipeline.ApplySort(rows, afterOrder, beforeOrder); }
    void RebuildFlat(std::vector<winrt::IInspectable>& rows);
    void RebuildGrouped(std::vector<winrt::IInspectable>& rows);
    void RebuildHierarchical(std::vector<winrt::IInspectable>& rows);
    // Both axes. Buckets the index's roots, hands the adapter the bucket-ordered roots as one
    // segment per bucket, then hands the grouped adapter one group per bucket whose Items are that
    // bucket's visible rows.
    void RebuildGroupedHierarchical(std::vector<winrt::IInspectable>& rows);

    // Builds and validates the parent-key index over the already-sorted, UNFILTERED rows; the
    // active filter is applied inside (matches plus ancestors). Throws E_INVALIDARG on invalid data
    // before anything is mutated, so the previous projection stays intact. Returns null -- and
    // queues a Refresh -- when a selector re-declared or retracted the relation mid-build; the
    // caller then publishes nothing.
    std::shared_ptr<ShapingHelpers::ParentKeyIndex> BuildHierarchyIndex(
        std::vector<winrt::IInspectable> const& sortedRows,
        uint64_t declarationGeneration);

    // Creates the adapter on first use, applies a pending intent reset (relation re-declared) and
    // hands it the index. `rootSegments` as for HierarchicalSourceAdapter::SetIndex.
    void PublishHierarchyIndex(std::shared_ptr<ShapingHelpers::ParentKeyIndex> index, std::vector<size_t> rootSegments);

    // Snapshot of the hierarchy adapter's currently visible rows, in order.
    std::vector<winrt::IInspectable> VisibleHierarchicalRows() const;

    // Re-derives each group's Items from the hierarchy adapter's current entries. Called on the
    // initial build and again whenever a node toggle splices the adapter, because the grouped
    // adapter's rows are those Items and nothing else would tell it the tree changed shape.
    void ResliceGroupsFromHierarchy();

    // Bucket assignment for each root, by object identity. Rebuilt by RebuildGroupedHierarchical
    // and read by ResliceGroupsFromHierarchy, which runs later and off a notification.
    std::unordered_map<void*, size_t> m_rootBucketIndex;
    std::vector<winrt::com_ptr<ShapedGroup>> m_hierarchyGroups;

    // Tears down the hierarchical projection: the coherent-edge callback, the adapter's index and
    // expansion intent, the grouped slices derived from it, and the engine's reference to the
    // adapter itself. Called by every rebuild path that is NOT hierarchical, because a retained
    // adapter would keep re-slicing groups -- and keep its last rows alive -- for a projection that
    // is no longer being shown.
    void ReleaseHierarchyProjection();
    // Completes a ClearParentBy that arrived while a publication was on the stack. Runs when that
    // publication unwinds, on success and failure alike; a no-op once non-hierarchical metadata has
    // been published.
    void CompleteDeferredHierarchyTeardown();
    // Set by ClearParentBy; cleared only once non-hierarchical metadata has actually been published
    // (m_hierarchyPublished), not merely once the adapter is released or a flat kind is staged.
    bool m_pendingHierarchyTeardown{ false };
    // Whether the consumer may currently hold hierarchical row metadata. Tracked apart from m_kind,
    // which a rebuild sets BEFORE publishing: set when a hierarchical publication starts, cleared
    // only when a non-hierarchical one completes.
    bool m_hierarchyPublished{ false };
    // Raises ProjectionRebuilt for the staged m_kind and maintains m_hierarchyPublished.
    void PublishProjection();
    // One completion attempt; CompleteDeferredHierarchyTeardown guards and replays it.
    void TryCompleteDeferredHierarchyTeardown();
    // Set while a teardown completion runs (and publishes). Nested ClearParentBy / completion /
    // Refresh / source-change requests are deferred behind it rather than re-entering.
    bool m_completingTeardown{ false };
    // A completion requested while one was already running; replayed once when it unwinds.
    bool m_teardownReplayRequested{ false };
    // The visible rows of a released, ungrouped tree while its teardown is owed.
    std::vector<winrt::IInspectable> m_releasedHierarchyRows;
    // Drops the adapter's collapse overrides on filter context rows. Called by every filter verb.
    void ResetHierarchyFilterOverlay();
    // Guards the re-slice against re-entering itself through the group mutations it performs, and
    // defers any Refresh requested meanwhile (see Refresh).
    bool m_reslicingGroups{ false };
    // Bumped whenever m_hierarchyGroups is replaced or released, so a re-slice that is still
    // publishing can tell its snapshot is stale.
    uint64_t m_hierarchyGeneration{ 0 };
    void RebuildUnshapedRows(std::vector<winrt::IInspectable> const& rows, wchar_t const* reason);
    bool IsIdentityRequired() const;
    // True when any of Filter / Sort / GroupBy is in force. Distinct from IsIdentityRequired,
    // which is also true for a merely mutable source: a mutable source with no verbs still wants
    // no identity, because there is no projection to anchor.
    bool HasAnyShapingVerb() const;
    // Every identity consumer funnels through here. Identity comes from each item's object
    // identity, so a row ALWAYS has one and no shaping verb has to refuse to run for want of one.
    ShapingHelpers::KeySelector const& EffectiveIdentitySelector() const noexcept
    {
        return m_intrinsicKeySelector;
    }
    // Confirm every row in the set a rebuild is about to publish has a usable, distinct identity.
    bool ValidateRowIdentities(std::vector<winrt::IInspectable> const& rows, wchar_t const*& reason) const;
    bool HasActiveSort() const;
    bool IsSourceMutable() const;
    bool TryGetRequiredRowIdentity(winrt::IInspectable const& item, winrt::hstring& identity, wchar_t const*& reason) const;
    bool TryGetGroupIdentity(winrt::IInspectable const& key, winrt::hstring& identity, wchar_t const*& reason) const;
    void ClearFlatRowIdentityTracking();
    void RebuildFlatRowIdentityTracking(std::vector<winrt::IInspectable> const& rows);
    bool TryGetTrackedFlatRowIndex(winrt::hstring const& identity, uint32_t& index) const;
    void ShiftTrackedFlatRowIndicesForInsert(uint32_t insertedIndex);
    void ShiftTrackedFlatRowIndicesForRemove(uint32_t removedIndex);
    // Prefixes a caller-facing message with the consumer's diagnostic name.
    winrt::hstring Diagnostic(std::wstring_view text) const;
    ShapingHelpers::ShapingPipeline::SortedInsertPlacement SortedInsertPlacementFor(winrt::IInspectable const& item) const;
    bool TryGetSourceItemCount(uint32_t& count) const;
    void RaiseProjectionRebuilt() const { if (m_projectionRebuilt) { m_projectionRebuilt(); } }
    void RaiseShapeSwapped() const { if (m_shapeSwapped) { m_shapeSwapped(); } }
    void RaiseShapingChanged(bool reorderOnly) const { if (m_shapingChanged) { m_shapingChanged(reorderOnly); } }

    // Single authoritative source. Filtering, sorting, and grouping derive the projection without
    // mutating it.
    winrt::IInspectable m_source{ nullptr };
    // Per-object identity, derived from each item's canonical IUnknown pointer. Built once and
    // retained rather than minted per call so the selector's own identity is stable, and so the
    // cost is paid once instead of on every row projection.
    ShapingHelpers::KeySelector m_intrinsicKeySelector{ RowIdentity::MakeObjectIdentitySelector() };
    // The recipe: which verbs are in force and in what order. This class keeps the projection.
    ShapingHelpers::ShapingPipeline m_pipeline{};
    // Retained layer-1 projection state for the FLAT path: the post-filter rows in source order
    // plus the shaped output. Holding FilteredSource is what makes an in-place re-sort produce
    // exactly what a full rebuild would -- a stable sort seeded from the previously sorted order
    // would break ties in the OLD sort's order instead of source order. Only valid while
    // HasProjection is true; every non-Refresh mutation of m_rows clears it.
    ShapingHelpers::ShapingState m_shapingState{};
    ShapingHelpers::KeySelector m_groupSelector{ nullptr };
    RowIdentity::IdentitySelector m_groupIdentitySelector{ nullptr };
    ProjectionKind m_kind{ ProjectionKind::None };
    bool m_projectedAsGrouped{ false };
    winrt::hstring m_diagnosticName{ L"ShapedItemsSource" };
    winrt::IObservableVector<winrt::IInspectable> m_rows{ nullptr };
    winrt::IObservableVector<winrt::IInspectable> m_groupSource{ nullptr };
    // Identities of the rows currently in the flat projection, and each one's index in it.
    // Maintained incrementally so the sorted fast-path can detect duplicate/empty identities and
    // locate a removed row without an O(n) WinRT ABI scan.
    std::unordered_set<winrt::hstring> m_flatRowIdentities;
    std::unordered_map<winrt::hstring, uint32_t> m_flatRowIdentityToIndex;
    // Guards re-entrant Refresh (a source notification arriving while a rebuild's ReplaceAll is
    // already mutating the projection).
    bool m_isRefreshing{ false };
    // Guards re-entrant incremental application: a synchronous VectorChanged handler that mutates
    // the source must not interleave a nested update against a half-updated projection.
    bool m_isApplyingIncrementalChange{ false };
    bool m_pendingRefresh{ false };
    // A Refresh request that arrived during a pass that then threw is posted to the dispatcher
    // (see Refresh); this coalesces the posts.
    void ScheduleRefreshReplay();
    // Posts a pending Refresh after a failed incremental application; keeps it if nothing posted.
    void PostPendingRefresh() noexcept;
    bool m_refreshReplayScheduled{ false };
    std::unordered_map<winrt::hstring, winrt::com_ptr<ShapedGroup>> m_groupCache;
    std::shared_ptr<GroupedSourceAdapter> m_groupedAdapter{};

    // The parent-key relation. Both set or both null; m_parentKeySelector is the "hierarchy is
    // declared" test everywhere.
    ShapingHelpers::KeySelector m_keySelector{ nullptr };
    ShapingHelpers::KeySelector m_parentKeySelector{ nullptr };
    // Bumped by SetParent / ClearParentBy. An index build that sees it move under it (a selector
    // re-declared the relation) discards its result, including any validation error.
    uint64_t m_parentDeclarationGeneration{ 0 };
    // Set by SetParent, consumed by the next publish: re-declaring the relation clears intent.
    bool m_parentRelationRedeclared{ false };
    // Set by the parent verbs, consumed by ApplyShapingChange. The hierarchy axis re-projects
    // rather than filtering/bucketing/sorting, so it has no description in the pipeline spec and
    // the spec diff cannot report it. Survives a shaping batch: the batch's own flag only defers
    // the apply, it does not consume this one.
    bool m_hierarchyAxisDirty{ false };
    std::shared_ptr<HierarchicalSourceAdapter> m_hierarchicalAdapter{};

    std::function<void()> m_projectionRebuilt{ nullptr };
    std::function<void()> m_shapeSwapped{ nullptr };
    std::function<void(bool)> m_shapingChanged{ nullptr };

    // Resolved once per bound source: what shape the source is and how to read it. Every indexed
    // read, count and observability question in this class goes through it, so no two of them can
    // disagree about what the source supports.
    ShapingHelpers::CollectionAccessor m_sourceAccessor{};

    winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged::CollectionChanged_revoker m_sourceCollectionChangedRevoker{};
    winrt::IObservableVector<winrt::IInspectable>::VectorChanged_revoker m_sourceVectorChangedRevoker{};
    winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector::VectorChanged_revoker m_sourceBindableVectorChangedRevoker{};
};
