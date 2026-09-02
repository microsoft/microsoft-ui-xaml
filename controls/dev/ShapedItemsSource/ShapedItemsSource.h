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

class GroupedSourceAdapter;
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
    // What the last rebuild actually produced -- the EFFECTIVE shape, not the requested one. A
    // grouping request degrades to Flat when group identity is unresolvable or collides, and a
    // source with no usable row identity degrades to Unshaped, a plain 1:1 mirror. Consumers read
    // this to decide how to interpret a row, so it must never report intent.
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
    };

    explicit ShapedItemsSource(winrt::IInspectable const& source);
    ~ShapedItemsSource();

    // Scope returned by DeferRefresh. Move-only: copying it would end the deferral early.
    class DeferRefreshScope
    {
    public:
        explicit DeferRefreshScope(ShapedItemsSource* owner) noexcept : m_owner(owner) {}
        DeferRefreshScope(DeferRefreshScope&& other) noexcept : m_owner(std::exchange(other.m_owner, nullptr)) {}
        DeferRefreshScope& operator=(DeferRefreshScope&& other) noexcept
        {
            if (this != &other)
            {
                Release();
                m_owner = std::exchange(other.m_owner, nullptr);
            }
            return *this;
        }
        DeferRefreshScope(DeferRefreshScope const&) = delete;
        DeferRefreshScope& operator=(DeferRefreshScope const&) = delete;
        ~DeferRefreshScope() { Release(); }

    private:
        void Release() noexcept
        {
            if (auto* const owner = std::exchange(m_owner, nullptr))
            {
                owner->EndShapingBatch();
            }
        }

        ShapedItemsSource* m_owner{ nullptr };
    };

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
    void SetFilter(TabularShapingHelpers::TabularPredicate const& predicate);
    void SetFilter(winrt::hstring const& axisToken, TabularShapingHelpers::TabularPredicate const& predicate);
    void ClearFilter();
    void ClearFilter(winrt::hstring const& axisToken);
    void SetGroup(
        TabularShapingHelpers::TabularKeySelector const& key,
        RowIdentity::TabularIdentitySelector const& groupIdentitySelector);
    void ClearGroup();
    void SetSort(
        winrt::hstring const& previousAxisToken,
        winrt::hstring const& axisToken,
        TabularShapingHelpers::TabularKeySelector const& key,
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
    // The selector every identity consumer must use. Derives identity from each item's object
    // identity, so shaping never depends on the app having a unique domain key.
    TabularShapingHelpers::TabularKeySelector const& IdentitySelector() const noexcept { return EffectiveIdentitySelector(); }

    void Refresh();

    // Suppresses intermediate projections while several verbs are declared as one change.
    // A consumer whose API surfaces shaping as a COLLECTION (e.g. a vector of sort descriptions)
    // has to re-declare every axis whenever one of them moves; without this each axis would
    // rebuild the projection and emit its own Reset. Re-entrant: only the outermost scope
    // applies. Spec diffing is unaffected -- the pipeline diffs against the last COMMITTED spec,
    // so one commit at the end sees exactly the accumulated change.
    //
    // Scope-bound rather than a Begin/End pair, matching ICollectionView::DeferRefresh: every
    // consumer was already wrapping the pair in a scope guard, and an unbalanced End would
    // strand the projection in a permanently deferred state.
    [[nodiscard]] DeferRefreshScope DeferRefresh();

private:
    void BeginShapingBatch();
    void EndShapingBatch();
    void SubscribeToSourceCollectionChanges();
    void UnsubscribeFromSourceCollectionChanges();
    void OnSourceCollectionChanged();
    void OnSourceCollectionChanged(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    void OnSourceVectorChanged(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    void ApplyIncrementalChange(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    void ApplyIncrementalVectorChange(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    bool TryApplyIncrementalSortedChange(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    void ApplyShapingChange();
    bool TryApplyShapingDeltaInPlace(TabularShapingHelpers::ShapingDelta const& delta);
    void InvalidateShapingState();
    static std::vector<winrt::IInspectable> Materialize(winrt::IInspectable const& source);
    void ApplyFilter(std::vector<winrt::IInspectable>& rows) const { m_pipeline.ApplyFilter(rows); }
    void ApplySort(std::vector<winrt::IInspectable>& rows, int32_t afterOrder = -1, int32_t beforeOrder = -1) const { m_pipeline.ApplySort(rows, afterOrder, beforeOrder); }
    void RebuildFlat(std::vector<winrt::IInspectable>& rows);
    void RebuildGrouped(std::vector<winrt::IInspectable>& rows);
    void RebuildUnshapedRows(std::vector<winrt::IInspectable> const& rows, wchar_t const* reason);
    bool IsIdentityRequired() const;
    // True when any of Filter / Sort / GroupBy is in force. Distinct from IsIdentityRequired,
    // which is also true for a merely mutable source: a mutable source with no verbs still wants
    // no identity, because there is no projection to anchor.
    bool HasAnyShapingVerb() const;
    // Every identity consumer funnels through here. Identity comes from each item's object
    // identity, so a row ALWAYS has one and no shaping verb has to refuse to run for want of one.
    TabularShapingHelpers::TabularKeySelector const& EffectiveIdentitySelector() const noexcept
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
    static winrt::hstring StringifyKey(winrt::IInspectable const& key);
    // Prefixes a caller-facing message with the consumer's diagnostic name.
    winrt::hstring Diagnostic(std::wstring_view text) const;
    TabularShapingHelpers::ShapingPipeline::SortedInsertPlacement SortedInsertPlacementFor(winrt::IInspectable const& item) const;
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
    TabularShapingHelpers::TabularKeySelector m_intrinsicKeySelector{ RowIdentity::MakeObjectIdentitySelector() };
    // The recipe: which verbs are in force and in what order. This class keeps the projection.
    TabularShapingHelpers::ShapingPipeline m_pipeline{};
    // Retained layer-1 projection state for the FLAT path: the post-filter rows in source order
    // plus the shaped output. Holding FilteredSource is what makes an in-place re-sort produce
    // exactly what a full rebuild would -- a stable sort seeded from the previously sorted order
    // would break ties in the OLD sort's order instead of source order. Only valid while
    // HasProjection is true; every non-Refresh mutation of m_rows clears it.
    TabularShapingHelpers::ShapingState m_shapingState{};
    TabularShapingHelpers::TabularKeySelector m_groupSelector{ nullptr };
    RowIdentity::TabularIdentitySelector m_groupIdentitySelector{ nullptr };
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
    // Depth of the current BeginShapingBatch scope, plus what the batch owes when it unwinds.
    uint32_t m_shapingBatchDepth{ 0 };
    bool m_shapingBatchHasShapingChange{ false };
    bool m_shapingBatchHasRefresh{ false };
    std::unordered_map<winrt::hstring, winrt::com_ptr<ShapedGroup>> m_groupCache;
    std::shared_ptr<GroupedSourceAdapter> m_groupedAdapter{};

    std::function<void()> m_projectionRebuilt{ nullptr };
    std::function<void()> m_shapeSwapped{ nullptr };
    std::function<void(bool)> m_shapingChanged{ nullptr };

    // Resolved once per bound source: what shape the source is and how to read it. Every indexed
    // read, count and observability question in this class goes through it, so no two of them can
    // disagree about what the source supports.
    TabularShapingHelpers::CollectionAccessor m_sourceAccessor{};

    winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged::CollectionChanged_revoker m_sourceCollectionChangedRevoker{};
    winrt::IObservableVector<winrt::IInspectable>::VectorChanged_revoker m_sourceVectorChangedRevoker{};
    winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector::VectorChanged_revoker m_sourceBindableVectorChangedRevoker{};
};
