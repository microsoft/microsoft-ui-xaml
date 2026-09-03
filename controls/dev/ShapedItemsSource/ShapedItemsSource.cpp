// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ShapedItemsSource.h"
#include "RowIdentity.h"
#include "GroupedSourceAdapter.h"
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
    return m_groupSelector || m_pipeline.HasFilter() || HasActiveSort() || IsSourceMutable();
}

bool ShapedItemsSource::HasAnyShapingVerb() const
{
    return m_pipeline.HasFilter() || m_groupSelector || HasActiveSort();
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

            if (m_groupSelector)
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

ShapingHelpers::ShapingPipeline::SortedInsertPlacement ShapedItemsSource::SortedInsertPlacementFor(winrt::IInspectable const& item) const
{
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

