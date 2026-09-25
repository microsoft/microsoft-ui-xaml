// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ShapedItemsSource.h"
#include "RowIdentity.h"
#include "GroupedSourceAdapter.h"
#include "HierarchicalSourceAdapter.h"
#include "SharedHelpers.h"
#include "TVDiag.h"
#include "ShapingHelpers.h"
#include "ShapingRevoke.h"
#include "ShapedGroup.h"

#include <cmath>
#include <algorithm>
#include <string_view>
#include <unordered_set>
#include <winrt/Microsoft.UI.Dispatching.h>

namespace
{

    winrt::Windows::Foundation::Collections::IVectorChangedEventArgs TryAsVectorChangedArgs(
        winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
    {
        return args;
    }

    winrt::Windows::Foundation::Collections::IVectorChangedEventArgs TryAsVectorChangedArgs(
        winrt::IInspectable const& args)
    {
        return args.try_as<winrt::Windows::Foundation::Collections::IVectorChangedEventArgs>();
    }

    void LogIdentityProjectionDisabled(wchar_t const* reason)
    {
#ifdef DBG
        TVDiag::DbgLogF(L"[ShapedItemsSource] Stable identity is required; disabling shaped identity projection (%ls).\n", reason ? reason : L"unspecified reason");
#else
        UNREFERENCED_PARAMETER(reason);
#endif
    }
}

ShapedItemsSource::ShapedItemsSource(winrt::IInspectable const& source) :
    m_source(source),
    m_rows(winrt::single_threaded_observable_vector<winrt::IInspectable>())
{
}

ShapedItemsSource::~ShapedItemsSource()
{
    UnsubscribeFromSourceCollectionChanges();
}

void ShapedItemsSource::Start()
{
    SubscribeToSourceCollectionChanges();
    Refresh();
}

void ShapedItemsSource::BeginShapingBatch()
{
    ++m_shapingBatchDepth;
}

ShapedItemsSource::DeferRefreshScope ShapedItemsSource::DeferRefresh()
{
    BeginShapingBatch();
    return DeferRefreshScope{ this };
}

void ShapedItemsSource::EndShapingBatch()
{
    MUX_ASSERT(m_shapingBatchDepth > 0);
    if (m_shapingBatchDepth == 0 || --m_shapingBatchDepth > 0)
    {
        return;
    }

    bool const rebuild = m_shapingBatchHasRefresh;
    bool const shapingChange = m_shapingBatchHasShapingChange;
    m_shapingBatchHasRefresh = false;
    m_shapingBatchHasShapingChange = false;

    // A pending identity-selector change is the stronger of the two: it invalidates the whole
    // projection, and the spec diff that ApplyShapingChange would commit is still owed either
    // way, so commit it first and let the rebuild publish the result.
    if (shapingChange)
    {
        ApplyShapingChange();
    }
    if (rebuild)
    {
        Refresh();
    }
}

void ShapedItemsSource::SetFilter(ShapingHelpers::Predicate const& predicate)
{
    m_pipeline.SetFilter(predicate);
    ApplyShapingChange();
}

void ShapedItemsSource::SetFilter(winrt::hstring const& axisToken, ShapingHelpers::Predicate const& predicate)
{
    m_pipeline.SetFilter(axisToken, predicate);
    ApplyShapingChange();
}

void ShapedItemsSource::ClearFilter()
{
    m_pipeline.ClearFilter();
    ApplyShapingChange();
}

void ShapedItemsSource::ClearFilter(winrt::hstring const& axisToken)
{
    m_pipeline.ClearFilter(axisToken);
    ApplyShapingChange();
}

void ShapedItemsSource::SetGroup(
    ShapingHelpers::KeySelector const& key,
    RowIdentity::IdentitySelector const& groupIdentitySelector)
{
    m_groupSelector = key;
    m_groupIdentitySelector = groupIdentitySelector;
    m_pipeline.MarkGroupVerb(key);
    ApplyShapingChange();
}

void ShapedItemsSource::ClearGroup()
{
    m_groupSelector = nullptr;
    m_groupIdentitySelector = nullptr;
    m_pipeline.ClearGroupVerb();
    ApplyShapingChange();
}

void ShapedItemsSource::SetChildren(
    ShapingHelpers::ChildrenFn const& children,
    ShapingHelpers::HasChildrenFn const& hasChildren)
{
    m_childrenSelector = children;
    m_hasChildrenSelector = hasChildren;

    // Grouping is NOT retracted. The two axes compose: GroupBy buckets the roots, and each root
    // still expands into its own subtree beneath its bucket's header. See
    // RebuildGroupedHierarchical for the row stream that produces.
    ApplyShapingChange();
}

void ShapedItemsSource::ClearChildren()
{
    m_childrenSelector = nullptr;
    m_hasChildrenSelector = nullptr;
    ApplyShapingChange();
}

void ShapedItemsSource::SetSort(
    winrt::hstring const& previousAxisToken,
    winrt::hstring const& axisToken,
    ShapingHelpers::KeySelector const& key,
    winrt::Windows::Foundation::IUnknown const& keyIdentity,
    winrt::hstring const& sortMemberPath,
    winrt::SortDirection direction)
{
    m_pipeline.SetSort(previousAxisToken, axisToken, key, keyIdentity, sortMemberPath, direction);
    ApplyShapingChange();
}

void ShapedItemsSource::ClearSorts()
{
    m_pipeline.ClearSorts();
    ApplyShapingChange();
}

void ShapedItemsSource::ClearSort(winrt::hstring const& axisToken)
{
    m_pipeline.ClearSort(axisToken);
    ApplyShapingChange();
}

void ShapedItemsSource::ClearSortsExcept(winrt::hstring const& axisToken)
{
    m_pipeline.ClearSortsExcept(axisToken);
    ApplyShapingChange();
}

std::vector<ShapedItemsSource::ActiveSortAxisInfo> ShapedItemsSource::ActiveSortAxisInfos() const
{
    std::vector<ActiveSortAxisInfo> infos;
    for (auto const& axis : m_pipeline.ActiveSortAxes(-1, -1))
    {
        infos.push_back({ axis.AxisToken, axis.SortMemberPath, axis.Direction });
    }
    return infos;
}

void ShapedItemsSource::ApplyShapingChange()
{
    if (m_shapingBatchDepth > 0)
    {
        // Deliberately do NOT commit the spec here: the pipeline diffs against the last
        // committed spec, so deferring the commit is what lets the whole batch read as one delta.
        m_shapingBatchHasShapingChange = true;
        return;
    }

    // Commit unconditionally, even when the in-place path is not taken: the committed spec is
    // the baseline the NEXT verb diffs against, so skipping it would make that diff report a
    // change that has already been applied.
    auto const delta = m_pipeline.CommitSpec();

    if (delta.IsNoOp())
    {
        // Re-declaring the identical shape. The projection already satisfies it, and a rebuild
        // would fire a Reset that drops every realized row for nothing. Reachable only from a
        // Clear* verb against a shape that has nothing to clear — every declaration re-mints its
        // description id, so a re-declaration always reads as a change.
        return;
    }

    if (TryApplyShapingDeltaInPlace(delta))
    {
        RaiseShapingChanged(true /* reorderOnly */);
        return;
    }

    Refresh();
    RaiseShapingChanged(false /* reorderOnly */);
}

// Scope: UNGROUPED sort-only changes. Grouped sort still rebuilds through the group adapter,
// because a sort declared before the group verb reorders the GROUPS, which layer 1's
// within-bucket sort cannot express. Doing that incrementally would mean teaching the adapter to
// consume a ReBucket / ReSortWithinBuckets delta, which this does not attempt.
//
// What it saves is model-side work: re-materializing the source, re-running the filter predicate
// over every row, and constructing a new ItemsSourceView and RowMetadataProvider. It does NOT
// avoid re-querying row identity — the identity/index map is ordinal, so a re-order invalidates
// it and RebuildFlatRowIdentityTracking re-projects identity over every row below. It
// also does NOT preserve realized containers — ReplaceAll is still a Reset, so the repeater
// re-realizes exactly as it would after a Refresh. Preserving containers across a re-order would
// require emitting Move notifications instead.
bool ShapedItemsSource::TryApplyShapingDeltaInPlace(ShapingHelpers::ShapingDelta const& delta)
{
    // Only a pure re-order is safe to do against rows already in hand. Anything touching
    // membership or bucketing needs the source, the group cache and the projection swap that
    // only Refresh performs.
    if (delta.RequiredWork != ShapingHelpers::ShapingWork::ReSortWithinBuckets)
    {
        return false;
    }

    // A grouped projection is rebuilt through the group adapter, and sorts declared before the
    // group verb reorder the GROUPS — which layer 1's within-bucket sort cannot express.
    if (m_groupSelector || m_projectedAsGrouped)
    {
        return false;
    }

    if (!m_rows || !m_shapingState.HasProjection || m_shapingState.IsGrouped)
    {
        return false;
    }

    // A rebuild is already going to run and will subsume this change.
    if (m_isRefreshing || m_isApplyingIncrementalChange || m_pendingRefresh)
    {
        return false;
    }

    // An unshaped mirror is not a shaped projection at all; re-sorting it in place would apply
    // shaping that Refresh deliberately refuses to apply. The m_shapingState.HasProjection test
    // above already rejects that case, and identity is never the blocker now --
    // EffectiveIdentitySelector always yields one -- so no further gate is needed here.

    // Prove the retained state still describes the live projection instead of trusting that every
    // mutation site remembered to invalidate it. The invalidation calls are the cheap first line
    // of defence; this is the one that makes a missed call — including from a splice site added
    // later — degrade to a full rebuild rather than silently re-sort a membership the projection
    // no longer has. O(n) reference comparisons against an O(n log n) sort that follows.
    if (m_shapingState.Items.size() != m_rows.Size())
    {
        return false;
    }
    for (uint32_t i = 0; i < m_rows.Size(); ++i)
    {
        if (m_shapingState.Items[i] != m_rows.GetAt(i))
        {
            return false;
        }
    }


    // Re-seats Items on FilteredSource (source order) and re-sorts, so ties break exactly as a
    // full reshape of the same spec would.
    ShapingHelpers::Reshape(m_shapingState, m_pipeline.CommittedSpec(), delta);

    m_rows.ReplaceAll(m_shapingState.Items);
    RebuildFlatRowIdentityTracking(m_shapingState.Items);
    return true;
}

void ShapedItemsSource::InvalidateShapingState()
{
    m_shapingState.HasProjection = false;
    m_shapingState.FilteredSource.clear();
    m_shapingState.Items.clear();
    m_shapingState.Buckets.clear();
    m_shapingState.IsGrouped = false;
}

void ShapedItemsSource::SubscribeToSourceCollectionChanges()
{
    UnsubscribeFromSourceCollectionChanges();

    // Classify the source once, here, where it is bound. Every later indexed read, count and
    // observability check reads off this resolution instead of re-probing.
    m_sourceAccessor = ShapingHelpers::CollectionAccessor{ m_source };

    if (!m_source)
    {
        return;
    }

    auto weakThis = weak_from_this();
    auto applyVectorChange = [weakThis](auto&&, auto&& args)
    {
        if (auto strongThis = weakThis.lock())
        {
            if (auto vectorArgs = TryAsVectorChangedArgs(args))
            {
                strongThis->OnSourceVectorChanged(vectorArgs);
            }
            else
            {
                strongThis->OnSourceCollectionChanged();
            }
        }
    };

    if (auto const& collection = m_sourceAccessor.AsNotifyCollectionChanged())
    {
        // INotifyCollectionChanged carries per-change details (.NET ObservableCollection<T>),
        // so route it through the incremental fast-path.
        m_sourceCollectionChangedRevoker = collection.CollectionChanged(winrt::auto_revoke,
            [weakThis](auto&&, winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
            {
                if (auto strongThis = weakThis.lock())
                {
                    strongThis->OnSourceCollectionChanged(args);
                }
            });
    }
    else if (auto const& vector = m_sourceAccessor.AsObservableVector())
    {
        // VectorChanged carries a single index + verb. For flat 1:1 projections, keep this
        // incremental instead of collapsing to ReplaceAll.
        m_sourceVectorChangedRevoker = vector.VectorChanged(winrt::auto_revoke, applyVectorChange);
    }
    else if (auto const& bindableVector = m_sourceAccessor.AsBindableObservableVector())
    {
        m_sourceBindableVectorChangedRevoker = bindableVector.VectorChanged(winrt::auto_revoke, applyVectorChange);
    }
}

void ShapedItemsSource::UnsubscribeFromSourceCollectionChanges()
{
    // Teardown runs on the owning UI thread: the projected owner (TableViewSource) is
    // reference-tracked, so its final_release marshals the whole delete -- and thus this
    // destructor-driven revoke -- back to the DispatcherQueue it was constructed on. The revoke
    // therefore never lands on the GC finalizer thread, so no thread guard is needed here.
    // SafeRevoke still swallows the benign failure an app can cause by tearing the publisher down
    // first. (Every handler also holds a weak reference, so an un-revoked subscription is already
    // inert once this object dies.)
    ShapingHelpers::SafeRevoke(m_sourceCollectionChangedRevoker);
    ShapingHelpers::SafeRevoke(m_sourceVectorChangedRevoker);
    ShapingHelpers::SafeRevoke(m_sourceBindableVectorChangedRevoker);
}

void ShapedItemsSource::OnSourceCollectionChanged()
{
    // UI-thread contract: like every XAML items source, this engine requires its underlying
    // collection to raise change notifications on the UI thread -- m_rows is an observable the
    // control binds to, and the identity index must stay in lockstep with it. A background-thread
    // notification is app misuse and is not supported.
    Refresh();
}

void ShapedItemsSource::OnSourceCollectionChanged(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
{
    // See the UI-thread contract on OnSourceCollectionChanged(): incremental InsertAt/RemoveAt/
    // SetAt below mutate the UI-affine projection directly, so they must run on the owning thread.

    // Re-entrant during a full rebuild: the in-flight Refresh() re-materializes the live source
    // when it completes, but changes after its initial materialization still need one coalesced
    // follow-up rebuild after the outer rebuild unwinds.
    if (m_isRefreshing)
    {
        m_pendingRefresh = true;
        return;
    }

    // Re-entrant during an incremental application: a synchronous VectorChanged handler (fired by
    // the InsertAt/RemoveAt/SetAt below) mutated the source. We cannot safely interleave a nested
    // incremental update against the half-updated projection/identity set (e.g. mid-Replace, after
    // the remove but before the insert). Defer a single full rebuild to run once the outer
    // application unwinds; Refresh() re-materializes live source and reseeds the identity set,
    // producing a consistent final projection.
    if (m_isApplyingIncrementalChange)
    {
        m_pendingRefresh = true;
        return;
    }

    {
        m_isApplyingIncrementalChange = true;
        auto guard = wil::scope_exit([this]() noexcept { m_isApplyingIncrementalChange = false; });
        ApplyIncrementalChange(args);
    }

    // A notification re-entered while we were applying: now that the projection/identity
    // invariants are consistent again, run exactly one deferred rebuild against live source.
    if (m_pendingRefresh)
    {
        m_pendingRefresh = false;
        Refresh();
    }
}

void ShapedItemsSource::OnSourceVectorChanged(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
{
    // See the UI-thread contract on OnSourceCollectionChanged().

    if (m_isRefreshing)
    {
        m_pendingRefresh = true;
        return;
    }

    if (m_isApplyingIncrementalChange)
    {
        m_pendingRefresh = true;
        return;
    }

    {
        m_isApplyingIncrementalChange = true;
        auto guard = wil::scope_exit([this]() noexcept { m_isApplyingIncrementalChange = false; });
        ApplyIncrementalVectorChange(args);
    }

    if (m_pendingRefresh)
    {
        m_pendingRefresh = false;
        Refresh();
    }
}

void ShapedItemsSource::ApplyIncrementalChange(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
{

    // Every path below either mutates m_rows without going through Refresh or falls back to
    // Refresh. The first leaves the retained layer-1 membership describing a projection that no
    // longer exists, so drop it up front rather than at each of the mutation sites; Refresh
    // repopulates it, and the in-place shaping path refuses to run without it.
    InvalidateShapingState();

    using winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;

    // A rebuild in flight, a grouped projection, or a not-yet-materialized projection -> full
    // rebuild (the incremental paths need a live flat projection + its view/metadata).
    if (m_isRefreshing || m_groupSelector || !m_rows || m_kind == ProjectionKind::None)
    {
        Refresh();
        return;
    }

    // SORTED flat projection (the Task Manager scenario): a single-item Add/Remove/Replace is
    // applied by binary-search insertion / find-remove in place, avoiding a full O(n)
    // re-materialize + re-sort + re-hash on every underlying change. Optional filter is applied
    // as an admission gate. Falls back to a full rebuild for anything it can't apply exactly.
    //
    // Invariant: re-sorting is driven by collection notifications on this path. An in-place
    // mutation of a row's sort-key field that raises only INotifyPropertyChanged leaves the row
    // at its old sort position until the next collection change, matching XAML ItemsControl
    // sources.
    if (HasActiveSort())
    {
        // Only take the in-place sorted fast-path when a shaped flat projection is active
        // (a Flat projection). After a safe-degrade (an Unshaped projection leaves an UNSORTED
        // mirror in m_rows while a sort is still configured), a binary-search insert would splice
        // into an unsorted list at a bogus index, so fall through to a full Refresh() that
        // re-shapes (or re-degrades) coherently.
        if (m_kind == ProjectionKind::Flat && TryApplyIncrementalSortedChange(args))
        {
            return;
        }
        Refresh();
        return;
    }

    // Filter-only (no sort): the projection is source-order-among-kept, so an incremental insert
    // position isn't a simple source index; rebuild for correctness.
    if (m_pipeline.HasFilter())
    {
        Refresh();
        return;
    }

    // UNSHAPED flat projection: mirrors the source 1:1, so a single-item change maps directly
    // to the same index in the projection observable. When stable row metadata is active, keep
    // the identity set/index map in sync; degraded/no-identity flat projections can still splice
    // by index because no identity metadata is exposed.
    const bool identityRequired = IsIdentityRequired() && m_kind != ProjectionKind::Unshaped;
    switch (args.Action())
    {
    case NotifyCollectionChangedAction::Add:
        if (auto const newItems = args.NewItems(); newItems && newItems.Size() == 1 && args.NewStartingIndex() >= 0)
        {
            const auto index = static_cast<uint32_t>(args.NewStartingIndex());
            if (index <= m_rows.Size())
            {
                auto const item = newItems.GetAt(0);
                if (identityRequired)
                {
                    winrt::hstring identity;
                    wchar_t const* reason = nullptr;
                    if (!TryGetRequiredRowIdentity(item, identity, reason) ||
                        !m_flatRowIdentities.insert(identity).second)
                    {
                        break; // empty/duplicate identity -> Refresh() re-validates and fails fast
                    }
                    ShiftTrackedFlatRowIndicesForInsert(index);
                    if (!m_flatRowIdentityToIndex.emplace(identity, index).second)
                    {
                        break; // stale identity/index tracking -> Refresh() reseeds it
                    }
                }
                m_rows.InsertAt(index, item);
                return;
            }
        }
        break;
    case NotifyCollectionChangedAction::Remove:
        if (args.OldStartingIndex() >= 0)
        {
            const auto index = static_cast<uint32_t>(args.OldStartingIndex());
            if (auto const oldItems = args.OldItems(); oldItems && oldItems.Size() == 1 && index < m_rows.Size())
            {
                if (identityRequired)
                {
                    winrt::hstring identity;
                    wchar_t const* reason = nullptr;
                    uint32_t trackedIndex = 0;
                    if (!TryGetRequiredRowIdentity(oldItems.GetAt(0), identity, reason) ||
                        !TryGetTrackedFlatRowIndex(identity, trackedIndex) ||
                        trackedIndex != index)
                    {
                        break; // stale identity/index tracking -> Refresh() reseeds it
                    }
                    m_flatRowIdentities.erase(identity);
                    m_flatRowIdentityToIndex.erase(identity);
                    ShiftTrackedFlatRowIndicesForRemove(index);
                }
                m_rows.RemoveAt(index);
                return;
            }
        }
        break;
    case NotifyCollectionChangedAction::Replace:
        if (auto const newItems = args.NewItems(); newItems && newItems.Size() == 1 && args.NewStartingIndex() >= 0)
        {
            const auto index = static_cast<uint32_t>(args.NewStartingIndex());
            if (index < m_rows.Size())
            {
                auto const newItem = newItems.GetAt(0);
                if (identityRequired)
                {
                    // Retire the outgoing row's identity, then admit the incoming one. An empty or
                    // duplicate incoming identity falls back to a full rebuild (which reseeds the
                    // set, so the transient erase below is harmless).
                    winrt::hstring oldIdentity;
                    wchar_t const* oldReason = nullptr;
                    if (TryGetRequiredRowIdentity(m_rows.GetAt(index), oldIdentity, oldReason))
                    {
                        uint32_t trackedIndex = 0;
                        if (!TryGetTrackedFlatRowIndex(oldIdentity, trackedIndex) ||
                            trackedIndex != index)
                        {
                            break; // stale identity/index tracking -> Refresh() reseeds it
                        }
                        m_flatRowIdentities.erase(oldIdentity);
                        m_flatRowIdentityToIndex.erase(oldIdentity);
                    }
                    else
                    {
                        break; // missing outgoing identity -> Refresh() re-validates
                    }
                    winrt::hstring newIdentity;
                    wchar_t const* newReason = nullptr;
                    if (!TryGetRequiredRowIdentity(newItem, newIdentity, newReason) ||
                        !m_flatRowIdentities.insert(newIdentity).second)
                    {
                        break; // empty/duplicate identity -> Refresh() re-validates and fails fast
                    }
                    if (!m_flatRowIdentityToIndex.emplace(newIdentity, index).second)
                    {
                        break; // stale identity/index tracking -> Refresh() reseeds it
                    }
                }
                m_rows.SetAt(index, newItem);
                return;
            }
        }
        break;
    default:
        break;
    }

    // Move / Reset / multi-item / out-of-range: atomic full rebuild.
    Refresh();
}

void ShapedItemsSource::ApplyIncrementalVectorChange(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
{

    // Same reasoning as ApplyIncrementalChange: the retained membership stops describing m_rows
    // the moment this splices it.
    InvalidateShapingState();

    using winrt::Windows::Foundation::Collections::CollectionChange;

    // VectorChanged has only a verb + index (no OldItems/NewItems). Keep the low-risk fast path
    // to flat 1:1 projections, where the source index is the projection index and the current
    // source/projection can provide the one item needed to splice m_rows and identity tracking.
    if (m_isRefreshing || m_groupSelector || HasActiveSort() || m_pipeline.HasFilter() ||
        !m_rows || m_kind == ProjectionKind::None)
    {
        Refresh();
        return;
    }

    const uint32_t index = args.Index();
    const bool identityRequired = IsIdentityRequired() && m_kind != ProjectionKind::Unshaped;
    switch (args.CollectionChange())
    {
    case CollectionChange::ItemInserted:
    {
        if (index > m_rows.Size())
        {
            break;
        }

        winrt::IInspectable item{ nullptr };
        if (!m_sourceAccessor.TryGetAt(index, item))
        {
            break;
        }

        if (identityRequired)
        {
            winrt::hstring identity;
            wchar_t const* reason = nullptr;
            if (!TryGetRequiredRowIdentity(item, identity, reason) ||
                !m_flatRowIdentities.insert(identity).second)
            {
                break;
            }
            ShiftTrackedFlatRowIndicesForInsert(index);
            if (!m_flatRowIdentityToIndex.emplace(identity, index).second)
            {
                break;
            }
        }

        m_rows.InsertAt(index, item);
        return;
    }
    case CollectionChange::ItemRemoved:
    {
        if (index >= m_rows.Size())
        {
            break;
        }

        const auto item = m_rows.GetAt(index);
        if (identityRequired)
        {
            winrt::hstring identity;
            wchar_t const* reason = nullptr;
            uint32_t trackedIndex = 0;
            if (!TryGetRequiredRowIdentity(item, identity, reason) ||
                !TryGetTrackedFlatRowIndex(identity, trackedIndex) ||
                trackedIndex != index)
            {
                break;
            }
            m_flatRowIdentities.erase(identity);
            m_flatRowIdentityToIndex.erase(identity);
            ShiftTrackedFlatRowIndicesForRemove(index);
        }

        m_rows.RemoveAt(index);
        return;
    }
    case CollectionChange::ItemChanged:
    {
        if (index >= m_rows.Size())
        {
            break;
        }

        winrt::IInspectable newItem{ nullptr };
        if (!m_sourceAccessor.TryGetAt(index, newItem))
        {
            break;
        }

        if (identityRequired)
        {
            winrt::hstring oldIdentity;
            wchar_t const* oldReason = nullptr;
            uint32_t trackedIndex = 0;
            if (!TryGetRequiredRowIdentity(m_rows.GetAt(index), oldIdentity, oldReason) ||
                !TryGetTrackedFlatRowIndex(oldIdentity, trackedIndex) ||
                trackedIndex != index)
            {
                break;
            }
            m_flatRowIdentities.erase(oldIdentity);
            m_flatRowIdentityToIndex.erase(oldIdentity);

            winrt::hstring newIdentity;
            wchar_t const* newReason = nullptr;
            if (!TryGetRequiredRowIdentity(newItem, newIdentity, newReason) ||
                !m_flatRowIdentities.insert(newIdentity).second)
            {
                break;
            }
            if (!m_flatRowIdentityToIndex.emplace(newIdentity, index).second)
            {
                break;
            }
        }

        m_rows.SetAt(index, newItem);
        return;
    }
    case CollectionChange::Reset:
    default:
        break;
    }

    Refresh();
}

bool ShapedItemsSource::TryApplyIncrementalSortedChange(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
{

    using winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;

    const bool identityRequired = IsIdentityRequired();

    // A stable sort breaks ties by SOURCE order, and the sorted projection does not carry the
    // source index of each row, so an item that lands inside a tie group cannot be placed
    // incrementally — except when it is the last item of the source, where "after every equal-key
    // row" is exactly what source order demands. Anything else falls back to a full rebuild,
    // which re-derives the tie order from the retained source.
    const auto isLastSourceIndex = [&](int32_t startingIndex)
    {
        uint32_t sourceCount = 0;
        return startingIndex >= 0 &&
            TryGetSourceItemCount(sourceCount) &&
            sourceCount > 0 &&
            static_cast<uint32_t>(startingIndex) == sourceCount - 1;
    };

    // Insert one item into the sorted projection (filter-gated, identity-checked). Returns
    // false to force a full rebuild (empty/duplicate identity, or an ambiguous tie position ->
    // re-validate + safe-degrade).
    auto tryInsert = [&](winrt::IInspectable const& item, bool tiesResolvableByAppend) -> bool
    {
        if (!item)
        {
            return false;
        }
        if (!m_pipeline.PassesFilter(item))
        {
            return true; // filtered out: projection unchanged
        }

        const auto placement = SortedInsertPlacementFor(item);
        if (placement.TiedWithExistingRow && !tiesResolvableByAppend)
        {
            return false; // source order decides this position -> full rebuild
        }

        if (identityRequired)
        {
            winrt::hstring identity;
            wchar_t const* reason = nullptr;
            if (!TryGetRequiredRowIdentity(item, identity, reason))
            {
                return false; // missing identity -> full rebuild will safe-degrade
            }
            if (!m_flatRowIdentities.insert(identity).second)
            {
                return false; // duplicate identity -> full rebuild will safe-degrade
            }
            ShiftTrackedFlatRowIndicesForInsert(placement.Index);
            if (!m_flatRowIdentityToIndex.emplace(identity, placement.Index).second)
            {
                return false; // stale identity/index map -> full rebuild will reseed it
            }
            m_rows.InsertAt(placement.Index, item);
            return true;
        }
        m_rows.InsertAt(placement.Index, item);
        return true;
    };

    // Remove one item from the projection by tracked identity; keep the identity index map in
    // sync. Returns false to force a full rebuild when consistency can't be proven.
    auto tryRemove = [&](winrt::IInspectable const& item) -> bool
    {
        if (!item)
        {
            return false;
        }
        if (identityRequired)
        {
            winrt::hstring identity;
            wchar_t const* reason = nullptr;
            uint32_t index = 0;
            // The identity is recomputed from the (current) item. If the item's identity key was
            // mutated in place before this notification, the recomputed identity won't match the
            // tracked identity/index; force a full rebuild, which reseeds the tracking from
            // scratch. Only remove incrementally when the map still points at a row with the same
            // identity, avoiding m_rows.IndexOf(item)'s O(n) WinRT ABI scan.
            if (!TryGetRequiredRowIdentity(item, identity, reason) ||
                !TryGetTrackedFlatRowIndex(identity, index))
            {
                return false;
            }
            m_flatRowIdentities.erase(identity);
            m_flatRowIdentityToIndex.erase(identity);
            ShiftTrackedFlatRowIndicesForRemove(index);
            m_rows.RemoveAt(index);
            return true;
        }

        return false;
    };

    switch (args.Action())
    {
    case NotifyCollectionChangedAction::Add:
        if (auto const newItems = args.NewItems(); newItems && newItems.Size() == 1)
        {
            return tryInsert(newItems.GetAt(0), isLastSourceIndex(args.NewStartingIndex()));
        }
        return false;
    case NotifyCollectionChangedAction::Remove:
        if (auto const oldItems = args.OldItems(); oldItems && oldItems.Size() == 1)
        {
            return tryRemove(oldItems.GetAt(0));
        }
        return false;
    case NotifyCollectionChangedAction::Replace:
    {
        auto const oldItems = args.OldItems();
        auto const newItems = args.NewItems();
        if (oldItems && newItems && oldItems.Size() == 1 && newItems.Size() == 1)
        {
            // Remove the old row, then re-insert the new value at its (possibly changed) sort
            // position — this also covers a same-object value change that re-orders the row.
            if (!tryRemove(oldItems.GetAt(0)))
            {
                return false;
            }
            return tryInsert(newItems.GetAt(0), isLastSourceIndex(args.NewStartingIndex()));
        }
        return false;
    }
    case NotifyCollectionChangedAction::Move:
    {
        // A sorted projection is independent of source order EXCEPT for tie order: rows the sort
        // cannot distinguish keep their source order, so moving one of them past another really
        // does reorder the projection. Ties are contiguous, so only the moved row's two sorted
        // neighbours have to be probed; if it has none, the move is genuinely invisible here.
        auto const movedItems = args.NewItems();
        if (!movedItems || movedItems.Size() != 1)
        {
            return false;
        }
        auto const moved = movedItems.GetAt(0);
        if (!moved || !identityRequired)
        {
            return false; // can't locate the row cheaply -> full rebuild
        }

        winrt::hstring identity;
        wchar_t const* reason = nullptr;
        uint32_t index = 0;
        if (!TryGetRequiredRowIdentity(moved, identity, reason) ||
            !TryGetTrackedFlatRowIndex(identity, index))
        {
            return false;
        }

        const uint32_t rowCount = m_rows.Size();
        if (index >= rowCount)
        {
            return false;
        }
        if (index > 0 && m_pipeline.CompareItemToRow(moved, m_rows.GetAt(index - 1)) == 0)
        {
            return false;
        }
        if (index + 1 < rowCount && m_pipeline.CompareItemToRow(moved, m_rows.GetAt(index + 1)) == 0)
        {
            return false;
        }
        return true;
    }
    default:
        return false; // Reset / multi-item -> full rebuild
    }
}


bool ShapedItemsSource::HasActiveSort() const
{
    return m_pipeline.HasActiveSort();
}

bool ShapedItemsSource::IsSourceMutable() const
{
    return m_sourceAccessor.IsObservable();
}

bool ShapedItemsSource::IsIdentityRequired() const
{
    return m_groupSelector || m_childrenSelector || m_pipeline.HasFilter() || HasActiveSort() || IsSourceMutable();
}

bool ShapedItemsSource::HasAnyShapingVerb() const
{
    return m_pipeline.HasFilter() || m_groupSelector || m_childrenSelector || HasActiveSort();
}

bool ShapedItemsSource::TryGetRequiredRowIdentity(
    winrt::IInspectable const& item,
    winrt::hstring& identity,
    wchar_t const*& reason) const
{
    return RowIdentity::TryGetRequiredRowIdentity(item, EffectiveIdentitySelector(), identity, reason);
}

bool ShapedItemsSource::ValidateRowIdentities(
    std::vector<winrt::IInspectable> const& rows,
    wchar_t const*& reason) const
{
    return RowIdentity::ValidateRowIdentities(rows, EffectiveIdentitySelector(), reason);
}

void ShapedItemsSource::ClearFlatRowIdentityTracking()
{
    RowIdentity::ClearFlatRowIdentityTracking(m_flatRowIdentities, m_flatRowIdentityToIndex);
}

void ShapedItemsSource::RebuildFlatRowIdentityTracking(std::vector<winrt::IInspectable> const& rows)
{
    RowIdentity::RebuildFlatRowIdentityTracking(
        rows,
        IsIdentityRequired(),
        EffectiveIdentitySelector(),
        m_flatRowIdentities,
        m_flatRowIdentityToIndex);
}

bool ShapedItemsSource::TryGetTrackedFlatRowIndex(winrt::hstring const& identity, uint32_t& index) const
{
    return RowIdentity::TryGetTrackedFlatRowIndex(
        identity,
        m_rows,
        EffectiveIdentitySelector(),
        m_flatRowIdentityToIndex,
        index);
}

void ShapedItemsSource::ShiftTrackedFlatRowIndicesForInsert(uint32_t insertedIndex)
{
    // Pure-append fast path. Callers invoke this BEFORE m_rows.InsertAt(insertedIndex, item), so
    // m_rows.Size() reflects the pre-insert row count and every tracked identity's index lies in
    // [0, m_rows.Size()). When insertedIndex >= m_rows.Size() the change is a tail append and
    // nothing existing satisfies entry.second >= insertedIndex — so the O(n) walk is guaranteed
    // to be a no-op. Skipping avoids the sweep on every append and prevents bulk-load (N sequential
    // appends) from degrading to O(N^2). Using m_rows.Size() rather than m_flatRowIdentityToIndex
    // .size() is intentional: the identity map can be strictly smaller than m_rows when a row was
    // skipped by RebuildFlatRowIdentityTracking, so the map's size is NOT a safe upper bound.
    if (m_rows && insertedIndex >= m_rows.Size())
    {
        return;
    }
    RowIdentity::ShiftTrackedFlatRowIndicesForInsert(m_flatRowIdentityToIndex, insertedIndex);
}

void ShapedItemsSource::ShiftTrackedFlatRowIndicesForRemove(uint32_t removedIndex)
{
    // Pure-tail-remove fast path. Callers invoke this BEFORE m_rows.RemoveAt(removedIndex), so
    // m_rows.Size() reflects the pre-remove row count and every tracked identity's index lies in
    // [0, m_rows.Size()). When removedIndex + 1 >= m_rows.Size() the removal is at the tail — no
    // remaining entry can have entry.second > removedIndex, so the O(n) walk is a guaranteed
    // no-op. As with the Insert path, m_rows.Size() (not the identity map's size) is the safe
    // upper bound because untracked rows can leave holes in the identity map.
    if (m_rows && removedIndex + 1 >= m_rows.Size())
    {
        return;
    }
    RowIdentity::ShiftTrackedFlatRowIndicesForRemove(m_flatRowIdentityToIndex, removedIndex);
}

bool ShapedItemsSource::TryGetGroupIdentity(
    winrt::IInspectable const& key,
    winrt::hstring& identity,
    wchar_t const*& reason) const
{
    return RowIdentity::TryGetGroupIdentity(key, m_groupIdentitySelector, identity, reason);
}

void ShapedItemsSource::RebuildUnshapedRows(std::vector<winrt::IInspectable> const& rows, wchar_t const* reason)
{
    LogIdentityProjectionDisabled(reason);

    // An unshaped mirror is not a shaped projection: no filter or sort was applied, so there is
    // no layer-1 membership to re-sort in place later.
    InvalidateShapingState();

    m_rows.ReplaceAll(rows);
    m_kind = ProjectionKind::Unshaped;

    // A grouped/degraded projection does not use the flat incremental fast-path.
    ClearFlatRowIdentityTracking();

    ReleaseHierarchyProjection();

    if (m_groupSource)
    {
        // Detach the adapter BEFORE clearing the internal group source. Otherwise Clear() fires
        // the adapter's outer-source subscription, which rebuilds synchronously and raises an
        // empty Reset into the ItemsRepeater still bound to the old grouped Entries (with realized
        // rows) -- an assertion failure / fault mid-teardown. RaiseProjectionRebuilt below is the
        // single controlled swap that moves the row axis to the flat projection.
        if (m_groupedAdapter)
        {
            m_groupedAdapter->DetachSourceQuietly();
        }
        m_groupSource.Clear();
    }
    m_groupCache.clear();
    m_projectedAsGrouped = false;
    RaiseProjectionRebuilt();
}

void ShapedItemsSource::Refresh()
{

    // Re-entrancy guard: a source notification that arrives while a rebuild is in flight
    // (e.g. an app mutating the source from a filter/sort/group callback) must not re-enter
    // ReplaceAll on the projection. Remember it and run one coalesced rebuild after the outer
    // rebuild unwinds so changes after materialization are not lost.
    if (m_isRefreshing)
    {
        m_pendingRefresh = true;
        return;
    }

    bool const wasProjectedAsGrouped = m_projectedAsGrouped;
    bool runPendingRefresh = false;
    {
        m_isRefreshing = true;
        m_pendingRefresh = false;
        auto refreshGuard = wil::scope_exit([this, &runPendingRefresh]() noexcept
        {
            m_isRefreshing = false;
            runPendingRefresh = m_pendingRefresh;
            m_pendingRefresh = false;
        });

        auto const authoritativeSource = m_source;
        auto rows = Materialize(authoritativeSource);

        if (!HasAnyShapingVerb())
        {
            // Nothing is being shaped, so this is a plain mirror of the source. Identity buys
            // nothing here -- there is no reordering to anchor against and no membership change to
            // splice surgically -- and minting it would cost a QI plus a string format per row on
            // every refresh of a table that asked for none of it.
            RebuildUnshapedRows(rows, L"no shaping verb");
        }
        else
        {
            ApplyFilter(rows);

            // A shaping verb is in force here (the branch above took the no-verb case), and a verb
            // always requires identity, so there is nothing to gate on.
            wchar_t const* reason = nullptr;
            if (!ValidateRowIdentities(rows, reason))
            {
                LogIdentityProjectionDisabled(reason);

                // Identity is derived from each item's object identity, which is unique among live
                // objects, so the expected failure is one object occupying more than one row --
                // there is no app-authored selector to blame and nothing to disambiguate with.
                // A row that cannot produce an identity at all lands here too (a null item, say),
                // and must not be reported as a duplicate.
                constexpr std::wstring_view c_duplicateObjectReason{ L"the same item object appears on more than one row" };
                if (reason && c_duplicateObjectReason == reason)
                {
                    throw winrt::hresult_invalid_argument(
                        Diagnostic(
                            L"The same item object appears in the source more than once. Rows are "
                            L"identified by object identity, so two rows backed by one object cannot "
                            L"be told apart. Use a distinct object per row."));
                }

                winrt::hstring message = Diagnostic(L"A row could not be given a stable identity");
                if (reason)
                {
                    message = message + L": " + winrt::hstring{ reason };
                }
                throw winrt::hresult_invalid_argument(message);
            }

            if (m_childrenSelector && m_groupSelector)
            {
                RebuildGroupedHierarchical(rows);
            }
            else if (m_childrenSelector)
            {
                RebuildHierarchical(rows);
            }
            else if (m_groupSelector)
            {
                RebuildGrouped(rows);
            }
            else
            {
                RebuildFlat(rows);
            }
        }
        MUX_ASSERT(m_source == authoritativeSource);
    }

    if (runPendingRefresh)
    {
        if (m_projectedAsGrouped != wasProjectedAsGrouped)
        {
            RaiseShapeSwapped();
        }
        Refresh();
        return;
    }

    // Grouped <-> flat swaps the ItemsSourceView and the row-metadata provider. The owning
    // TableView cached both (plus grouped-ness) when it bound, so without this it would keep
    // projecting the previous shape - and would read a raw item as a GroupedEntry, or miss the
    // group-header rows entirely.
    if (m_projectedAsGrouped != wasProjectedAsGrouped)
    {
        RaiseShapeSwapped();
    }
}

void ShapedItemsSource::RebuildFlat(std::vector<winrt::IInspectable>& rows)
{

    // Retain the post-filter membership in SOURCE order before sorting. A later sort-only change
    // re-seats on this rather than on the already-sorted output, so its stable sort breaks ties
    // the same way a full rebuild of that spec would.
    m_shapingState.FilteredSource = rows;

    ApplySort(rows);

    m_shapingState.Items = rows;
    m_shapingState.Buckets.clear();
    m_shapingState.IsGrouped = false;
    m_shapingState.HasProjection = true;

    m_rows.ReplaceAll(rows);
    m_kind = ProjectionKind::Flat;

    // Seed the identity tracking that the incremental fast-path maintains, so it can detect
    // duplicate/empty identities and locate sorted removes without O(n) WinRT IndexOf scans.
    RebuildFlatRowIdentityTracking(rows);

    ReleaseHierarchyProjection();

    // Releasing any prior grouped projection: switching grouped->flat must not retain the stale
    // group observable/cache. They are rebuilt from scratch by RebuildGrouped on the next GroupBy,
    // so holding them here only leaks the previous grouping (and its cached ShapedGroups).
    if (m_groupSource)
    {
        // Detach the adapter before Clear() so its subscription does not re-enter Rebuild()
        // synchronously and Reset the ItemsRepeater still bound to the old grouped Entries.
        // RaiseProjectionRebuilt below performs the single controlled swap to the flat row axis.
        if (m_groupedAdapter)
        {
            m_groupedAdapter->DetachSourceQuietly();
        }
        m_groupSource.Clear();
    }
    m_groupCache.clear();
    m_projectedAsGrouped = false;
    RaiseProjectionRebuilt();
}

std::vector<winrt::IInspectable> ShapedItemsSource::Materialize(winrt::IInspectable const& source)
{
    return ShapingHelpers::EnumerateInspectableItems(source, true);
}

void ShapedItemsSource::RebuildGrouped(std::vector<winrt::IInspectable>& rows)
{
    // The grouped projection is materialized through the group adapter, not from the retained
    // layer-1 state, so leaving that state live would let the in-place path re-sort a flat
    // projection that is no longer the one being shown.
    InvalidateShapingState();
    // A grouped projection does not use the flat incremental fast-path.
    ClearFlatRowIdentityTracking();
    // Plain grouping: no hierarchy axis, so any adapter a previous grouped+hierarchical projection
    // left behind must go before the group slices are rebuilt from the flat rows.
    ReleaseHierarchyProjection();
    // Sorts requested before GroupBy establish the group order. Sorts requested after GroupBy
    // are applied per bucket below, preserving the group order while sorting within each group.
    ApplySort(rows, -1, m_pipeline.GroupOrder());

    std::vector<ShapingHelpers::KeyedBucket> keyedBuckets;
    wchar_t const* degradeReason = nullptr;
    const bool grouped = ShapingHelpers::BucketizeToGroups(
        rows,
        [this](winrt::IInspectable const& item) -> winrt::IInspectable
        {
            try { return m_groupSelector(item); }
            catch (...) { return nullptr; }
        },
        [this](winrt::IInspectable const& key, winrt::hstring& identity, wchar_t const*& reason)
        {
            return TryGetGroupIdentity(key, identity, reason);
        },
        [this](winrt::IInspectable const& existingKey, winrt::IInspectable const& newKey)
        {
            // Not a collision when the app supplied a groupIdentitySelector (the identity is
            // authoritative, so two distinct key instances mapping to the same identity is
            // intentional — e.g. a per-item composite key) or the keys are genuinely equal.
            return m_groupIdentitySelector || RowIdentity::GroupKeysEqual(existingKey, newKey);
        },
        keyedBuckets,
        degradeReason);

    if (!grouped)
    {
        // Spec contract (same shape as the row-identity fail-fast): an unresolvable,
        // unstable, or colliding *group* identity is a caller bug — the GroupBy(...) key
        // selector (and optional groupIdentitySelector) must produce a stable, non-empty
        // string identity per bucket, without two distinct group-key instances collapsing
        // to the same identity unless the app opted in via groupIdentitySelector. Silently
        // flattening the projection would let the app ship with grouping mysteriously "not
        // working" and no diagnostic.
        MUX_ASSERT_MSG(false,
            L"GroupBy key selector produced an invalid group identity "
            L"(empty, non-string, throwing, or two distinct group keys collapsing to the "
            L"same identity without a groupIdentitySelector opt-in). Fix the GroupBy(...) "
            L"selector so every group has a stable non-empty unique string identity, or "
            L"supply a groupIdentitySelector that resolves the collision intentionally. "
            L"See the per-bucket reason string logged via LogIdentityProjectionDisabled.");
        LogIdentityProjectionDisabled(degradeReason);
        winrt::hstring message = Diagnostic(L"GroupBy key selector produced an invalid group identity");
        if (degradeReason)
        {
            message = message + L": " + winrt::hstring{ degradeReason };
        }
        throw winrt::hresult_invalid_argument(message);
    }

    if (!m_groupSource)
    {
        m_groupSource = winrt::single_threaded_observable_vector<winrt::IInspectable>();
    }

    std::vector<winrt::IInspectable> groups;
    groups.reserve(keyedBuckets.size());
    std::unordered_set<winrt::hstring> liveKeys;

    // The flat shaped rows kept under grouping: every kept row, in group order, with the
    // per-bucket sort applied and no header entries. Accumulated here rather than re-derived
    // afterwards because this loop already walks the buckets in their final order. This keeps
    // Rows() coherent as the flat shaped projection even while the presented row axis is the
    // grouped adapter.
    std::vector<winrt::IInspectable> flatRows;
    flatRows.reserve(rows.size());

    for (auto& bucket : keyedBuckets)
    {
        auto const& keyString = bucket.Identity;
        ApplySort(bucket.Items, m_pipeline.GroupOrder(), -1);
        flatRows.insert(flatRows.end(), bucket.Items.begin(), bucket.Items.end());

        winrt::com_ptr<ShapedGroup> group;
        auto cacheIt = m_groupCache.find(keyString);
        if (cacheIt != m_groupCache.end())
        {
            // Deliberately keep the cached group's existing key object. The cache is keyed by
            // identity, so the incoming key is identity-equivalent to the one already held, but it
            // is a different object whenever the key selector minted a fresh one or the bucket
            // merged several equal keys. Rebinding it would churn the object ICollectionViewGroup
            // publishes as Group() on every reshape, for no gain.
            group = cacheIt->second;
        }
        else
        {
            group = winrt::make_self<ShapedGroup>(bucket.Key, bucket.Identity);
            m_groupCache.emplace(keyString, group);
        }

        group->GroupKey(bucket.Identity);
        group->SetItems(bucket.Items);
        groups.push_back(group.as<winrt::IInspectable>());
        liveKeys.insert(keyString);
    }

    for (auto it = m_groupCache.begin(); it != m_groupCache.end();)
    {
        if (liveKeys.find(it->first) == liveKeys.end())
        {
            it = m_groupCache.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (!m_groupedAdapter)
    {
        m_groupedAdapter = std::make_shared<GroupedSourceAdapter>();
    }
    else
    {
        // A prior grouped projection already attached the adapter to m_groupSource. Detach it
        // quietly so the ReplaceAll below fires NO subscription: the Source() re-attach then
        // rebuilds the adapter exactly once against the fully-populated groups. Leaving it
        // attached would rebuild twice (ReplaceAll's subscription -> synchronous Rebuild, then a
        // forced Refresh) and briefly publish the intermediate group set.
        m_groupedAdapter->DetachSourceQuietly();
    }

    // Populate m_groupSource BEFORE (re-)attaching so the adapter's subscribe + synchronous
    // Rebuild inside Source(value) sees a fully-loaded m_groupSource and publishes m_entries in
    // one coherent Reset. Attaching to an empty (or stale) m_groupSource and then loading it
    // caused an observable intermediate state: consumers saw the wrong entries, then a second
    // Reset after the rebuild.
    m_groupSource.ReplaceAll(groups);
    m_groupedAdapter->Source(m_groupSource);

    // The presented row axis under grouping is the adapter's computed ItemsSourceView, which has
    // no vector form. Keep Rows() maintained as the flat shaped projection anyway: without this
    // write m_rows would keep whatever the last FLAT rebuild left -- for the canonical
    // From(...).GroupBy(...) chain that is the raw, unshaped source -- so shaping verbs applied
    // AFTER GroupBy would not be reflected in the flat projection.
    //
    // Expansion is deliberately not applied: this is the shaped DATA. Collapsing a group hides
    // rows from the presented row axis without removing them from the projection, and a flat
    // vector whose size changed when a chevron is clicked would be reporting UI state.
    m_rows.ReplaceAll(flatRows);

    m_kind = ProjectionKind::Grouped;
    m_projectedAsGrouped = true;
    RaiseProjectionRebuilt();
}

void ShapedItemsSource::RebuildHierarchical(std::vector<winrt::IInspectable>& rows)
{
    // Same reasoning as RebuildGrouped: the presented rows are materialized by an adapter, not by
    // the retained layer-1 state, so leaving that state live would let the in-place path re-sort a
    // projection that is no longer the one being shown.
    InvalidateShapingState();
    ClearFlatRowIdentityTracking();

    // `rows` arrives already filtered by the caller. Sorting it here shapes the ROOT sibling set;
    // every deeper level is shaped by the ShapeSiblings callback in EnsureHierarchicalAdapter, as
    // the walk reaches it. There is no group order to respect, so this is a plain full sort.
    ApplySort(rows);

    // Ungrouped: nothing else owns the root order, so the roots are shaped here and the grouped
    // machinery stays dormant.
    EnsureHierarchicalAdapter(rows, true /* shapeRoots */);

    // Releasing any prior grouped projection, for the same reason RebuildFlat does: switching
    // grouped+hierarchical -> hierarchical must not leave the stale group observable live, or the
    // grouped adapter would keep publishing headers over a row axis that no longer has any.
    if (m_groupSource)
    {
        if (m_groupedAdapter)
        {
            m_groupedAdapter->DetachSourceQuietly();
        }
        m_groupSource.Clear();
    }
    m_groupCache.clear();
    m_hierarchyGroups.clear();
    m_rootBucketIndex.clear();

    // Rows() stays the flat shaped projection. Under a hierarchy the presented row axis is the
    // adapter's ItemsSourceView, but unlike grouping the VISIBLE row set is itself the meaningful
    // flat reading -- every visible row is a real data row -- so mirroring the adapter's entries is
    // both correct and what a consumer expects Rows() to say.
    //
    // This is a SNAPSHOT taken at rebuild time. A later expand/collapse splices the adapter's
    // entries without rebuilding the projection, so Rows() does not track expansion between
    // rebuilds. The presented row axis (the adapter's view) always does, which is what the control
    // and the row-metadata provider consume; Rows() is the shaped-data reading, and a consumer
    // needing live visible rows must read the adapter.
    m_rows.ReplaceAll(VisibleHierarchicalRows());

    m_kind = ProjectionKind::Hierarchical;
    // Not a grouped projection: there are no header rows, so a consumer asking "is this grouped"
    // must hear no, or it will look for a GroupedEntry at every index.
    m_projectedAsGrouped = false;
    RaiseProjectionRebuilt();
}

// Both axes. The row stream is:
//
//   [Header A] [rootA1] [rootA1's expanded subtree...] [rootA2] ... [Header B] [rootB1] ...
//
// The group key is applied to the ROOTS only. A descendant is never re-bucketed: its depth is what
// makes it a descendant, and a row cannot simultaneously be at depth 3 under its parent and at
// depth 0 under a header. So headers exist at the top level and nowhere else, which is also what a
// user means by "group the tree".
void ShapedItemsSource::RebuildGroupedHierarchical(std::vector<winrt::IInspectable>& rows)
{
    InvalidateShapingState();
    ClearFlatRowIdentityTracking();

    // Sorts declared BEFORE GroupBy establish the group order; sorts declared after are applied
    // within each bucket below. Identical split to RebuildGrouped -- the hierarchy changes what
    // happens to a bucket's members, not how buckets are ordered.
    ApplySort(rows, -1, m_pipeline.GroupOrder());

    std::vector<ShapingHelpers::KeyedBucket> keyedBuckets;
    wchar_t const* degradeReason = nullptr;
    const bool grouped = ShapingHelpers::BucketizeToGroups(
        rows,
        [this](winrt::IInspectable const& item) -> winrt::IInspectable
        {
            try { return m_groupSelector(item); }
            catch (...) { return nullptr; }
        },
        [this](winrt::IInspectable const& key, winrt::hstring& identity, wchar_t const*& reason)
        {
            return TryGetGroupIdentity(key, identity, reason);
        },
        [this](winrt::IInspectable const& existingKey, winrt::IInspectable const& newKey)
        {
            return m_groupIdentitySelector || RowIdentity::GroupKeysEqual(existingKey, newKey);
        },
        keyedBuckets,
        degradeReason);

    if (!grouped)
    {
        // Same fail-fast contract as RebuildGrouped: an unusable group identity is a caller bug,
        // and silently flattening would ship an app whose grouping mysteriously does nothing.
        MUX_ASSERT_MSG(false,
            L"GroupBy key selector produced an invalid group identity over a hierarchical source "
            L"(empty, non-string, throwing, or two distinct group keys collapsing to the same "
            L"identity without a groupIdentitySelector opt-in).");
        LogIdentityProjectionDisabled(degradeReason);
        winrt::hstring message = Diagnostic(L"GroupBy key selector produced an invalid group identity");
        if (degradeReason)
        {
            message = message + L": " + winrt::hstring{ degradeReason };
        }
        throw winrt::hresult_invalid_argument(message);
    }

    // Roots in final presentation order: bucket by bucket, sorted within each. Contiguity is
    // load-bearing -- the re-slice below finds each bucket's rows by walking the adapter's entries
    // once and cutting at every depth-0 row, which only works if a bucket's roots are adjacent.
    std::vector<winrt::IInspectable> orderedRoots;
    orderedRoots.reserve(rows.size());
    m_rootBucketIndex.clear();

    for (size_t bucketIndex = 0; bucketIndex < keyedBuckets.size(); ++bucketIndex)
    {
        auto& bucket = keyedBuckets[bucketIndex];
        ApplySort(bucket.Items, m_pipeline.GroupOrder(), -1);
        for (auto const& root : bucket.Items)
        {
            // Keyed by raw COM pointer, not by identity string: this map is consulted once per
            // depth-0 entry during the re-slice, and the entries ARE the same objects.
            m_rootBucketIndex[winrt::get_abi(root)] = bucketIndex;
            orderedRoots.push_back(root);
        }
    }

    // shapeRoots=false: the root order is the GROUP order, and letting the per-level callback sort
    // the roots among themselves would interleave the buckets again and destroy the contiguity the
    // re-slice depends on. Roots were already filtered by the caller and sorted per bucket above.
    EnsureHierarchicalAdapter(orderedRoots, false /* shapeRoots */);

    if (!m_groupSource)
    {
        m_groupSource = winrt::single_threaded_observable_vector<winrt::IInspectable>();
    }

    // One ShapedGroup per bucket, reusing the cache on the same terms as RebuildGrouped so a
    // reshape does not churn the object a group publishes as Group().
    std::vector<winrt::IInspectable> groups;
    groups.reserve(keyedBuckets.size());
    std::unordered_set<winrt::hstring> liveKeys;
    m_hierarchyGroups.clear();
    m_hierarchyGroups.reserve(keyedBuckets.size());

    for (auto const& bucket : keyedBuckets)
    {
        auto const& keyString = bucket.Identity;

        winrt::com_ptr<ShapedGroup> group;
        auto cacheIt = m_groupCache.find(keyString);
        if (cacheIt != m_groupCache.end())
        {
            group = cacheIt->second;
        }
        else
        {
            group = winrt::make_self<ShapedGroup>(bucket.Key, bucket.Identity);
            m_groupCache.emplace(keyString, group);
        }

        group->GroupKey(bucket.Identity);
        // Items are deliberately NOT the bucket's roots: a group's rows are its roots PLUS every
        // currently-visible descendant of those roots. ResliceGroupsFromHierarchy fills them in
        // from the adapter, which is the only thing that knows what is expanded.
        m_hierarchyGroups.push_back(group);
        groups.push_back(group.as<winrt::IInspectable>());
        liveKeys.insert(keyString);
    }

    for (auto it = m_groupCache.begin(); it != m_groupCache.end();)
    {
        if (liveKeys.find(it->first) == liveKeys.end())
        {
            it = m_groupCache.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Fill the groups BEFORE the grouped adapter is attached, so its subscribe + synchronous
    // rebuild inside Source(value) sees finished groups and publishes one coherent Reset -- the
    // same ordering constraint RebuildGrouped documents.
    ResliceGroupsFromHierarchy();

    if (!m_groupedAdapter)
    {
        m_groupedAdapter = std::make_shared<GroupedSourceAdapter>();
    }
    else
    {
        m_groupedAdapter->DetachSourceQuietly();
    }

    m_groupSource.ReplaceAll(groups);
    m_groupedAdapter->Source(m_groupSource);

    // Rows() is the flat shaped projection: every visible row, in group order, headers excluded.
    m_rows.ReplaceAll(VisibleHierarchicalRows());

    m_kind = ProjectionKind::GroupedHierarchical;
    // Grouped IS true here: the presented axis is the grouped adapter's, so it carries header rows
    // and a consumer must look for GroupedEntry.
    m_projectedAsGrouped = true;
    RaiseProjectionRebuilt();
}

void ShapedItemsSource::ReleaseHierarchyProjection()
{
    if (m_hierarchicalAdapter)
    {
        // Callback first: DetachSourceQuietly must not be able to drive a re-slice on the way out.
        m_hierarchicalAdapter->ProjectionChanged(nullptr);
        m_hierarchicalAdapter->DetachSourceQuietly();
    }

    if (m_hierarchySource)
    {
        m_hierarchySource.Clear();
    }

    m_hierarchyGroups.clear();
    m_rootBucketIndex.clear();
}

void ShapedItemsSource::EnsureHierarchicalAdapter(std::vector<winrt::IInspectable> const& roots, bool shapeRoots)
{
    if (!m_hierarchySource)
    {
        m_hierarchySource = winrt::single_threaded_observable_vector<winrt::IInspectable>();
    }

    if (!m_hierarchicalAdapter)
    {
        m_hierarchicalAdapter = std::make_shared<HierarchicalSourceAdapter>();
    }
    else
    {
        // A prior hierarchical projection already attached the adapter to m_hierarchySource.
        // Detach quietly so the ReplaceAll below fires no subscription and the Source() re-attach
        // rebuilds exactly once against the fully-populated roots -- the identical two-rebuild /
        // intermediate-publish hazard RebuildGrouped documents.
        m_hierarchicalAdapter->DetachSourceQuietly();
    }

    // Drop the previous projection callback before the rebuild: it is re-established below only
    // when this projection actually needs it, and a stale one would re-slice groups that this
    // rebuild is in the middle of replacing.
    m_hierarchicalAdapter->ProjectionChanged(nullptr);

    // Per-level shaping. This runs INSIDE the adapter's walk, so it must touch nothing but the
    // vector it is handed -- see the re-entrancy contract on ShapeSiblingsFn. Filtering and sorting
    // each sibling set among its own peers is the same principle as RebuildGrouped's per-bucket
    // ApplySort: comparing a parent against its own child is meaningless and must not be
    // expressible.
    //
    // NOTE on filtering: this is match-node-only, not the ancestor retention §5 describes. A child
    // that fails the predicate is dropped even if one of ITS descendants matches. Ancestor
    // retention requires testing descendants of collapsed nodes, which is exactly what the lazy
    // walk exists to avoid; reconciling the two is deferred with the rest of that section.
    //
    // The ROOT set is never routed here (the adapter shapes only the levels it discovers), so
    // shapeRoots does not gate this callback; it documents which caller already owns root order.
    std::weak_ptr<ShapedItemsSource> weakThis = weak_from_this();
    m_hierarchicalAdapter->ShapeSiblings(
        [weakThis](std::vector<winrt::IInspectable>& siblings)
        {
            if (auto strongThis = weakThis.lock())
            {
                strongThis->ApplyFilter(siblings);
                strongThis->ApplySort(siblings);
            }
        });

    m_hierarchicalAdapter->ChildrenSelector(m_childrenSelector);
    m_hierarchicalAdapter->HasChildrenSelector(m_hasChildrenSelector);

    // Populate before attaching, for the same coherence reason as the grouped path.
    m_hierarchySource.ReplaceAll(roots);
    m_hierarchicalAdapter->Source(m_hierarchySource);

    // Under grouping the adapter's entries are NOT the presented axis -- the grouped adapter's
    // are -- so a node toggle, which splices the hierarchy adapter in place, would otherwise be
    // invisible. This callback is what carries it across to the groups. It is the adapter's
    // COHERENT edge rather than the raw vector notification: a multi-row splice raises the latter
    // once per row and from inside the mutation, so a re-slice driven by it would read the
    // descriptor side-table mid-repair and would do it N times.
    if (!shapeRoots)
    {
        m_hierarchicalAdapter->ProjectionChanged(
            [weakThis]()
            {
                if (auto strongThis = weakThis.lock())
                {
                    strongThis->ResliceGroupsFromHierarchy();
                }
            });
    }
}

std::vector<winrt::IInspectable> ShapedItemsSource::VisibleHierarchicalRows() const
{
    std::vector<winrt::IInspectable> visibleRows;
    if (!m_hierarchicalAdapter)
    {
        return visibleRows;
    }

    if (auto const entries = m_hierarchicalAdapter->Entries())
    {
        const int32_t count = entries.Count();
        visibleRows.reserve(static_cast<size_t>(count > 0 ? count : 0));
        for (int32_t i = 0; i < count; ++i)
        {
            visibleRows.push_back(entries.GetAt(i));
        }
    }
    return visibleRows;
}

void ShapedItemsSource::ResliceGroupsFromHierarchy()
{
    if (!m_hierarchicalAdapter || m_hierarchyGroups.empty())
    {
        return;
    }

    // Setting a group's Items notifies the grouped adapter, which rebuilds; that rebuild must not
    // re-enter here. Plain bool, matching the adapters: this whole stack is UI-thread-affine.
    if (m_reslicingGroups)
    {
        return;
    }
    m_reslicingGroups = true;
    auto guard = wil::scope_exit([this]() noexcept { m_reslicingGroups = false; });

    std::vector<std::vector<winrt::IInspectable>> slices(m_hierarchyGroups.size());

    auto const entries = m_hierarchicalAdapter->Entries();
    const int32_t count = entries ? entries.Count() : 0;

    // One pass. Every depth-0 row opens the bucket it was assigned at build time and every row
    // after it belongs to that bucket until the next depth-0 row -- which is exactly the adapter's
    // own emission order, a root immediately followed by its visible subtree.
    size_t currentBucket = 0;
    bool haveBucket = false;
    for (int32_t i = 0; i < count; ++i)
    {
        auto const* const node = m_hierarchicalAdapter->TryGetNodeRow(i);
        if (!node)
        {
            continue;
        }

        if (node->Depth == 0)
        {
            auto const it = m_rootBucketIndex.find(winrt::get_abi(node->Item));
            if (it == m_rootBucketIndex.end())
            {
                // A root the bucket map does not know. Only reachable if the source mutated
                // between the bucketize and this pass, in which case a full rebuild is already
                // queued; drop this subtree rather than charge it to the previous bucket.
                haveBucket = false;
                continue;
            }
            currentBucket = it->second;
            haveBucket = true;
        }

        if (haveBucket && currentBucket < slices.size())
        {
            slices[currentBucket].push_back(node->Item);
        }
    }

    for (size_t i = 0; i < m_hierarchyGroups.size(); ++i)
    {
        m_hierarchyGroups[i]->SetItems(slices[i]);
    }
}

ShapingHelpers::ShapingPipeline::SortedInsertPlacement ShapedItemsSource::SortedInsertPlacementFor(winrt::IInspectable const& item) const{
    return m_pipeline.SortedInsertPlacementFor(
        item,
        m_rows ? m_rows.Size() : 0,
        [this](uint32_t index) { return m_rows.GetAt(index); });
}

bool ShapedItemsSource::TryGetSourceItemCount(uint32_t& count) const
{
    if (!m_sourceAccessor.IsIndexable())
    {
        return false;
    }
    count = m_sourceAccessor.Count();
    return true;
}

winrt::hstring ShapedItemsSource::StringifyKey(winrt::IInspectable const& key)
{
    return RowIdentity::StringifyKey(key);
}

winrt::hstring ShapedItemsSource::Diagnostic(std::wstring_view text) const
{
    return m_diagnosticName + L": " + winrt::hstring{ text };
}

