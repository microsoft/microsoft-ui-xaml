// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "pch.h"
#include "common.h"

#include "ShapingPipeline.h"
#include "ShapingHelpers.h"
#include "RowIdentity.h"
#include "HierarchyContract5.h"
#include "LiveShapingTracker.h"

#include <winrt/Microsoft.UI.Dispatching.h>

class GroupedSourceAdapter;
class ShapedGroup;
namespace ShapingHelpers {}
namespace cw5
{
namespace ShapingHelpers { using namespace ::ShapingHelpers; }

using ::GroupedSourceAdapter;
class HierarchicalSourceAdapter;
class HierarchyStructureCache;
struct HierarchyIndex;
struct HierarchyStructureChangeLog;
using ::ShapedGroup;

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
        // A tree via the hierarchy adapter; no synthetic rows, a parent IS a data row.
        Hierarchical,
        // Both axes: ROOTS are bucketed into groups (headers at depth 0 only); descendants stay
        // under their parent.
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

    // Declares the source a HIERARCHY: `children` yields a node's child collection, and the
    // optional `hasChildren` answers "is this expandable" without enumerating, which is what keeps
    // a collapsed subtree genuinely unrealized. Clearing returns the projection to flat/grouped.
    //
    // Composes with grouping. When both are in force the GROUP key is applied to the ROOTS: the
    // top level is bucketed under headers, and each root still expands into its own subtree.
    void SetChildren(
        ShapingHelpers::ChildrenFn const& children,
        ShapingHelpers::HasChildrenFn const& hasChildren);
    void ClearChildren();
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
    bool HasActiveSort() const;

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

    // Under a children-selector hierarchy: whether `item` is a node of the shaped tree (a root or
    // a descendant, visible or collapsed; a kept node when a filter retains ancestors). Empty
    // when no hierarchy is projected, so the caller falls back to Rows().
    std::optional<bool> ContainsHierarchyItem(winrt::IInspectable const& item) const;
    // The selector every identity consumer must use. Derives identity from each item's object
    // identity, so shaping never depends on the app having a unique domain key.
    ShapingHelpers::KeySelector const& IdentitySelector() const noexcept { return EffectiveIdentitySelector(); }

    void Refresh();

    // Child-collection changes under a hierarchy are coalesced into one posted update per
    // dispatcher turn. Applies a pending one now, so a caller about to read or act on the
    // projection sees it current. No-op when nothing is pending, or while an update is running.
    void FlushPendingStructureChanges();

    // False when a shaped projection could not identify every row (a null item, or one object on
    // two rows), so a shaping verb that would throw from Refresh can be refused up front.
    bool CanProjectWithRowIdentity() const;
    // One switch for sort keys, group key and filter result together. Under a children-selector
    // hierarchy the structure cache observes the items instead and re-shapes affected sibling
    // lists; off, child-collection change notifications are the only invalidator.
    void SetLiveShaping(bool enabled);
    bool IsLiveShapingEnabled() const noexcept { return m_liveShapingEnabled; }

    // Lets the owner hold live restores (e.g. while a cell editor is open). A held restore stays
    // dirty and runs the turn after ResumeHeldLiveShaping.
    void SetLiveShapingHold(std::function<bool()> isHeld) { m_liveShapingHold = std::move(isHeld); }
    void ResumeHeldLiveShaping();

    // Runs per live item change before snapshot comparison, so the owner can refresh state the
    // selectors read (comparer ranks). Arg: restore already posted. True forces a reshape. Also
    // runs for each arriving item, and once with a null item when live shaping turns on (every
    // item may have changed untracked).
    void SetLiveItemChangedHook(std::function<bool(winrt::IInspectable const&, bool)> hook) { m_liveItemChangedHook = std::move(hook); }

    // Lets the owner drop per-row state (comparer ranks) for rows that left the source: called with the
    // removed item after an in-place sorted removal, or with null and every source row after a rebuild.
    void SetRowsReleasedHook(std::function<void(winrt::IInspectable const&, std::vector<winrt::IInspectable> const&)> hook) { m_rowsReleasedHook = std::move(hook); }

    // Lets the owner restamp realized rows whose hierarchy metadata (chevron, expanded or context
    // state, sibling position) was patched in place, which no collection notification reports.
    void SetRowsMetadataChangedHook(std::function<void()> hook) { m_rowsMetadataChangedHook = std::move(hook); }

private:
    void SubscribeToSourceCollectionChanges();
    void UnsubscribeFromSourceCollectionChanges();
    void OnSourceCollectionChanged();
    void OnSourceCollectionChanged(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    void OnSourceVectorChanged(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    void ApplyIncrementalChange(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    void ApplyIncrementalVectorChange(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args);
    bool TryApplyIncrementalSortedChange(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    bool TryReshapeStaleSortedFlatProjection();
    bool IsFlatProjectionSorted() const;
    bool IsFlatRowInSortOrderWithNeighbours(uint32_t index) const;
    void TrackFlatItemPropertyChanged(winrt::IInspectable const& item);
    void UntrackFlatItemPropertyChanged(winrt::IInspectable const& item);
    void ClearFlatItemPropertyChangedTracking();
    void RebuildFlatItemPropertyChangedTracking(std::vector<winrt::IInspectable> const& rows);
    // Single owner for sort-key staleness: live shaping already subscribes to every item and
    // re-shapes on any shaping-relevant edit, so the sort-key subscriptions would be a second
    // handler per row observing the same change.
    bool ShouldTrackSortKeyChanges() const { return HasActiveSort() && !IsLiveShapingEnabled(); }
    void ClearSortDirtyRows();
    void OnFlatItemPropertyChanged(uintptr_t itemAddress, winrt::hstring const& propertyName);
    void UpdateSortKeyPropertyFilter();
    void ApplyShapingChange();
    bool TryApplyShapingDeltaInPlace(ShapingHelpers::ShapingDelta const& delta, bool allowDuringIncrementalChange = false);
    void InvalidateShapingState();
    static std::vector<winrt::IInspectable> Materialize(winrt::IInspectable const& source);
    void ApplyFilter(std::vector<winrt::IInspectable>& rows) const { m_pipeline.ApplyFilter(rows); }
    void ApplySort(std::vector<winrt::IInspectable>& rows, int32_t afterOrder = -1, int32_t beforeOrder = -1) const { m_pipeline.ApplySort(rows, afterOrder, beforeOrder); }
    void RebuildFlat(std::vector<winrt::IInspectable>& rows);
    void RebuildGrouped(std::vector<winrt::IInspectable>& rows);
    void RebuildHierarchical(std::vector<winrt::IInspectable>& rows);
    // Both axes: buckets the roots, one group per bucket; each root keeps its subtree.
    void RebuildGroupedHierarchical(std::vector<winrt::IInspectable>& rows);

    // Bumped by every source change notification, including ones deferred behind a rebuild.
    uint64_t m_sourceChangeStamp{ 0 };
    // Stamp captured when the current Refresh materialized the source.
    uint64_t m_refreshSourceStamp{ 0 };
    // Stamp whose rows the last successful shaped Refresh validated; never a real stamp until then.
    uint64_t m_validatedSourceStamp{ ~uint64_t{ 0 } };

    // Creates or re-attaches the hierarchy adapter over `roots`. `shapeRoots` false means the
    // caller owns root order (grouped) and installs the re-slice callback; `rootSegments` are the
    // per-bucket root counts then.
    void EnsureHierarchicalAdapter(std::vector<winrt::IInspectable> const& roots, bool shapeRoots, std::vector<size_t> rootSegments = {});

    std::vector<winrt::IInspectable> VisibleHierarchicalRows() const;

    // Re-derives group Items from the hierarchy adapter; also on node toggles, since nothing else
    // tells the grouped adapter the tree changed.
    void ResliceGroupsFromHierarchy();

    // Root -> bucket, by object identity.
    std::unordered_map<void*, size_t> m_rootBucketIndex;
    std::vector<winrt::com_ptr<ShapedGroup>> m_hierarchyGroups;

    // Tears down the hierarchical projection. Every non-hierarchical rebuild calls it, else the
    // adapter keeps re-slicing groups and pinning rows.
    void ReleaseHierarchyProjection();
    // Raises ProjectionRebuilt for the staged m_kind.
    void PublishProjection() const { RaiseProjectionRebuilt(); }
    // Re-entrancy guard for the re-slice; Refresh defers while set.
    bool m_reslicingGroups{ false };

    // Ancestor retention under a filter: a node is kept when it or any descendant matches; a kept
    // node that does not itself match is a context row. Computed by a pre-walk over the shared
    // structure cache (lazy hierarchy), or inside BuildHierarchyIndex and kept current from the
    // index by TryUpdateHierarchyIndex (eager). Keyed by ABI
    // pointer; m_filterKeepAlive pins the objects so a pointer
    // cannot be recycled while the sets are live.
    void ComputeFilterRetention(std::vector<winrt::IInspectable> const& roots);
    void ClearFilterRetention();
    void ResetFilterContextOverlay();
    bool IsFilterKept(winrt::IInspectable const& item) const;
    bool IsFilterContext(winrt::IInspectable const& item) const;
    bool m_filterRetentionActive{ false };
    std::unordered_set<void*> m_filterKept;
    std::unordered_set<void*> m_filterContext;
    std::vector<winrt::IInspectable> m_filterKeepAlive;

    // Structure cache shared by the retention pre-walk and the adapter's emit walk. With live
    // shaping off, child-collection change notifications are its only invalidator.
    std::shared_ptr<HierarchyStructureCache> m_structureCache;
    HierarchyStructureCache& EnsureStructureCache();
    void ResetStructureCache();
    void OnStructureChanged();
    void ApplyStructureChange();
    void ScheduleStructureFlush();
    bool m_structureFlushPending{ false };
    bool m_structureFlushPosted{ false };
    bool m_inStructureFlush{ false };

    // Eager hierarchy: WithChildren without a HasChildrenSelector. The whole tree is walked once and
    // every sibling set shaped into a HierarchyIndex the adapter emits from; filter retention is
    // derived in that walk instead of ComputeFilterRetention. Afterwards a child collection change,
    // a live edit, a filter change or a refresh that does not re-sort updates the index from the
    // structure cache's change log (UpdateHierarchyIndex) instead of walking the tree again -- with
    // a filter, re-deriving retention from the index's own edges and re-shaping only the sibling
    // sets whose kept membership moved. With a HasChildrenSelector the hierarchy stays lazy and has
    // no index.
    bool IsEagerHierarchy() const noexcept { return m_childrenSelector && !m_hasChildrenSelector; }
    // Filter (by kept set under retention, else by predicate) and sort one sibling set: the
    // adapter's ShapeSiblings and the eager build, so both produce and share the same lists.
    void ShapeHierarchySiblings(std::vector<winrt::IInspectable>& siblings) const;
    // Must run inside a structure pass. Null when the structure cannot be indexed (a cycle, an
    // over-deep chain, a change during the walk); the adapter then walks the slow way.
    std::shared_ptr<HierarchyIndex> BuildHierarchyIndex(std::vector<winrt::IInspectable> const& roots, bool computeRetention);
    // Brings m_hierarchyIndex (and, with `retention`, the kept sets) current against `roots` and
    // the cache's change log, in place, when it can; otherwise rebuilds it in full. Either way the
    // log is consumed. `reevaluateAll` re-reads every filter verdict rather than only those of the
    // nodes the log names. Must run inside a structure pass. TryUpdateHierarchyIndex reports
    // whether a root's kept state moved (the root set is shaped by Refresh, not the index). With
    // `changed`, an in-place update reports the nodes whose shaped children, child count or
    // context state may have moved (true); a rebuild leaves it empty and returns false.
    bool UpdateHierarchyIndex(
        std::vector<winrt::IInspectable> const& roots,
        bool retention,
        bool reevaluateAll,
        std::vector<winrt::IInspectable>* changed = nullptr);
    bool TryUpdateHierarchyIndex(
        std::vector<winrt::IInspectable> const& roots,
        HierarchyStructureChangeLog const& log,
        bool retention,
        bool reevaluateAll,
        bool& rootsKeptChanged,
        std::vector<winrt::IInspectable>* changed = nullptr);
    // Shared by both: indexes `item` and every descendant not yet in the index (post-order, with
    // the unshaped children each was read with), and shapes one indexed node (children, child
    // count, identity, trust) from the cache.
    bool IndexHierarchySubtree(
        HierarchyIndex& index,
        winrt::IInspectable const& item,
        std::vector<uint32_t>& added,
        std::vector<std::shared_ptr<const std::vector<winrt::IInspectable>>>& addedChildren);
    void ShapeHierarchyIndexNode(
        HierarchyIndex& index,
        uint32_t slot,
        bool computeRetention,
        ShapingHelpers::ShapeSiblingsFn const& shape,
        std::vector<uint32_t>& seenUnder);
    // Whether the indexed structure (every live node's unshaped edges) is acyclic and no deeper than
    // the walk supports, re-deriving every live node's Height. One flat pass over the index; the
    // update's fallback when its local check of new edges runs out of budget.
    static bool IsHierarchyIndexWalkable(HierarchyIndex& index);
    std::shared_ptr<HierarchyIndex> m_hierarchyIndex;
    // Moved by every filter change; an index under an older one re-reads every verdict.
    uint64_t m_filterGeneration{ 0 };
    bool m_buildingHierarchyIndex{ false };
    bool m_structureChangedDuringBuild{ false };

    // Hierarchy live shaping (children-selector hierarchy only; LiveShapingTracker is bypassed).
    void OnHierarchyItemChanged(winrt::IInspectable const& item, winrt::hstring const& propertyName);
    bool IsShapeAffectedBy(winrt::hstring const& propertyName, bool isRoot) const;
    void ScheduleLiveShapingFlush();
    void FlushLiveShaping();
    bool m_liveShapingFlushPosted{ false };
    bool m_liveShapingNeedsRefresh{ false };
    bool m_liveShapingNeedsRebuild{ false };

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
    bool IsSourceMutable() const;
    bool TryGetRequiredRowIdentity(winrt::IInspectable const& item, winrt::hstring& identity, wchar_t const*& reason) const;
    bool TryGetGroupIdentity(winrt::IInspectable const& key, winrt::hstring& identity, wchar_t const*& reason) const;
    // Buckets by m_groupSelector; throws E_INVALIDARG on an invalid group identity.
    std::vector<ShapingHelpers::KeyedBucket> BucketizeGroupsOrThrow(std::vector<winrt::IInspectable> const& items);
    // Finds or creates the cached group for the bucket and sets its GroupKey.
    winrt::com_ptr<ShapedGroup> AcquireCachedGroup(ShapingHelpers::KeyedBucket const& bucket);
    void PruneGroupCache(std::unordered_set<winrt::hstring> const& liveKeys);
    // Loads m_groupSource and (re-)attaches the grouped adapter, publishing one Reset.
    void AttachGroupSource(std::vector<winrt::IInspectable> const& groups);
    void ReattachGroupedAdapterAfterFailedRefill();
    // Detaches the grouped adapter, then clears m_groupSource and m_groupCache.
    void ReleaseGroupedProjection();
    void ClearFlatRowIdentityTracking();
    void RebuildFlatRowIdentityTracking(std::vector<winrt::IInspectable> const& rows);
    bool TryGetTrackedFlatRowIndex(winrt::hstring const& identity, uint32_t& index) const;
    void ShiftTrackedFlatRowIndicesForInsert(uint32_t insertedIndex);
    void ShiftTrackedFlatRowIndicesForRemove(uint32_t removedIndex);
    // Prefixes a caller-facing message with the consumer's diagnostic name.
    winrt::hstring Diagnostic(std::wstring_view text) const;
    bool TryGetSourceItemCount(uint32_t& count) const;
    struct LiveShapeSnapshot
    {
        std::vector<winrt::hstring> SortKeys;
        winrt::hstring GroupKey;
        bool PassesFilter{ true };
    };
    LiveShapeSnapshot CaptureLiveShapeSnapshot(winrt::IInspectable const& item) const;
    static bool LiveShapeSnapshotsDiffer(
        LiveShapeSnapshot const& left,
        LiveShapeSnapshot const& right);
    static void const* LiveShapingKeyFor(winrt::IInspectable const& item);
    // Mark-and-sweep reconcile; surviving items keep their subscription.
    void RefreshLiveShapingSubscriptions(std::vector<winrt::IInspectable> const& items);
    void ClearLiveShapingSubscriptions();
    void ResubscribeLiveShapingFromSource();
    // O(1) per-item delta for incremental paths.
    void AddLiveShapingSubscription(winrt::IInspectable const& item);
    void RemoveLiveShapingSubscription(winrt::IInspectable const& item);
    // Subscription delta driven by the args, not the projection, so every apply path is covered.
    void ApplyLiveShapingDelta(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args);
    void OnLiveShapedItemChanged(
        winrt::IInspectable const& item,
        winrt::hstring const& propertyName);
    // Marks stale and posts ONE restore; N changes in a turn cost one reshape.
    void MarkLiveShapingDirty();
    // Posts a restore for a dirty projection unless one is queued or the hold still applies.
    void EnsureLiveShapingRestore();
    // Posts RestoreLiveShaping to the owner's queue. Runs it now only when the owner has no
    // dispatcher at all (test host); a refused post leaves it dirty for the next change.
    void PostLiveShapingRestore();
    void RestoreLiveShaping();
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
    // HasProjection is true. The sorted incremental path keeps it in lockstep with m_rows; every
    // other non-Refresh mutation of m_rows clears it.
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
    // Sort-key changes raise no collection notification, so while sorted, INPC rows are observed
    // and a change to a sort-key property marks the row dirty for the next collection change.
    // Keyed by IUnknown address (stable across re-orders); holding Item keeps the address unique.
    struct FlatItemPropertyChangedSubscription
    {
        winrt::IInspectable Item{ nullptr };
        winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged::PropertyChanged_revoker Revoker{};
    };
    std::unordered_map<uintptr_t, FlatItemPropertyChangedSubscription> m_flatItemPropertyChangedRevokers;
    bool m_flatItemPropertyChangedTrackingActive{ false };
    bool m_flatItemsMayChangeSortWithoutNotification{ false };
    // First path segment of each active sort axis' SortMemberPath.
    std::unordered_set<winrt::hstring> m_sortKeyPropertyNames;
    // Some axis has no path (delegate key / comparer), so any property may affect the sort.
    bool m_anyPropertyMayAffectSort{ false };
    // Owner thread only: off-thread PropertyChanged is marshalled before it reaches these.
    std::unordered_set<uintptr_t> m_sortDirtyRows;
    bool m_sortDirtyOverflow{ false };
    bool m_raiseReorderAfterIncrementalChange{ false };
    // Bumped by every rebuild and every invalidation of the retained shaping state. The sorted
    // incremental path plans indices, then calls into app code (sort keys, comparers, m_rows
    // listeners) before splicing; a changed value means those indices describe a projection that
    // no longer exists.
    uint64_t m_projectionGeneration{ 0 };
    // Guards re-entrant Refresh (a source notification arriving while a rebuild's ReplaceAll is
    // already mutating the projection). Also held while a deferred hierarchy teardown publishes, so
    // requests from that publication's handlers are deferred the same way.
    bool m_isRefreshing{ false };
    // Guards re-entrant incremental application: a synchronous VectorChanged handler that mutates
    // the source must not interleave a nested update against a half-updated projection.
    bool m_isApplyingIncrementalChange{ false };
    bool m_pendingRefresh{ false };
    // Coalesces posted replays of a Refresh requested during a pass that threw.
    void ScheduleRefreshReplay();
    // Posts a pending Refresh after a failed incremental application; keeps it if nothing posted.
    void PostPendingRefresh() noexcept;
    bool m_refreshReplayScheduled{ false };
    std::unordered_map<winrt::hstring, winrt::com_ptr<ShapedGroup>> m_groupCache;
    std::shared_ptr<GroupedSourceAdapter> m_groupedAdapter{};
    std::shared_ptr<LiveShapingTracker> m_liveShaping{ std::make_shared<LiveShapingTracker>() };
    // Owning thread (m_rows is bound to it); PropertyChanged is always handled here. Null off a
    // dispatcher thread: changes then run inline on the owner thread, flagged on others.
    winrt::Microsoft::UI::Dispatching::DispatcherQueue m_ownerQueue{ nullptr };
    DWORD m_ownerThreadId{ 0 };
    // Set off-thread when a PropertyChanged could not be marshalled to the owner; the owner's next
    // live change forces a full restore. Shared so the handler never has to lock `this` off-thread.
    std::shared_ptr<std::atomic<bool>> m_liveShapingChangeDropped{ std::make_shared<std::atomic<bool>>(false) };
    bool m_liveShapingEnabled{ false };
    std::unordered_map<void const*, LiveShapeSnapshot> m_liveShapeSnapshots;
    // Pending live reshape: the projection may not reflect the captured snapshots.
    bool m_liveShapingDirty{ false };
    // A restore is queued on m_ownerQueue (so it implies m_ownerQueue is set). Separate from dirty
    // so a refused post or a failed restore is retried by the next change instead of stranding the
    // dirty flag.
    bool m_liveShapingRestorePosted{ false };
    std::function<bool()> m_liveShapingHold{};
    // A restore found the hold on and parked; ResumeHeldLiveShaping re-posts it if still dirty.
    // Set only while dirty, but a refresh may clear dirty first, so it alone does not imply dirty.
    bool m_liveShapingHeld{ false };
    std::function<bool(winrt::IInspectable const&, bool)> m_liveItemChangedHook{};
    std::function<void(winrt::IInspectable const&, std::vector<winrt::IInspectable> const&)> m_rowsReleasedHook{};
    std::function<void()> m_rowsMetadataChangedHook{};

    // Children-selector hierarchy; m_childrenSelector is the "hierarchy declared" test.
    ShapingHelpers::ChildrenFn m_childrenSelector{ nullptr };
    ShapingHelpers::HasChildrenFn m_hasChildrenSelector{ nullptr };
    // Root set the hierarchy adapter is attached to.
    winrt::IObservableVector<winrt::IInspectable> m_hierarchySource{ nullptr };
    // Set by children verbs, consumed by ApplyShapingChange: the pipeline spec diff cannot see the
    // hierarchy axis. Survives a shaping batch.
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

} // namespace cw5
