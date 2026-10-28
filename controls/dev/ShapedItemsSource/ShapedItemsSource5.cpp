// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ShapedItemsSource5.h"
#include "RowIdentity.h"
#include "GroupedSourceAdapter.h"
#include "HierarchicalSourceAdapter5.h"
#include "HierarchyStructureCache5.h"
#include "HierarchyIndex5.h"
#include "SharedHelpers.h"
#include "TVDiag.h"
#include "ShapingHelpers.h"
#include "ShapingRevoke.h"
#include "ShapedGroup.h"

#include <cmath>
#include <functional>
#include <optional>
#include <algorithm>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <winrt/Microsoft.UI.Dispatching.h>

namespace ShapingHelpers {}
namespace cw5
{
namespace ShapingHelpers { using namespace ::ShapingHelpers; }


namespace
{
    // First segment of a binding path ("Address.City" -> "Address"): the name an item raises
    // PropertyChanged for. Empty for "(ns:Type.Prop)" or "[0]", meaning "may depend on anything".
    std::wstring_view LeadingPropertyName(std::wstring_view path)
    {
        auto const begin = path.find_first_not_of(L" \t");
        if (begin == std::wstring_view::npos)
        {
            return {};
        }
        path.remove_prefix(begin);

        auto const name = path.substr(0, path.find_first_of(L".[ \t"));
        if (name.empty() || name.front() == L'(')
        {
            return {};
        }
        return name;
    }

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
    m_rows(winrt::single_threaded_observable_vector<winrt::IInspectable>()),
    m_ownerQueue(winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread()),
    m_ownerThreadId(::GetCurrentThreadId())
{
}

ShapedItemsSource::~ShapedItemsSource()
{
    UnsubscribeFromSourceCollectionChanges();
    ClearLiveShapingSubscriptions();
    ResetStructureCache();
}

void ShapedItemsSource::Start()
{
    // Not in the constructor: needs weak_from_this so an off-thread handler cannot outlive us.
    m_liveShaping->SetChangeHandler(
        [weakThis = weak_from_this(), queue = m_ownerQueue, ownerThreadId = m_ownerThreadId, dropped = m_liveShapingChangeDropped](winrt::IInspectable const& item, winrt::hstring const& propertyName)
        {
            if (queue && !queue.HasThreadAccess())
            {
                // Off-thread: all touched state belongs to the owning thread, so marshal it.
                const bool posted = queue.TryEnqueue([weakThis, item, propertyName]()
                    {
                        if (auto const strongThis = weakThis.lock())
                        {
                            strongThis->OnLiveShapedItemChanged(item, propertyName);
                        }
                    });
                if (!posted)
                {
                    dropped->store(true);
                }
                return;
            }

            if (!queue && ::GetCurrentThreadId() != ownerThreadId)
            {
                // No queue to marshal to. Flag it; the owner's next live change forces a full
                // restore, which re-reads every item.
                dropped->store(true);
                return;
            }

            if (auto const strongThis = weakThis.lock())
            {
                strongThis->OnLiveShapedItemChanged(item, propertyName);
            }
        });

    SubscribeToSourceCollectionChanges();
    Refresh();
}

void ShapedItemsSource::SetLiveShaping(bool enabled)
{
    if (m_liveShapingEnabled == enabled)
    {
        return;
    }

    m_liveShapingEnabled = enabled;

    if (enabled)
    {
        // Live shaping takes over staleness detection; anything the sort-key subscriptions had
        // pending is covered by the restore posted below.
        ClearFlatItemPropertyChangedTracking();

        ResubscribeLiveShapingFromSource();
        if (m_liveItemChangedHook)
        {
            m_liveItemChangedHook(nullptr, true);
        }

        MarkLiveShapingDirty();
    }
    else
    {
        ClearLiveShapingSubscriptions();

        // Sort-key tracking owns staleness again. The projection is current (live shaping kept it
        // so), hence tracking restarts with nothing dirty.
        if (m_kind == ProjectionKind::Flat && m_shapingState.HasProjection)
        {
            RebuildFlatItemPropertyChangedTracking(m_shapingState.Items);
        }

        m_liveShapingNeedsRefresh = false;
        m_liveShapingNeedsRebuild = false;
    }

    // The children-selector hierarchy observes its items through the structure cache instead.
    if (m_structureCache)
    {
        m_structureCache->TrackItemChanges(enabled);
        if (enabled)
        {
            // Lists shaped while nobody was watching may already be out of order; the next pass
            // re-shapes them against current values.
            m_structureCache->InvalidateShapedChildren();
        }
    }
}

void ShapedItemsSource::SetFilter(ShapingHelpers::Predicate const& predicate)
{
    m_pipeline.SetFilter(predicate);
    ResetFilterContextOverlay();
    ApplyShapingChange();
}

void ShapedItemsSource::SetFilter(winrt::hstring const& axisToken, ShapingHelpers::Predicate const& predicate)
{
    m_pipeline.SetFilter(axisToken, predicate);
    ResetFilterContextOverlay();
    ApplyShapingChange();
}

void ShapedItemsSource::ClearFilter()
{
    m_pipeline.ClearFilter();
    ResetFilterContextOverlay();
    ApplyShapingChange();
}

void ShapedItemsSource::ClearFilter(winrt::hstring const& axisToken)
{
    m_pipeline.ClearFilter(axisToken);
    ResetFilterContextOverlay();
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

    // A new selector may map the same item to a different collection; nothing cached under the
    // old one is trustworthy.
    ResetStructureCache();
    // The hierarchy axis is invisible to the spec diff; without this a children verb is a no-op.
    // Re-declaration always counts as a change: the selectors are delegates.
    m_hierarchyAxisDirty = true;

    // Grouping is NOT retracted: GroupBy buckets the roots (see RebuildGroupedHierarchical).
    ApplyShapingChange();
}

void ShapedItemsSource::ClearChildren()
{
    // Nothing declared: no-op, avoid a needless Reset.
    if (!m_childrenSelector && !m_hasChildrenSelector)
    {
        return;
    }

    m_childrenSelector = nullptr;
    m_hasChildrenSelector = nullptr;
    ResetStructureCache();
    m_hierarchyAxisDirty = true;
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
    // Commit unconditionally, even when the in-place path is not taken: the committed spec is
    // the baseline the NEXT verb diffs against, so skipping it would make that diff report a
    // change that has already been applied.
    auto const delta = m_pipeline.CommitSpec();
    UpdateSortKeyPropertyFilter();

    if (delta.FilterChanged)
    {
        ++m_filterGeneration;
    }

    // Sibling lists are shaped by filter and sort only; grouping buckets the roots and never
    // reaches them, so a grouping-only change keeps every cached shaped list. An eager hierarchy
    // filters every sibling list by its kept set, and its index update re-shapes exactly the lists
    // whose kept membership moved, so a filter-only change there drops none of them up front.
    if ((delta.FilterChanged || delta.SortChanged) && m_structureCache)
    {
        if (delta.SortChanged || !IsEagerHierarchy() || m_hierarchyAxisDirty)
        {
            m_structureCache->InvalidateShapedChildren();
        }
        else
        {
            m_structureCache->InvalidateSnapshots();
        }
    }

    // Consumed regardless of delta: the diff cannot see the hierarchy axis.
    const bool hierarchyChanged = m_hierarchyAxisDirty;
    m_hierarchyAxisDirty = false;

    if (delta.IsNoOp() && !hierarchyChanged)
    {
        // Re-declaring the identical shape. The projection already satisfies it, and a rebuild
        // would fire a Reset that drops every realized row for nothing. Reachable only from a
        // Clear* verb against a shape that has nothing to clear — every declaration re-mints its
        // description id, so a re-declaration always reads as a change.
        return;
    }

    // In-place paths cannot switch flat <-> hierarchical; a hierarchy change always rebuilds.
    if (!hierarchyChanged && TryApplyShapingDeltaInPlace(delta))
    {
        // Skips Refresh(), so re-capture live-shaping keys against the new spec (no COM churn).
        if (IsLiveShapingEnabled())
        {
            ResubscribeLiveShapingFromSource();
        }
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
bool ShapedItemsSource::TryApplyShapingDeltaInPlace(ShapingHelpers::ShapingDelta const& delta, bool allowDuringIncrementalChange)
{
    // Only a pure re-order is safe to do against rows already in hand. Anything touching
    // membership or bucketing needs the source, the group cache and the projection swap that
    // only Refresh performs.
    if (delta.RequiredWork != ShapingHelpers::ShapingWork::ReSortWithinBuckets)
    {
        return false;
    }

    // A grouped projection is rebuilt through the group adapter, and sorts declared before the
    // group verb reorder the GROUPS — which layer 1's within-bucket sort cannot express. A
    // hierarchy re-sorts every sibling set through the adapter.
    if (m_groupSelector || m_projectedAsGrouped || m_childrenSelector)
    {
        return false;
    }

    if (!m_rows || !m_shapingState.HasProjection || m_shapingState.IsGrouped)
    {
        return false;
    }

    // A rebuild is already going to run and will subsume this change.
    if (m_isRefreshing || (!allowDuringIncrementalChange && m_isApplyingIncrementalChange) || m_pendingRefresh)
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

    ++m_projectionGeneration;
    m_rows.ReplaceAll(m_shapingState.Items);
    RebuildFlatRowIdentityTracking(m_shapingState.Items);

    // Membership is unchanged and subscriptions are keyed by object, so only start/stop observing
    // when a sort appears or disappears. Every row was just re-sorted, so nothing is dirty.
    if (!ShouldTrackSortKeyChanges())
    {
        ClearFlatItemPropertyChangedTracking();
    }
    else if (!m_flatItemPropertyChangedTrackingActive)
    {
        RebuildFlatItemPropertyChangedTracking(m_shapingState.Items);
    }
    else
    {
        ClearSortDirtyRows();
    }
    return true;
}

void ShapedItemsSource::InvalidateShapingState()
{
    ++m_projectionGeneration;
    m_shapingState.HasProjection = false;
    m_shapingState.FilteredSource.clear();
    m_shapingState.Items.clear();
    m_shapingState.Buckets.clear();
    m_shapingState.IsGrouped = false;
    ClearFlatItemPropertyChangedTracking();
}

void ShapedItemsSource::SubscribeToSourceCollectionChanges()
{
    UnsubscribeFromSourceCollectionChanges();

    // Classify the source once, here, where it is bound. Every later indexed read, count and
    // observability check reads off this resolution instead of re-probing.
    m_sourceAccessor = ShapingHelpers::CollectionAccessor{ m_source, ShapingHelpers::CollectionOwner::Caller };

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
    ++m_sourceChangeStamp;
    Refresh();
}

void ShapedItemsSource::OnSourceCollectionChanged(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
{
    // See the UI-thread contract on OnSourceCollectionChanged(): incremental InsertAt/RemoveAt/
    // SetAt below mutate the UI-affine projection directly, so they must run on the owning thread.
    ++m_sourceChangeStamp;

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

    try
    {
        m_isApplyingIncrementalChange = true;
        m_raiseReorderAfterIncrementalChange = false;
        auto guard = wil::scope_exit([this]() noexcept { m_isApplyingIncrementalChange = false; });
        ApplyIncrementalChange(args);
    }
    catch (...)
    {
        // Post (not run) any request deferred behind the failed pass; the caller gets this error.
        PostPendingRefresh();
        throw;
    }

    const bool raiseReorder = std::exchange(m_raiseReorderAfterIncrementalChange, false);

    // A notification re-entered while we were applying: now that the projection/identity
    // invariants are consistent again, run exactly one deferred rebuild against live source.
    if (m_pendingRefresh)
    {
        m_pendingRefresh = false;
        Refresh();
    }

    // A stale-sort repair re-ordered the projection; raise ShapingChanged(reorderOnly) so the owner
    // treats the Reset as a re-shape. Raised last so its handler may safely touch the source.
    if (raiseReorder)
    {
        RaiseShapingChanged(true /* reorderOnly */);
    }
}

void ShapedItemsSource::OnSourceVectorChanged(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
{
    // See the UI-thread contract on OnSourceCollectionChanged().
    ++m_sourceChangeStamp;

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

    try
    {
        m_isApplyingIncrementalChange = true;
        auto guard = wil::scope_exit([this]() noexcept { m_isApplyingIncrementalChange = false; });
        ApplyIncrementalVectorChange(args);
    }
    catch (...)
    {
        // Post (not run) any request deferred behind the failed pass; the caller gets this error.
        PostPendingRefresh();
        throw;
    }

    if (m_pendingRefresh)
    {
        m_pendingRefresh = false;
        Refresh();
    }
}

void ShapedItemsSource::ApplyIncrementalChange(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
{
    // Subscription DELTA from the args, here rather than per splice site, so every branch is
    // covered at cost proportional to the change.
    ApplyLiveShapingDelta(args);

    using winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;

    // Full rebuild unless there is a live flat projection. A root-set change under a hierarchy
    // re-projects through the adapter.
    if (m_isRefreshing || m_groupSelector || m_childrenSelector || !m_rows || m_kind == ProjectionKind::None)
    {
        Refresh();
        return;
    }

    // SORTED flat projection: apply a single-item change in place (binary-search insert / tracked
    // removal) instead of an O(n) rebuild per change; anything else falls back to Refresh.
    //
    // Invariant: re-sorting is driven by collection notifications on this path. An in-place
    // mutation of a row's sort-key field that raises only INotifyPropertyChanged leaves the row
    // at its old sort position until the next collection change repairs it first. With a filter,
    // always rebuild: a filter-key INPC change can make the retained FilteredSource stale.
    if (HasActiveSort())
    {
        const bool canTrySortedIncremental =
            !m_pipeline.HasFilter() &&
            ((args.Action() == NotifyCollectionChangedAction::Add &&
                    args.NewItems() && args.NewItems().Size() == 1) ||
                (args.Action() == NotifyCollectionChangedAction::Remove &&
                    args.OldItems() && args.OldItems().Size() == 1) ||
                (args.Action() == NotifyCollectionChangedAction::Replace &&
                    args.OldItems() && args.OldItems().Size() == 1 &&
                    args.NewItems() && args.NewItems().Size() == 1) ||
                (args.Action() == NotifyCollectionChangedAction::Move &&
                    args.NewItems() && args.NewItems().Size() == 1));

        // Only take the in-place sorted fast-path when a shaped flat projection is active
        // (a Flat projection). After a safe-degrade (an Unshaped projection leaves an UNSORTED
        // mirror in m_rows while a sort is still configured), a binary-search insert would splice
        // into an unsorted list at a bogus index, so fall through to a full Refresh() that
        // re-shapes (or re-degrades) coherently.
        if (canTrySortedIncremental &&
            m_kind == ProjectionKind::Flat &&
            TryReshapeStaleSortedFlatProjection() &&
            TryApplyIncrementalSortedChange(args))
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

    // The paths below mutate m_rows outside Refresh, which would leave the retained layer-1 state
    // stale, so drop it (Refresh repopulates it). The sorted path above keeps it in lockstep instead.
    InvalidateShapingState();

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
    // VectorChanged carries no items, so the subscription delta is applied at each splice site;
    // Refresh() paths reconcile themselves.

    // Same reasoning as ApplyIncrementalChange: the retained membership stops describing m_rows
    // the moment this splices it.
    InvalidateShapingState();

    using winrt::Windows::Foundation::Collections::CollectionChange;

    // VectorChanged has only a verb + index (no OldItems/NewItems). Keep the low-risk fast path
    // to flat 1:1 projections, where the source index is the projection index and the current
    // source/projection can provide the one item needed to splice m_rows and identity tracking.
    if (m_isRefreshing || m_groupSelector || m_childrenSelector || HasActiveSort() || m_pipeline.HasFilter() ||
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
        AddLiveShapingSubscription(item);
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
        RemoveLiveShapingSubscription(item);
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

        auto const outgoing = m_rows.GetAt(index);
        m_rows.SetAt(index, newItem);
        // Remove before add, so reassigning the SAME object stays subscribed.
        RemoveLiveShapingSubscription(outgoing);
        AddLiveShapingSubscription(newItem);
        return;
    }
    case CollectionChange::Reset:
    default:
        break;
    }

    Refresh();
}

bool ShapedItemsSource::TryReshapeStaleSortedFlatProjection()
{
    if (!HasActiveSort() ||
        m_kind != ProjectionKind::Flat ||
        m_groupSelector ||
        m_projectedAsGrouped ||
        !m_rows)
    {
        return true;
    }

    // Live shaping owns staleness here, but its repair is posted, so a change arriving before that
    // turn can see an order its own edit already invalidated. Refuse the in-place splice while a
    // restore is pending and let the caller rebuild instead of binary-searching an unsorted list.
    if (IsLiveShapingEnabled())
    {
        return !m_liveShapingDirty;
    }

    std::unordered_set<uintptr_t> dirtyRows;
    dirtyRows.swap(m_sortDirtyRows);
    const bool overflow = std::exchange(m_sortDirtyOverflow, false);

    if (dirtyRows.empty() && !overflow)
    {
        return true;
    }

    // The projection was sorted before these keys changed, so any out-of-order adjacent pair has a
    // dirty row: checking dirty rows' neighbours is complete. Above 1/8 dirty, a full scan is cheaper.
    bool sorted = true;
    if (overflow || dirtyRows.size() * 8 > m_rows.Size())
    {
        sorted = IsFlatProjectionSorted();
    }
    else
    {
        for (auto const address : dirtyRows)
        {
            auto const identity = RowIdentity::FormatObjectIdentity(address);
            if (m_flatRowIdentityToIndex.find(identity) == m_flatRowIdentityToIndex.end())
            {
                continue;
            }

            uint32_t index = 0;
            if (!TryGetTrackedFlatRowIndex(identity, index))
            {
                sorted = IsFlatProjectionSorted();
                break;
            }

            if (!IsFlatRowInSortOrderWithNeighbours(index))
            {
                sorted = false;
                break;
            }
        }
    }

    if (sorted)
    {
        return true;
    }

    ShapingHelpers::ShapingDelta delta;
    delta.SortChanged = true;
    delta.RequiredWork = ShapingHelpers::ShapingWork::ReSortWithinBuckets;
    if (!TryApplyShapingDeltaInPlace(delta, true /* allowDuringIncrementalChange */))
    {
        return false;
    }

    m_raiseReorderAfterIncrementalChange = true;
    return true;
}

bool ShapedItemsSource::IsFlatProjectionSorted() const
{
    if (!m_rows || m_rows.Size() < 2)
    {
        return true;
    }

    for (uint32_t i = 1; i < m_rows.Size(); ++i)
    {
        if (m_pipeline.CompareItemToRow(m_rows.GetAt(i - 1), m_rows.GetAt(i)) > 0)
        {
            return false;
        }
    }
    return true;
}

bool ShapedItemsSource::IsFlatRowInSortOrderWithNeighbours(uint32_t index) const
{
    const uint32_t rowCount = m_rows ? m_rows.Size() : 0;
    if (index >= rowCount)
    {
        return false;
    }

    auto const row = m_rows.GetAt(index);
    if (index > 0 && m_pipeline.CompareItemToRow(m_rows.GetAt(index - 1), row) > 0)
    {
        return false;
    }
    if (index + 1 < rowCount && m_pipeline.CompareItemToRow(row, m_rows.GetAt(index + 1)) > 0)
    {
        return false;
    }
    return true;
}

void ShapedItemsSource::UpdateSortKeyPropertyFilter()
{
    std::unordered_set<winrt::hstring> names;
    bool anyProperty = false;
    for (auto const& axis : m_pipeline.ActiveSortAxes(-1, -1))
    {
        // No usable path (delegate key/comparer): any property change may move a row.
        auto const name = LeadingPropertyName(axis.SortMemberPath);
        if (name.empty())
        {
            anyProperty = true;
            break;
        }
        names.emplace(name);
    }

    m_sortKeyPropertyNames = std::move(names);
    m_anyPropertyMayAffectSort = anyProperty;
}

void ShapedItemsSource::OnFlatItemPropertyChanged(uintptr_t itemAddress, winrt::hstring const& propertyName)
{
    // Beyond this, one linear check is cheaper than per-row lookups.
    constexpr size_t c_maxIndividuallyTrackedDirtyRows = 256;


    // An empty name is INotifyPropertyChanged's "all properties changed".
    if (!propertyName.empty() &&
        !m_anyPropertyMayAffectSort &&
        m_sortKeyPropertyNames.find(propertyName) == m_sortKeyPropertyNames.end())
    {
        return;
    }

    if (m_sortDirtyOverflow)
    {
        return;
    }

    if (m_sortDirtyRows.size() >= c_maxIndividuallyTrackedDirtyRows)
    {
        m_sortDirtyRows.clear();
        m_sortDirtyOverflow = true;
        return;
    }

    m_sortDirtyRows.insert(itemAddress);
}

void ShapedItemsSource::TrackFlatItemPropertyChanged(winrt::IInspectable const& item)
{
    auto const propertyChangedItem = item ? item.try_as<winrt::Microsoft::UI::Xaml::Data::INotifyPropertyChanged>() : nullptr;
    if (!propertyChangedItem)
    {
        m_flatItemsMayChangeSortWithoutNotification = true;
        return;
    }

    auto const address = RowIdentity::GetObjectAddress(item);
    if (m_flatItemPropertyChangedRevokers.find(address) != m_flatItemPropertyChangedRevokers.end())
    {
        return;
    }

    auto weakThis = weak_from_this();
    m_flatItemPropertyChangedRevokers.emplace(
        address,
        FlatItemPropertyChangedSubscription{
            item,
            propertyChangedItem.PropertyChanged(winrt::auto_revoke,
                [weakThis, address, queue = m_ownerQueue, ownerThreadId = m_ownerThreadId](
                    auto&&, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs const& args)
                {
                    // Never lock off-thread: that lock could be the last strong reference, running
                    // the destructor (and its UI-affine revokers) on the app's worker thread. Same
                    // marshalling as the live-shaping handler in Start().
                    if (queue && !queue.HasThreadAccess())
                    {
                        queue.TryEnqueue([weakThis, address, propertyName = args.PropertyName()]()
                            {
                                if (auto const strongThis = weakThis.lock())
                                {
                                    strongThis->OnFlatItemPropertyChanged(address, propertyName);
                                }
                            });
                        return;
                    }

                    if (!queue && ::GetCurrentThreadId() != ownerThreadId)
                    {
                        // Nothing to marshal to. Dropping the mark leaves the row stale until a
                        // rebuild, the documented contract for unobserved sort-key edits.
                        return;
                    }

                    if (auto const strongThis = weakThis.lock())
                    {
                        strongThis->OnFlatItemPropertyChanged(address, args.PropertyName());
                    }
                }) });
}

void ShapedItemsSource::UntrackFlatItemPropertyChanged(winrt::IInspectable const& item)
{
    if (item && !m_flatItemPropertyChangedRevokers.empty())
    {
        m_flatItemPropertyChangedRevokers.erase(RowIdentity::GetObjectAddress(item));
    }
}

void ShapedItemsSource::ClearSortDirtyRows()
{
    m_sortDirtyRows.clear();
    m_sortDirtyOverflow = false;
}

void ShapedItemsSource::ClearFlatItemPropertyChangedTracking()
{
    m_flatItemPropertyChangedRevokers.clear();
    m_flatItemPropertyChangedTrackingActive = false;
    m_flatItemsMayChangeSortWithoutNotification = false;
    ClearSortDirtyRows();
}

void ShapedItemsSource::RebuildFlatItemPropertyChangedTracking(std::vector<winrt::IInspectable> const& rows)
{
    // Surviving rows keep their subscription; the rest are revoked when `previous` is destroyed.
    auto previous = std::move(m_flatItemPropertyChangedRevokers);
    ClearFlatItemPropertyChangedTracking();

    if (!ShouldTrackSortKeyChanges())
    {
        return;
    }

    m_flatItemPropertyChangedTrackingActive = true;
    m_flatItemPropertyChangedRevokers.reserve(rows.size());
    for (auto const& item : rows)
    {
        if (item && !previous.empty())
        {
            if (auto const it = previous.find(RowIdentity::GetObjectAddress(item)); it != previous.end())
            {
                m_flatItemPropertyChangedRevokers.insert(previous.extract(it));
                continue;
            }
        }
        TrackFlatItemPropertyChanged(item);
    }
}

// Sorted, unfiltered, ungrouped flat projection only. Nothing is mutated until every precondition
// passes, so a false return leaves all state untouched and the commit cannot fail half-way.
bool ShapedItemsSource::TryApplyIncrementalSortedChange(winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
{
    using winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;

    // With no filter, FilteredSource is the source in source order, so the change's own index
    // addresses it directly; an identity mismatch there falls back to Refresh.
    if (!m_rows ||
        m_pipeline.HasFilter() ||
        !m_shapingState.HasProjection ||
        m_shapingState.IsGrouped ||
        m_shapingState.Items.size() != m_rows.Size())
    {
        return false;
    }

    auto& filteredSource = m_shapingState.FilteredSource;
    const uint32_t rowCount = m_rows.Size();
    const uint64_t generation = m_projectionGeneration;

    // Planning calls app code (sort keys, comparers), and committing a removal raises m_rows
    // listeners, either of which can rebuild the projection. Re-check before every raw vector
    // splice; a stale plan returns false and the caller rebuilds.
    auto const isPlanCurrent = [&]()
    {
        return generation == m_projectionGeneration &&
            m_rows && m_shapingState.HasProjection &&
            m_shapingState.Items.size() == m_rows.Size();
    };

    // A count mismatch means a change was missed; only a rebuild recovers.
    auto const liveSourceCountIs = [&](size_t expected)
    {
        uint32_t liveCount = 0;
        return !TryGetSourceItemCount(liveCount) || liveCount == expected;
    };

    struct RowChange
    {
        winrt::IInspectable Item{ nullptr };
        winrt::hstring Identity;
        uint32_t RowIndex{};
        size_t SourceIndex{};
    };

    auto const planRemove = [&](winrt::IInspectable const& item, int32_t oldStartingIndex, RowChange& removal) -> bool
    {
        if (!item || oldStartingIndex < 0)
        {
            return false;
        }

        const auto sourceIndex = static_cast<size_t>(oldStartingIndex);
        if (sourceIndex >= filteredSource.size() || filteredSource[sourceIndex] != item)
        {
            return false;
        }

        winrt::hstring identity;
        wchar_t const* reason = nullptr;
        uint32_t rowIndex = 0;
        if (!TryGetRequiredRowIdentity(item, identity, reason) ||
            !TryGetTrackedFlatRowIndex(identity, rowIndex) ||
            rowIndex >= rowCount)
        {
            return false;
        }

        removal = { item, identity, rowIndex, sourceIndex };
        return true;
    };

    // Indices are as they will be after `removal` (if any) is committed.
    auto const planInsert = [&](winrt::IInspectable const& item, int32_t newStartingIndex, RowChange const* removal, RowChange& insertion) -> bool
    {
        if (!item || newStartingIndex < 0)
        {
            return false;
        }

        const size_t sourceSize = filteredSource.size() - (removal ? 1 : 0);
        const auto sourceIndex = static_cast<size_t>(newStartingIndex);
        if (sourceIndex > sourceSize || !liveSourceCountIs(sourceSize + 1))
        {
            return false;
        }

        winrt::hstring identity;
        wchar_t const* reason = nullptr;
        if (!TryGetRequiredRowIdentity(item, identity, reason))
        {
            return false;
        }
        if (m_flatRowIdentities.find(identity) != m_flatRowIdentities.end() &&
            !(removal && removal->Identity == identity))
        {
            return false;
        }

        const uint32_t skippedRow = removal ? removal->RowIndex : rowCount;
        const uint32_t remainingCount = rowCount - (removal ? 1 : 0);
        auto const rowAt = [&](uint32_t index) { return m_rows.GetAt(index < skippedRow ? index : index + 1); };
        const auto placement = m_pipeline.SortedInsertPlacementFor(item, remainingCount, rowAt);
        if (placement.Index > remainingCount)
        {
            return false;
        }

        // Ties break by source order, which rows don't carry; only a source-last item can be placed
        // in a tie group (after every equal-key row).
        if (placement.TiedWithExistingRow && sourceIndex != sourceSize)
        {
            return false;
        }

        // Non-INPC rows may be stale unseen; at least verify the landing neighbourhood is ordered.
        if (m_flatItemsMayChangeSortWithoutNotification &&
            ((placement.Index > 0 && m_pipeline.CompareItemToRow(rowAt(placement.Index - 1), item) > 0) ||
                (placement.Index < remainingCount && m_pipeline.CompareItemToRow(item, rowAt(placement.Index)) > 0)))
        {
            return false;
        }

        insertion = { item, identity, placement.Index, sourceIndex };
        return true;
    };

    // m_rows is mutated last so all other state is consistent when listeners are notified.
    auto const commitRemove = [&](RowChange const& removal) -> bool
    {
        if (!isPlanCurrent() ||
            removal.SourceIndex >= filteredSource.size() ||
            removal.RowIndex >= m_shapingState.Items.size() ||
            m_shapingState.Items[removal.RowIndex] != removal.Item)
        {
            return false;
        }

        m_flatRowIdentities.erase(removal.Identity);
        m_flatRowIdentityToIndex.erase(removal.Identity);
        ShiftTrackedFlatRowIndicesForRemove(removal.RowIndex);
        filteredSource.erase(filteredSource.begin() + removal.SourceIndex);
        m_shapingState.Items.erase(m_shapingState.Items.begin() + removal.RowIndex);
        UntrackFlatItemPropertyChanged(removal.Item);
        if (m_rowsReleasedHook)
        {
            m_rowsReleasedHook(removal.Item, {});
        }
        m_rows.RemoveAt(removal.RowIndex);
        return true;
    };

    auto const commitInsert = [&](RowChange const& insertion) -> bool
    {
        if (!isPlanCurrent() ||
            insertion.SourceIndex > filteredSource.size() ||
            insertion.RowIndex > m_shapingState.Items.size() ||
            m_flatRowIdentities.find(insertion.Identity) != m_flatRowIdentities.end())
        {
            return false;
        }

        m_flatRowIdentities.insert(insertion.Identity);
        ShiftTrackedFlatRowIndicesForInsert(insertion.RowIndex);
        m_flatRowIdentityToIndex.insert_or_assign(insertion.Identity, insertion.RowIndex);
        filteredSource.insert(filteredSource.begin() + insertion.SourceIndex, insertion.Item);
        m_shapingState.Items.insert(m_shapingState.Items.begin() + insertion.RowIndex, insertion.Item);
        if (m_flatItemPropertyChangedTrackingActive)
        {
            TrackFlatItemPropertyChanged(insertion.Item);
        }
        m_rows.InsertAt(insertion.RowIndex, insertion.Item);
        return true;
    };

    // An app callout that rebuilds mid-plan leaves the binary search reading rows against bounds it
    // computed earlier, so the failure can surface as a throw (out-of-range GetAt) rather than a
    // refused commit. Either way the plan is void; rebuild instead of surfacing it to the app.
    auto const apply = [&]() -> bool
    {
        switch (args.Action())
        {
        case NotifyCollectionChangedAction::Add:
        {
            auto const newItems = args.NewItems();
            RowChange insertion;
            if (!newItems || newItems.Size() != 1 ||
                !planInsert(newItems.GetAt(0), args.NewStartingIndex(), nullptr, insertion))
            {
                return false;
            }
            return commitInsert(insertion);
        }
        case NotifyCollectionChangedAction::Remove:
        {
            auto const oldItems = args.OldItems();
            RowChange removal;
            if (!oldItems || oldItems.Size() != 1 ||
                !planRemove(oldItems.GetAt(0), args.OldStartingIndex(), removal) ||
                !liveSourceCountIs(filteredSource.size() - 1))
            {
                return false;
            }
            return commitRemove(removal);
        }
        case NotifyCollectionChangedAction::Replace:
        {
            auto const oldItems = args.OldItems();
            auto const newItems = args.NewItems();
            RowChange removal;
            RowChange insertion;
            if (!oldItems || !newItems || oldItems.Size() != 1 || newItems.Size() != 1 ||
                args.OldStartingIndex() != args.NewStartingIndex() ||
                !planRemove(oldItems.GetAt(0), args.OldStartingIndex(), removal) ||
                !planInsert(newItems.GetAt(0), args.NewStartingIndex(), &removal, insertion))
            {
                return false;
            }
            // The removal's m_rows notification can rebuild before the insert lands; if it did, the
            // insert refuses and the caller's rebuild supersedes the half-applied replace.
            return commitRemove(removal) && commitInsert(insertion);
        }
        case NotifyCollectionChangedAction::Move:
        {
            // A sorted projection is independent of source order EXCEPT for tie order: rows the sort
            // cannot distinguish keep their source order, so moving one of them past another really
            // does reorder the projection. Ties are contiguous, so only the moved row's two sorted
            // neighbours have to be probed; if it has none, the move is genuinely invisible here.
            auto const movedItems = args.NewItems();
            if (!movedItems || movedItems.Size() != 1 ||
                args.OldStartingIndex() < 0 || args.NewStartingIndex() < 0)
            {
                return false;
            }

            auto const moved = movedItems.GetAt(0);
            RowChange removal;
            const auto newSourceIndex = static_cast<size_t>(args.NewStartingIndex());
            if (!planRemove(moved, args.OldStartingIndex(), removal) ||
                newSourceIndex >= filteredSource.size() ||
                !liveSourceCountIs(filteredSource.size()))
            {
                return false;
            }

            const uint32_t index = removal.RowIndex;
            if (index > 0 && m_pipeline.CompareItemToRow(moved, m_rows.GetAt(index - 1)) == 0)
            {
                return false;
            }
            if (index + 1 < rowCount && m_pipeline.CompareItemToRow(moved, m_rows.GetAt(index + 1)) == 0)
            {
                return false;
            }

            // The comparisons above call app code.
            if (!isPlanCurrent() ||
                removal.SourceIndex >= filteredSource.size() ||
                newSourceIndex >= filteredSource.size())
            {
                return false;
            }

            filteredSource.erase(filteredSource.begin() + removal.SourceIndex);
            filteredSource.insert(filteredSource.begin() + newSourceIndex, moved);
            return true;
        }
        default:
            return false; // Reset / multi-item -> full rebuild
        }
    };

    try
    {
        return apply();
    }
    catch (...)
    {
        if (!isPlanCurrent())
        {
            return false;
        }
        throw;
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

bool ShapedItemsSource::CanProjectWithRowIdentity() const
{
    // A shaped projection validated its rows when it was built.
    if (m_kind != ProjectionKind::None && m_kind != ProjectionKind::Unshaped &&
        m_validatedSourceStamp == m_sourceChangeStamp)
    {
        return true;
    }

    // Mirrors Refresh: identity is validated over the rows a shaped projection would publish.
    auto rows = Materialize(m_source);
    // A hierarchy keeps filtered-out roots that have a matching descendant; validate them all.
    if (!m_childrenSelector)
    {
        ApplyFilter(rows);
    }

    wchar_t const* reason = nullptr;
    if (!ValidateRowIdentities(rows, reason))
    {
        LogIdentityProjectionDisabled(reason);
        return false;
    }
    return true;
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

std::vector<ShapingHelpers::KeyedBucket> ShapedItemsSource::BucketizeGroupsOrThrow(std::vector<winrt::IInspectable> const& items)
{
    std::vector<ShapingHelpers::KeyedBucket> keyedBuckets;
    wchar_t const* rejectReason = nullptr;
    const bool grouped = ShapingHelpers::BucketizeToGroups(
        items,
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
        rejectReason);

    if (!grouped)
    {
        // Spec contract (same shape as the row-identity fail-fast): an unresolvable,
        // unstable, or colliding *group* identity is a caller bug — the GroupBy(...) key
        // selector (and optional groupIdentitySelector) must produce a stable, non-empty
        // string identity per bucket, without two distinct group-key instances collapsing
        // to the same identity unless the app opted in via groupIdentitySelector. Silently
        // flattening the projection would let the app ship with grouping mysteriously "not
        // working" and no diagnostic.
        //
        // Thrown, not asserted: caller data, and an assert would skip the unwind in checked builds.
        LogIdentityProjectionDisabled(rejectReason);
        winrt::hstring message = Diagnostic(L"GroupBy key selector produced an invalid group identity");
        if (rejectReason)
        {
            message = message + L": " + winrt::hstring{ rejectReason };
        }
        throw winrt::hresult_invalid_argument(message);
    }

    return keyedBuckets;
}

winrt::com_ptr<ShapedGroup> ShapedItemsSource::AcquireCachedGroup(ShapingHelpers::KeyedBucket const& bucket)
{
    winrt::com_ptr<ShapedGroup> group;
    auto cacheIt = m_groupCache.find(bucket.Identity);
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
        m_groupCache.emplace(bucket.Identity, group);
    }

    group->GroupKey(bucket.Identity);
    return group;
}

void ShapedItemsSource::PruneGroupCache(std::unordered_set<winrt::hstring> const& liveKeys)
{
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
}

void ShapedItemsSource::AttachGroupSource(std::vector<winrt::IInspectable> const& groups)
{
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
}

void ShapedItemsSource::ReattachGroupedAdapterAfterFailedRefill()
{
    // Reattach so a refill that threw after the quiet detach does not orphan the adapter.
    if (m_groupedAdapter && m_groupSource && !m_groupedAdapter->Source())
    {
        m_groupedAdapter->Source(m_groupSource);
    }
}

void ShapedItemsSource::ReleaseGroupedProjection()
{
    if (m_groupSource)
    {
        // Detach the adapter BEFORE clearing the internal group source. Otherwise Clear() fires
        // the adapter's outer-source subscription, which rebuilds synchronously and raises an
        // empty Reset into the ItemsRepeater still bound to the old grouped Entries (with realized
        // rows) -- an assertion failure / fault mid-teardown. The caller's PublishProjection is the
        // single controlled swap that moves the row axis to the new projection.
        if (m_groupedAdapter)
        {
            m_groupedAdapter->DetachSourceQuietly();
        }
        m_groupSource.Clear();
    }
    m_groupCache.clear();
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
    ReleaseGroupedProjection();
    m_projectedAsGrouped = false;
    PublishProjection();
}

void ShapedItemsSource::Refresh()
{

    // Re-entrancy guard: a source notification that arrives while a rebuild is in flight
    // (e.g. an app mutating the source from a filter/sort/group callback) must not re-enter
    // ReplaceAll on the projection. Remember it and run one coalesced rebuild after the outer
    // rebuild unwinds so changes after materialization are not lost.
    //
    // A group re-slice and a teardown completion are guarded the same way: a nested rebuild would
    // replace groups under their loops.
    if (m_isRefreshing || m_reslicingGroups)
    {
        m_pendingRefresh = true;
        return;
    }

    bool const wasProjectedAsGrouped = m_projectedAsGrouped;
    bool runPendingRefresh = false;
    ++m_projectionGeneration;
    try
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
        m_refreshSourceStamp = m_sourceChangeStamp;

        // Cleared before the re-read: an off-thread change dropped while the items are being read
        // keeps the flag set, so the next live change still forces a rebuild.
        m_liveShapingChangeDropped->store(false);
        auto rows = Materialize(authoritativeSource);
        if (m_rowsReleasedHook)
        {
            m_rowsReleasedHook(nullptr, rows);
        }
        RefreshLiveShapingSubscriptions(rows);

        // One structure pass across the retention pre-walk and the adapter's emit walk, so each
        // node's children are validated (selector call, and enumeration if changed) once per
        // refresh rather than once per walk.
        std::optional<HierarchyStructureCache::Pass> structurePass;
        if (m_childrenSelector)
        {
            EnsureStructureCache();
            structurePass.emplace(m_structureCache);

            // The full, unfiltered root set: reachability is measured from it, so a root a filter
            // hides keeps its cached subtree.
            m_structureCache->Roots(rows);
        }

        // All snapshots were just recaptured, so any pending live restore is satisfied -- unless
        // the rebuild below throws, in which case the catch re-marks it.
        m_liveShapingDirty = false;

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
            // Eager hierarchy: the whole tree is walked and shaped here, before the roots are
            // filtered, and that walk derives retention too. Lazy: retention alone, by its own walk.
            const bool eager = IsEagerHierarchy();
            if (!eager)
            {
                m_hierarchyIndex = nullptr;
            }
            if (m_childrenSelector && m_pipeline.HasFilter())
            {
                // Ancestor retention: a root survives when it or any descendant matches.
                if (eager)
                {
                    // Every verdict is re-read (a refresh is how an app reports changes nobody
                    // observed), but only the lists whose kept membership moved are re-shaped.
                    UpdateHierarchyIndex(rows, true /* retention */, true /* reevaluateAll */);
                }
                else
                {
                    ComputeFilterRetention(rows);
                }
                rows.erase(
                    std::remove_if(rows.begin(), rows.end(), [this](auto const& row) { return !IsFilterKept(row); }),
                    rows.end());
            }
            else
            {
                if (eager)
                {
                    // Re-shaped in full only when the sort changed (the cache's log is then Full);
                    // otherwise only what moved since the index was last current. Leaves retention
                    // cleared either way.
                    UpdateHierarchyIndex(rows, false /* retention */, false /* reevaluateAll */);
                }
                else
                {
                    ClearFilterRetention();
                }
                ApplyFilter(rows);
            }

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
            m_validatedSourceStamp = m_refreshSourceStamp;
        }
        MUX_ASSERT(m_source == authoritativeSource);
    }
    catch (...)
    {
        // The snapshots may already describe data the kept projection does not show (a transient
        // duplicate key, say). Stay dirty without posting, so the next live change retries the
        // full rebuild instead of matching the fresh snapshots and dropping the change.
        if (IsLiveShapingEnabled())
        {
            m_liveShapingDirty = true;
        }
        m_validatedSourceStamp = ~uint64_t{ 0 };

        // A request that arrived during the failed pass describes newer state; post it (inline
        // would throw into the same app call). Only such requests replay, so bad data cannot loop.
        // Contained: a scheduling failure must not replace the actionable exception rethrown below.
        if (runPendingRefresh)
        {
            try { ScheduleRefreshReplay(); } catch (...) {}
        }

        throw;
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

void ShapedItemsSource::PostPendingRefresh() noexcept
{
    if (!m_pendingRefresh)
    {
        return;
    }

    try
    {
        ScheduleRefreshReplay();
    }
    catch (...)
    {
    }

    // Kept if nothing was posted, so the next change rebuilds in full.
    if (m_refreshReplayScheduled)
    {
        m_pendingRefresh = false;
    }
}

void ShapedItemsSource::ScheduleRefreshReplay()
{
    if (m_refreshReplayScheduled)
    {
        return;
    }

    // No dispatcher (test host): nothing to post to; the next reshape re-materializes anyway.
    auto const queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
    if (!queue)
    {
        return;
    }

    std::weak_ptr<ShapedItemsSource> weakThis = weak_from_this();
    m_refreshReplayScheduled = queue.TryEnqueue([weakThis]()
    {
        auto strongThis = weakThis.lock();
        if (!strongThis)
        {
            return;
        }

        strongThis->m_refreshReplayScheduled = false;
        try
        {
            strongThis->Refresh();
        }
        catch (...)
        {
            // No app call to receive this; the previous projection stays and the next verb or
            // source change re-throws.
        }
    });
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
    RebuildFlatItemPropertyChangedTracking(rows);

    ReleaseHierarchyProjection();

    // Releasing any prior grouped projection: switching grouped->flat must not retain the stale
    // group observable/cache. They are rebuilt from scratch by RebuildGrouped on the next GroupBy,
    // so holding them here only leaks the previous grouping (and its cached ShapedGroups).
    ReleaseGroupedProjection();
    m_projectedAsGrouped = false;
    PublishProjection();
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

    auto keyedBuckets = BucketizeGroupsOrThrow(rows);

    // Release any leftover hierarchy adapter, but only after bucketize: if that throws, the
    // previous projection must stay whole.
    ReleaseHierarchyProjection();

    if (!m_groupSource)
    {
        m_groupSource = winrt::single_threaded_observable_vector<winrt::IInspectable>();
    }

    std::vector<winrt::IInspectable> groups;
    groups.reserve(keyedBuckets.size());
    std::unordered_set<winrt::hstring> liveKeys;

    // The flat shaped rows kept under grouping: every kept row, in group order, with the
    // per-bucket sort applied and no header entries. Accumulated while the per-bucket sort walks
    // the buckets in their final order. This keeps
    // Rows() coherent as the flat shaped projection even while the presented row axis is the
    // grouped adapter.
    std::vector<winrt::IInspectable> flatRows;
    flatRows.reserve(rows.size());

    for (auto& bucket : keyedBuckets)
    {
        ApplySort(bucket.Items, m_pipeline.GroupOrder(), -1);
        flatRows.insert(flatRows.end(), bucket.Items.begin(), bucket.Items.end());
    }

    // The presented row axis under grouping is the adapter's computed ItemsSourceView, which has
    // no vector form. Keep Rows() maintained as the flat shaped projection anyway: without this
    // write m_rows would keep whatever the last FLAT rebuild left -- for the canonical
    // From(...).GroupBy(...) chain that is the raw, unshaped source -- so shaping verbs applied
    // AFTER GroupBy would not be reflected in the flat projection.
    //
    // Expansion is deliberately not applied: this is the shaped DATA. Collapsing a group hides
    // rows from the presented row axis without removing them from the projection, and a flat
    // vector whose size changed when a chevron is clicked would be reporting UI state.
    //
    // Written before the groups are filled: the adapter's Reset below closes an open edit, which
    // asks Rows() whether the edited item is still present.
    m_rows.ReplaceAll(flatRows);

    // Detach first: a live adapter would rebuild per changed group, briefly publishing a moved row
    // in no group, which drops its selection.
    if (m_groupedAdapter)
    {
        m_groupedAdapter->DetachSourceQuietly();
    }

    try
    {
        for (auto& bucket : keyedBuckets)
        {
            auto group = AcquireCachedGroup(bucket);
            group->SetItems(bucket.Items);
            groups.push_back(group.as<winrt::IInspectable>());
            liveKeys.insert(bucket.Identity);
        }
    }
    catch (...)
    {
        ReattachGroupedAdapterAfterFailedRefill();
        throw;
    }

    PruneGroupCache(liveKeys);
    AttachGroupSource(groups);

    m_kind = ProjectionKind::Grouped;
    m_projectedAsGrouped = true;
    PublishProjection();
}

void ShapedItemsSource::RebuildHierarchical(std::vector<winrt::IInspectable>& rows)
{
    // Rows come from an adapter now, so the in-place path must not re-sort stale layer-1 state.
    InvalidateShapingState();
    ClearFlatRowIdentityTracking();

    // `rows` arrives already filtered. Sorting it here shapes the ROOT sibling set; every deeper
    // level is shaped by the ShapeSiblings callback as the adapter's walk reaches it.
    ApplySort(rows);

    // Release any prior grouped projection so its adapter stops publishing headers.
    ReleaseGroupedProjection();
    m_hierarchyGroups.clear();
    m_rootBucketIndex.clear();

    // Ungrouped: one segment spanning every root.
    EnsureHierarchicalAdapter(rows, true /* shapeRoots */);

    // Rows() mirrors the visible rows as a SNAPSHOT; it does not track later expand/collapse.
    // Consumers needing live visible rows read the adapter.
    m_rows.ReplaceAll(VisibleHierarchicalRows());

    m_kind = ProjectionKind::Hierarchical;
    // No header rows, so not grouped.
    m_projectedAsGrouped = false;
    PublishProjection();
}

// Both axes. Row stream:
//   [Header A] [rootA1] [rootA1's expanded subtree...] [rootA2] ... [Header B] [rootB1] ...
// The group key applies to ROOTS only; a descendant keeps its depth under its parent.
void ShapedItemsSource::RebuildGroupedHierarchical(std::vector<winrt::IInspectable>& rows)
{
    // Bucket like RebuildGrouped: pre-GroupBy sorts set header order; later sorts apply per bucket.
    ApplySort(rows, -1, m_pipeline.GroupOrder());

    // Same fail-fast contract as RebuildGrouped; nothing is mutated yet.
    auto keyedBuckets = BucketizeGroupsOrThrow(rows);

    InvalidateShapingState();
    ClearFlatRowIdentityTracking();

    // Roots bucket by bucket. Each bucket's roots must be contiguous: the re-slice cuts entries at
    // depth-0 rows.
    std::vector<winrt::IInspectable> orderedRoots;
    orderedRoots.reserve(rows.size());
    std::vector<size_t> segments;
    segments.reserve(keyedBuckets.size());
    std::unordered_map<void*, size_t> rootBucketIndex;

    for (size_t bucketIndex = 0; bucketIndex < keyedBuckets.size(); ++bucketIndex)
    {
        auto& bucket = keyedBuckets[bucketIndex];
        ApplySort(bucket.Items, m_pipeline.GroupOrder(), -1);
        for (auto const& root : bucket.Items)
        {
            // Keyed by raw COM pointer: the re-slice looks up the same objects.
            rootBucketIndex[winrt::get_abi(root)] = bucketIndex;
            orderedRoots.push_back(root);
        }
        segments.push_back(bucket.Items.size());
    }

    m_rootBucketIndex = std::move(rootBucketIndex);
    // Keep the adapter attach below from re-slicing stale groups.
    m_hierarchyGroups.clear();

    // shapeRoots=false: the root order is the GROUP order; sorting the roots among themselves
    // would interleave the buckets and break the contiguity the re-slice depends on.
    EnsureHierarchicalAdapter(orderedRoots, false /* shapeRoots */, std::move(segments));

    if (!m_groupSource)
    {
        m_groupSource = winrt::single_threaded_observable_vector<winrt::IInspectable>();
    }

    // Reuse cached groups as RebuildGrouped does, so Group() objects do not churn.
    std::vector<winrt::IInspectable> groups;
    groups.reserve(keyedBuckets.size());
    std::unordered_set<winrt::hstring> liveKeys;
    m_hierarchyGroups.reserve(keyedBuckets.size());

    for (auto const& bucket : keyedBuckets)
    {
        auto group = AcquireCachedGroup(bucket);
        // Items (roots plus visible descendants) are filled by ResliceGroupsFromHierarchy.
        m_hierarchyGroups.push_back(group);
        groups.push_back(group.as<winrt::IInspectable>());
        liveKeys.insert(bucket.Identity);
    }

    PruneGroupCache(liveKeys);

    // Fill groups BEFORE attaching the grouped adapter so it publishes one coherent Reset.
    if (m_groupedAdapter)
    {
        m_groupedAdapter->DetachSourceQuietly();
    }
    try
    {
        ResliceGroupsFromHierarchy();
    }
    catch (...)
    {
        ReattachGroupedAdapterAfterFailedRefill();
        throw;
    }
    AttachGroupSource(groups);

    // Rows(): visible rows in group order, headers excluded.
    m_rows.ReplaceAll(VisibleHierarchicalRows());

    m_kind = ProjectionKind::GroupedHierarchical;
    // Presented via the grouped adapter, so header rows exist.
    m_projectedAsGrouped = true;
    PublishProjection();
}

void ShapedItemsSource::ReleaseHierarchyProjection()
{
    if (m_hierarchicalAdapter)
    {
        // Callback first: DetachSourceQuietly must not be able to drive a re-slice on the way out.
        m_hierarchicalAdapter->ProjectionChanged(nullptr);
        m_hierarchicalAdapter->DetachSourceQuietly();

        // The adapter is REUSED across hierarchy declarations, so it outlives the one being
        // retracted here. Its contract callbacks are app code (a children selector that captured a
        // view model roots it for the adapter's lifetime), so release them too.
        m_hierarchicalAdapter->ReleaseCallbacks();
    }

    if (m_hierarchySource)
    {
        m_hierarchySource.Clear();
    }

    ClearFilterRetention();
    ResetStructureCache();
    m_structureFlushPending = false;
    m_hierarchyGroups.clear();
    m_rootBucketIndex.clear();
}

void ShapedItemsSource::EnsureHierarchicalAdapter(std::vector<winrt::IInspectable> const& roots, bool shapeRoots, std::vector<size_t> rootSegments)
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
        // Detach quietly so the ReplaceAll below fires no subscription and the Source() re-attach
        // rebuilds exactly once against the fully-populated roots.
        m_hierarchicalAdapter->DetachSourceQuietly();
    }

    // Drop the old callback first: it would re-slice groups this rebuild is replacing.
    m_hierarchicalAdapter->ProjectionChanged(nullptr);

    // This rebuild reads the structure afresh, so a coalesced change still pending is part of it.
    m_structureFlushPending = false;

    // Shared with the retention pre-walk. Installed before any setter below, each of which
    // rebuilds, so the adapter never mints a private cache of its own.
    EnsureStructureCache();
    m_hierarchicalAdapter->StructureCache(m_structureCache);

    // A filter hides live nodes from the walk; their expansion intent must survive it.
    m_hierarchicalAdapter->PartialShape(m_pipeline.HasFilter());

    // Per-level shaping. Runs INSIDE the adapter's walk, so it touches nothing but the vector it is
    // handed (see the re-entrancy contract on ShapeSiblingsFn). Under an active filter the kept set
    // from ComputeFilterRetention decides membership, so context rows survive.
    std::weak_ptr<ShapedItemsSource> weakThis = weak_from_this();
    m_hierarchicalAdapter->ShapeSiblings(
        [weakThis](std::vector<winrt::IInspectable>& siblings)
        {
            if (auto strongThis = weakThis.lock())
            {
                strongThis->ShapeHierarchySiblings(siblings);
            }
        });

    // Context rows (kept only for a matching descendant) present from the adapter's filter overlay.
    // Installed before Source() so the attach walk already sees them.
    if (m_filterRetentionActive)
    {
        m_hierarchicalAdapter->ContextSelector(
            [weakThis](winrt::IInspectable const& item)
            {
                auto strongThis = weakThis.lock();
                return strongThis && strongThis->IsFilterContext(item);
            });
    }
    else
    {
        m_hierarchicalAdapter->ContextSelector(nullptr);
    }

    m_hierarchicalAdapter->ChildrenSelector(m_childrenSelector);
    m_hierarchicalAdapter->HasChildrenSelector(m_hasChildrenSelector);
    // Expansion verbs act on the projection, so a coalesced structure change is applied first.
    m_hierarchicalAdapter->PendingChangesFlusher(
        [weakThis]()
        {
            if (auto strongThis = weakThis.lock())
            {
                strongThis->FlushPendingStructureChanges();
            }
        });
    // Per-bucket root counts, so a grouped root's sibling position is within its bucket.
    m_hierarchicalAdapter->RootSegments(std::move(rootSegments));

    // Populate before attaching, for the same coherence reason as the grouped path.
    m_hierarchySource.ReplaceAll(roots);
    // The eager build's result, if this refresh produced one; the attach walk emits from it.
    m_hierarchicalAdapter->SetHierarchyIndex(IsEagerHierarchy() ? m_hierarchyIndex : nullptr);
    m_hierarchicalAdapter->Source(m_hierarchySource);

    // Under grouping, node toggles splice only the hierarchy adapter; this carries them to the
    // groups. Uses the COHERENT edge: raw vector events fire per row, mid-repair.
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
    // Pin/snapshot state up front: SetItems below runs app handlers.
    auto const adapter = m_hierarchicalAdapter;
    if (!adapter || m_hierarchyGroups.empty())
    {
        return;
    }

    // SetItems triggers a grouped rebuild that must not re-enter. UI-thread only, so a plain bool.
    if (m_reslicingGroups)
    {
        return;
    }

    {
        m_reslicingGroups = true;
        auto guard = wil::scope_exit([this]() noexcept { m_reslicingGroups = false; });

        // On a throwing handler, still post a Refresh deferred behind the re-slice.
        auto completion = wil::scope_exit([this]() noexcept
        {
            m_reslicingGroups = false;
            if (!m_isRefreshing && !m_isApplyingIncrementalChange && std::exchange(m_pendingRefresh, false))
            {
                try { ScheduleRefreshReplay(); } catch (...) {}
            }
        });

        auto const groups = m_hierarchyGroups;

        std::vector<std::vector<winrt::IInspectable>> slices(groups.size());
        // Header counts roots, not the descendant rows a slice also holds.
        std::vector<int32_t> rootCounts(groups.size(), 0);

        auto const entries = adapter->Entries();
        const int32_t count = entries ? entries.Count() : 0;

        // Each depth-0 row opens its bucket; following rows belong to it until the next root.
        size_t currentBucket = 0;
        bool haveBucket = false;
        for (int32_t i = 0; i < count; ++i)
        {
            auto const* const node = adapter->TryGetNodeRow(i);
            if (!node)
            {
                continue;
            }

            if (node->Depth == 0)
            {
                auto const it = m_rootBucketIndex.find(winrt::get_abi(node->Item));
                if (it == m_rootBucketIndex.end())
                {
                    // Unknown root (source mutated; rebuild queued): drop its subtree.
                    haveBucket = false;
                    continue;
                }
                currentBucket = it->second;
                haveBucket = true;
                if (currentBucket < rootCounts.size())
                {
                    ++rootCounts[currentBucket];
                }
            }

            if (haveBucket && currentBucket < slices.size())
            {
                slices[currentBucket].push_back(node->Item);
            }
        }

        for (size_t i = 0; i < groups.size(); ++i)
        {
            // Count before items: SetItems re-mints the header from the count.
            groups[i]->GroupChildCount(rootCounts[i]);
            groups[i]->SetItems(slices[i]);
        }

        completion.release();
    }

    // Replay a deferred Refresh unless an outer Refresh/incremental pass will.
    if (!m_isRefreshing && !m_isApplyingIncrementalChange && std::exchange(m_pendingRefresh, false))
    {
        Refresh();
    }
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

winrt::hstring ShapedItemsSource::Diagnostic(std::wstring_view text) const
{
    return m_diagnosticName + L": " + winrt::hstring{ text };
}

void const* ShapedItemsSource::LiveShapingKeyFor(winrt::IInspectable const& item)
{
    if (!item)
    {
        return nullptr;
    }

    auto const unknown = item.as<winrt::Windows::Foundation::IUnknown>();
    return winrt::get_abi(unknown);
}

ShapedItemsSource::LiveShapeSnapshot ShapedItemsSource::CaptureLiveShapeSnapshot(
    winrt::IInspectable const& item) const
{
    LiveShapeSnapshot snapshot{};

    // A throwing selector counts as null, as in the rebuild, so it can't escape the app's setter.
    auto const keyOf = [&item](ShapingHelpers::KeySelector const& selector) -> winrt::hstring
    {
        if (!selector)
        {
            return {};
        }
        try { return RowIdentity::StringifyKey(selector(item)); } catch (...) { return RowIdentity::StringifyKey(nullptr); }
    };

    // Called only with live shaping on, so every shape input is captured.
    for (auto const& axis : m_pipeline.ActiveSortAxes(-1, -1))
    {
        snapshot.SortKeys.push_back(keyOf(axis.Key));
    }

    if (m_groupSelector)
    {
        // Compare by group identity, not display text (keys often print alike, hiding a regroup).
        winrt::IInspectable groupKey{ nullptr };
        try { groupKey = m_groupSelector(item); } catch (...) {}
        winrt::hstring identity;
        wchar_t const* reason = nullptr;
        if (TryGetGroupIdentity(groupKey, identity, reason))
        {
            snapshot.GroupKey = L"id:" + identity;
        }
        else
        {
            try { snapshot.GroupKey = L"key:" + ShapingHelpers::ValueKey::ToObjectLookupKey(groupKey, false); } catch (...) {}
        }
    }

    snapshot.PassesFilter = m_pipeline.PassesFilter(item);

    return snapshot;
}

bool ShapedItemsSource::LiveShapeSnapshotsDiffer(
    LiveShapeSnapshot const& left,
    LiveShapeSnapshot const& right)
{
    return left.PassesFilter != right.PassesFilter ||
        left.GroupKey != right.GroupKey ||
        left.SortKeys != right.SortKeys;
}

void ShapedItemsSource::RefreshLiveShapingSubscriptions(
    std::vector<winrt::IInspectable> const& items)
{
    // The children-selector hierarchy observes its items through the structure cache instead.
    if (!IsLiveShapingEnabled() || m_childrenSelector)
    {
        ClearLiveShapingSubscriptions();
        return;
    }

    // Mark-and-sweep: only arrivals subscribe and departures revoke, avoiding per-row COM churn.
    std::unordered_set<void const*> live;
    live.reserve(items.size());

    // Snapshots are rebuilt, not reconciled: the committed spec may have changed.
    std::unordered_map<void const*, LiveShapeSnapshot> snapshots;
    snapshots.reserve(items.size());

    for (auto const& item : items)
    {
        if (!item)
        {
            continue;
        }

        auto const key = LiveShapingKeyFor(item);
        live.insert(key);
        m_liveShaping->Subscribe(item);
        snapshots[key] = CaptureLiveShapeSnapshot(item);
    }

    m_liveShaping->RetainOnly(live);
    m_liveShapeSnapshots = std::move(snapshots);
}

void ShapedItemsSource::AddLiveShapingSubscription(winrt::IInspectable const& item)
{
    if (!IsLiveShapingEnabled() || m_childrenSelector || !item)
    {
        return;
    }

    m_liveShaping->Subscribe(item);
    m_liveShapeSnapshots[LiveShapingKeyFor(item)] = CaptureLiveShapeSnapshot(item);

    // A re-inserted item may still hold a rank from before it was edited while out of the source.
    if (m_liveItemChangedHook)
    {
        m_liveItemChangedHook(item, true);
    }
}

void ShapedItemsSource::RemoveLiveShapingSubscription(winrt::IInspectable const& item)
{
    // Not gated on IsLiveShapingEnabled: pruning leftovers is always correct. Gated on there being
    // anything to prune, so a source that never went live pays no identity QI per removal.
    if (!item || (m_liveShaping->Empty() && m_liveShapeSnapshots.empty()))
    {
        return;
    }

    m_liveShaping->Unsubscribe(item);
    m_liveShapeSnapshots.erase(LiveShapingKeyFor(item));
}

void ShapedItemsSource::ApplyLiveShapingDelta(
    winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
{
    if (!IsLiveShapingEnabled() || m_childrenSelector)
    {
        return;
    }

    using winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;

    auto const unsubscribeAll = [this](winrt::Microsoft::UI::Xaml::Interop::IBindableVector const& items)
    {
        if (!items)
        {
            return;
        }
        for (uint32_t i = 0; i < items.Size(); ++i)
        {
            RemoveLiveShapingSubscription(items.GetAt(i));
        }
    };

    auto const subscribeAll = [this](winrt::Microsoft::UI::Xaml::Interop::IBindableVector const& items)
    {
        if (!items)
        {
            return;
        }
        for (uint32_t i = 0; i < items.Size(); ++i)
        {
            AddLiveShapingSubscription(items.GetAt(i));
        }
    };

    switch (args.Action())
    {
    case NotifyCollectionChangedAction::Add:
        subscribeAll(args.NewItems());
        break;
    case NotifyCollectionChangedAction::Remove:
        unsubscribeAll(args.OldItems());
        break;
    case NotifyCollectionChangedAction::Replace:
        // Remove before add, so a replace that reuses the same object ends up subscribed.
        unsubscribeAll(args.OldItems());
        subscribeAll(args.NewItems());
        break;
    case NotifyCollectionChangedAction::Move:
        // Ordering only; subscriptions unaffected.
        break;
    case NotifyCollectionChangedAction::Reset:
    default:
        // Callers route resets to Refresh(), which reconciles.
        break;
    }
}

void ShapedItemsSource::ClearLiveShapingSubscriptions()
{
    m_liveShaping->UnsubscribeAll();
    m_liveShapeSnapshots.clear();
    // Nothing tracked, so nothing live is pending: a posted restore finds the flag clear and
    // returns, and a later re-enable starts from a fresh restore.
    m_liveShapingDirty = false;
    m_liveShapingHeld = false;
    m_liveShapingChangeDropped->store(false);
}

void ShapedItemsSource::ResubscribeLiveShapingFromSource()
{
    if (!IsLiveShapingEnabled())
    {
        ClearLiveShapingSubscriptions();
        return;
    }

    RefreshLiveShapingSubscriptions(Materialize(m_source));
}

void ShapedItemsSource::OnLiveShapedItemChanged(
    winrt::IInspectable const& item,
    winrt::hstring const&)
{
    if (!IsLiveShapingEnabled() || m_childrenSelector || !item)
    {
        return;
    }

    // A change the owner could not be told about off-thread: snapshots cannot reveal it, so force
    // the full restore, which re-reads every item.
    if (m_liveShapingChangeDropped->exchange(false))
    {
        MarkLiveShapingDirty();
    }

    // Before the coalescing early-out: the owner must see every changed item.
    if (m_liveItemChangedHook && m_liveItemChangedHook(item, m_liveShapingDirty))
    {
        MarkLiveShapingDirty();
        return;
    }

    // A full restore is already pending; skip snapshot capture so bulk edits cost one capture.
    // Still re-arm it: an earlier post may have been refused, failed, or parked behind a hold
    // that has since lifted.
    if (m_liveShapingDirty)
    {
        EnsureLiveShapingRestore();
        return;
    }

    // Discards changes no verb reads. Compare, don't store: the cached snapshot must keep matching
    // the projection until the restore runs.
    auto const key = LiveShapingKeyFor(item);
    auto const existing = m_liveShapeSnapshots.find(key);
    if (existing != m_liveShapeSnapshots.end() &&
        !LiveShapeSnapshotsDiffer(existing->second, CaptureLiveShapeSnapshot(item)))
    {
        return;
    }

    MarkLiveShapingDirty();
}

void ShapedItemsSource::MarkLiveShapingDirty()
{
    m_liveShapingDirty = true;
    EnsureLiveShapingRestore();
}

void ShapedItemsSource::EnsureLiveShapingRestore()
{
    if (!m_liveShapingDirty || m_liveShapingRestorePosted)
    {
        return;
    }

    if (m_liveShapingHeld)
    {
        // A held restore normally waits for ResumeHeldLiveShaping. If the hold has since lifted
        // without one (the owner was destroyed mid-edit), re-post so live shaping does not stall.
        if (m_liveShapingHold && m_liveShapingHold())
        {
            return;
        }
        m_liveShapingHeld = false;
    }

    // Posted: coalesces a turn's writes into one rebuild and stays out of the app's
    // PropertyChanged handler.
    PostLiveShapingRestore();
}

void ShapedItemsSource::PostLiveShapingRestore()
{
    if (m_liveShapingRestorePosted)
    {
        return;
    }

    if (m_ownerQueue)
    {
        auto weakThis = weak_from_this();
        // A refused post means the owner's queue is shutting down. Stay dirty rather than rebuild
        // inside the app's PropertyChanged; the next change re-arms it.
        m_liveShapingRestorePosted = m_ownerQueue.TryEnqueue([weakThis]()
            {
                if (auto const strongThis = weakThis.lock())
                {
                    strongThis->m_liveShapingRestorePosted = false;
                    try
                    {
                        strongThis->RestoreLiveShaping();
                    }
                    catch (...)
                    {
                        // No app call to receive this; Refresh left the source dirty, so the
                        // next live change retries.
                    }
                }
            });
        return;
    }

    // The owner thread has no dispatcher at all (test host), so there is no later turn to post to.
    // This is the one case that restores inside the caller; contain failures as the posted path does.
    try
    {
        RestoreLiveShaping();
    }
    catch (...)
    {
    }
}

void ShapedItemsSource::RestoreLiveShaping()
{
    if (!m_liveShapingDirty)
    {
        // Another refresh already recaptured every snapshot.
        return;
    }

    if (!IsLiveShapingEnabled())
    {
        // Live shaping turned off while in flight.
        m_liveShapingDirty = false;
        return;
    }

    if (m_liveShapingHold && m_liveShapingHold())
    {
        // Moving rows would end the edit; stay dirty until the owner resumes.
        m_liveShapingHeld = true;
        return;
    }

    Refresh();

    // No verb ran, so notify UIA; rows may have regrouped or filtered, not just reordered.
    RaiseShapingChanged(false /* reorderOnly */);
}

void ShapedItemsSource::ResumeHeldLiveShaping()
{
    if (!std::exchange(m_liveShapingHeld, false))
    {
        return;
    }

    // Posted: caller is closing an edit, often inside app callbacks or layout. Skips the post when a
    // refresh in the meantime already satisfied the restore.
    EnsureLiveShapingRestore();
}

void ShapedItemsSource::ClearFilterRetention()
{
    // The cached shaped lists were filtered by the kept set; without it they filter by predicate.
    if (m_filterRetentionActive && m_structureCache)
    {
        m_structureCache->InvalidateShapedChildren();
    }
    m_filterRetentionActive = false;
    m_filterKept.clear();
    m_filterContext.clear();
    m_filterKeepAlive.clear();
}

void ShapedItemsSource::ResetFilterContextOverlay()
{
    if (m_hierarchicalAdapter)
    {
        m_hierarchicalAdapter->ResetContextOverlay();
    }
}

std::optional<bool> ShapedItemsSource::ContainsHierarchyItem(winrt::IInspectable const& item) const
{
    if (!m_childrenSelector || !m_hierarchicalAdapter)
    {
        return std::nullopt;
    }
    if (m_filterRetentionActive)
    {
        return IsFilterKept(item);
    }
    return m_structureCache && m_structureCache->IsReachable(item);
}

bool ShapedItemsSource::IsFilterKept(winrt::IInspectable const& item) const
{
    return item && m_filterKept.find(winrt::get_abi(item)) != m_filterKept.end();
}

bool ShapedItemsSource::IsFilterContext(winrt::IInspectable const& item) const
{
    return item && m_filterContext.find(winrt::get_abi(item)) != m_filterContext.end();
}

void ShapedItemsSource::ComputeFilterRetention(std::vector<winrt::IInspectable> const& roots)
{
    // The previous kept set decides whether the cached shaped lists, which were filtered by it,
    // survive. Its keep-alive is held until the comparison is done so none of its pointers can be
    // recycled by a new object in the meantime.
    const bool wasActive = m_filterRetentionActive;
    auto const previousKept = std::move(m_filterKept);
    auto const previousKeepAlive = std::move(m_filterKeepAlive);
    m_filterKept.clear();
    m_filterContext.clear();
    m_filterKeepAlive.clear();
    m_filterRetentionActive = true;

    std::unordered_map<void*, bool> memo;
    std::unordered_set<void*> onPath;

    // Children come from the shared structure cache, inside the pass Refresh holds, so the
    // adapter's emit walk that follows reuses every selector call and enumeration made here, and
    // the cache keeps observing every collection read here, hidden or not.
    auto& structure = EnsureStructureCache();

    // Shaped lists filtered by the previous kept set are dropped unless the new set turns out to be
    // identical -- including when the walk below does not complete.
    auto invalidateShaped = wil::scope_exit([&structure]() noexcept { structure.InvalidateShapedChildren(); });
    // Post-order: a node is kept when it matches or any child is kept. Every child is visited (no
    // short-circuit) so the whole subtree is memoized and its collections observed by the cache.
    // A cycle or an over-deep chain stops the descent here; the adapter's walk reports it if it
    // becomes visible.
    std::function<bool(winrt::IInspectable const&, int32_t)> visit =
        [&](winrt::IInspectable const& item, int32_t depth) -> bool
    {
        void* const abi = winrt::get_abi(item);
        if (auto it = memo.find(abi); it != memo.end())
        {
            return it->second;
        }

        const bool match = m_pipeline.PassesFilter(item);
        bool descendantKept = false;
        if (depth < HierarchicalSourceAdapter::c_maxHierarchyDepth && onPath.insert(abi).second)
        {
            // Shared, not copied: nothing here shapes it, and the cache never mutates a published
            // set in place.
            auto const children = structure.ChildrenOf(item, m_childrenSelector);

            for (auto const& child : *children)
            {
                if (child && visit(child, depth + 1))
                {
                    descendantKept = true;
                }
            }
            onPath.erase(abi);
        }

        const bool kept = match || descendantKept;
        memo.emplace(abi, kept);
        m_filterKeepAlive.push_back(item);
        if (kept)
        {
            m_filterKept.insert(abi);
            if (!match)
            {
                m_filterContext.insert(abi);
            }
        }
        return kept;
    };

    for (auto const& root : roots)
    {
        if (root)
        {
            visit(root, 0);
        }
    }

    if (wasActive && previousKept == m_filterKept)
    {
        invalidateShaped.release();
    }
}

void ShapedItemsSource::ShapeHierarchySiblings(std::vector<winrt::IInspectable>& siblings) const
{
    // Under retention the kept set decides membership, so context rows survive.
    if (m_filterRetentionActive)
    {
        siblings.erase(
            std::remove_if(siblings.begin(), siblings.end(), [this](auto const& item) { return !IsFilterKept(item); }),
            siblings.end());
    }
    else
    {
        ApplyFilter(siblings);
    }
    ApplySort(siblings);
}

std::shared_ptr<HierarchyIndex> ShapedItemsSource::BuildHierarchyIndex(
    std::vector<winrt::IInspectable> const& roots,
    bool computeRetention)
{
    constexpr int32_t c_maxDepth = HierarchicalSourceAdapter::c_maxHierarchyDepth;
    auto& structure = EnsureStructureCache();

    m_buildingHierarchyIndex = true;
    m_structureChangedDuringBuild = false;
    auto buildingGuard = wil::scope_exit([this]() noexcept { m_buildingHierarchyIndex = false; });

    // Retention, when asked for, is derived exactly as ComputeFilterRetention derives it (same
    // memo, same cycle and depth cut-offs, same bookkeeping); only the walk is iterative and shared.
    const bool wasActive = m_filterRetentionActive;
    std::unordered_set<void*> previousKept;
    std::vector<winrt::IInspectable> previousKeepAlive;
    if (computeRetention)
    {
        previousKept = std::move(m_filterKept);
        previousKeepAlive = std::move(m_filterKeepAlive);
        m_filterKept.clear();
        m_filterContext.clear();
        m_filterKeepAlive.clear();
        m_filterRetentionActive = true;
    }
    auto invalidateShaped = wil::scope_exit([&structure]() noexcept { structure.InvalidateShapedChildren(); });
    if (!computeRetention)
    {
        invalidateShaped.release();
    }

    // Phase 1: one post-order walk over the UNSHAPED tree through the cache (which validates and
    // subscribes every collection on the way). Each object gets one node slot, children first.
    auto index = std::make_shared<HierarchyIndex>();
    bool indexable = true;

    struct Visit
    {
        bool Kept;
        // Levels from this node to its deepest descendant, itself included.
        int32_t Height;
        uint32_t Slot;
    };
    struct Frame
    {
        winrt::IInspectable Item;
        HierarchyStructureCache::Children Children;
        size_t Next;
        int32_t Depth;
        int32_t Height;
        bool Match;
        bool DescendantKept;
        bool OnPath;
    };
    std::unordered_map<void*, Visit> memo;
    std::unordered_set<void*> onPath;
    std::vector<Frame> stack;

    auto deliver = [&stack](bool kept, int32_t height)
    {
        if (!stack.empty())
        {
            auto& parent = stack.back();
            parent.DescendantKept = parent.DescendantKept || kept;
            parent.Height = std::max(parent.Height, height);
        }
    };
    auto open = [&](winrt::IInspectable const& item, int32_t depth)
    {
        void* const abi = winrt::get_abi(item);
        if (auto it = memo.find(abi); it != memo.end())
        {
            // Reached again through another parent (a DAG): emitted under this path too, so its
            // chain must also fit from here.
            if (depth + it->second.Height > c_maxDepth)
            {
                indexable = false;
            }
            deliver(it->second.Kept, it->second.Height);
            return;
        }

        Frame frame{ item, nullptr, 0, depth, 0, !computeRetention || m_pipeline.PassesFilter(item), false, false };
        if (depth < c_maxDepth && onPath.insert(abi).second)
        {
            frame.OnPath = true;
            frame.Children = structure.ChildrenOf(item, m_childrenSelector);
        }
        else
        {
            // A cycle or an over-deep chain. The walk stops descending, as retention always has;
            // the index is abandoned so the adapter's own walk reports it once it becomes visible.
            indexable = false;
        }
        stack.push_back(std::move(frame));
    };

    for (auto const& root : roots)
    {
        if (!root)
        {
            continue;
        }

        open(root, 0);
        while (!stack.empty())
        {
            {
                auto& top = stack.back();
                if (top.Children && top.Next < top.Children->size())
                {
                    auto const child = (*top.Children)[top.Next++];
                    const auto childDepth = top.Depth + 1;
                    if (child)
                    {
                        open(child, childDepth);
                    }
                    continue;
                }
            }

            auto frame = std::move(stack.back());
            stack.pop_back();
            void* const abi = winrt::get_abi(frame.Item);
            if (frame.OnPath)
            {
                onPath.erase(abi);
            }

            const bool kept = frame.Match || frame.DescendantKept;
            const int32_t height = frame.Height + 1;
            if (frame.Depth + height > c_maxDepth)
            {
                indexable = false;
            }

            // Under a cycle the inner (cut-off) visit is memoized first and wins, as in
            // ComputeFilterRetention; the kept set still records every visit.
            auto [it, inserted] = memo.try_emplace(abi, Visit{ kept, height, HierarchyIndex::npos });
            if (inserted && indexable)
            {
                const auto slot = static_cast<uint32_t>(index->Nodes.size());
                it->second.Slot = slot;
                index->ByItem.emplace(abi, slot);
                auto& node = index->Nodes.emplace_back();
                node.Item = frame.Item;
                node.Height = height;
                node.Passes = frame.Match;
                node.Retention = !kept ? HierarchyIndex::State::Out : frame.Match ? HierarchyIndex::State::Match : HierarchyIndex::State::Context;

                // Every child was visited (and, the walk being indexable so far, given a slot) first.
                if (frame.Children)
                {
                    for (auto const& child : *frame.Children)
                    {
                        if (!child)
                        {
                            continue;
                        }
                        auto const found = memo.find(winrt::get_abi(child));
                        if (found != memo.end() && found->second.Slot != HierarchyIndex::npos)
                        {
                            node.Raw.push_back(found->second.Slot);
                        }
                    }
                    std::sort(node.Raw.begin(), node.Raw.end());
                    node.Raw.erase(std::unique(node.Raw.begin(), node.Raw.end()), node.Raw.end());
                    for (auto const child : node.Raw)
                    {
                        index->Nodes[child].Parents.push_back(slot);
                    }
                }
            }

            if (computeRetention)
            {
                m_filterKeepAlive.push_back(frame.Item);
                if (kept)
                {
                    m_filterKept.insert(abi);
                    if (!frame.Match)
                    {
                        m_filterContext.insert(abi);
                    }
                }
            }

            deliver(kept, height);
        }

        if (!indexable && !computeRetention)
        {
            // Nothing else this walk is needed for.
            return nullptr;
        }
    }

    // Shaped lists filtered by the previous kept set are dropped unless the new set is identical.
    // Before shaping below, which must not reuse them.
    if (computeRetention && wasActive && previousKept == m_filterKept)
    {
        invalidateShaped.release();
    }
    invalidateShaped.reset();

    if (!indexable || m_structureChangedDuringBuild)
    {
        return nullptr;
    }

    for (auto const& root : roots)
    {
        if (root)
        {
            index->Roots.push_back(memo.find(winrt::get_abi(root))->second.Slot);
        }
    }

    // Phase 2: shape every sibling set that can be shown, through the cache's shaped lists, with
    // the very callback the adapter's walk uses -- so the two share every list either produces.
    const ShapingHelpers::ShapeSiblingsFn shape = [this](std::vector<winrt::IInspectable>& siblings) { ShapeHierarchySiblings(siblings); };
    std::vector<uint32_t> seenUnder(index->Nodes.size(), HierarchyIndex::npos);
    try
    {
        for (uint32_t slot = 0; slot < static_cast<uint32_t>(index->Nodes.size()); ++slot)
        {
            if (computeRetention && !IsFilterKept(index->Nodes[slot].Item))
            {
                // Never shown, so never shaped or trusted.
                continue;
            }
            ShapeHierarchyIndexNode(*index, slot, computeRetention, shape, seenUnder);
        }
    }
    catch (...)
    {
        // A sort or filter that throws is reported by the walk, for the set it is shaping, only if
        // that set is ever shown.
        return nullptr;
    }

    if (m_structureChangedDuringBuild)
    {
        return nullptr;
    }

    index->Cache = &structure;
    index->CacheVersion = structure.Version();
    index->Filtered = computeRetention;
    index->FilterGeneration = m_filterGeneration;
    // Everything logged so far is reflected in what was just built, including the lists this
    // build's own reads replaced.
    structure.TakeChangeLog();
    return index;
}

void ShapedItemsSource::ShapeHierarchyIndexNode(
    HierarchyIndex& index,
    uint32_t slot,
    bool computeRetention,
    ShapingHelpers::ShapeSiblingsFn const& shape,
    std::vector<uint32_t>& seenUnder)
{
    auto& structure = *m_structureCache;
    auto const item = index.Nodes[slot].Item;

    winrt::hstring identity = structure.Identity(item);
    bool trusted = structure.IsTrusted(item);
    int32_t childCount = 0;
    std::vector<uint32_t> children;
    if (!structure.ChildrenOf(item, m_childrenSelector)->empty())
    {
        auto const shaped = structure.ShapedChildrenOf(item, m_childrenSelector, shape);
        childCount = static_cast<int32_t>(shaped->size());
        children.reserve(shaped->size());
        for (auto const& child : *shaped)
        {
            if (!child)
            {
                continue;
            }

            auto const found = index.ByItem.find(winrt::get_abi(child));
            if (found == index.ByItem.end())
            {
                trusted = false;
                continue;
            }

            // The same object twice in one shaped set: the walk rejects that once the set is
            // visible, so leave it to the walk.
            const auto childSlot = found->second;
            if (childSlot >= seenUnder.size())
            {
                seenUnder.resize(index.Nodes.size(), HierarchyIndex::npos);
            }
            if (seenUnder[childSlot] == slot)
            {
                trusted = false;
            }
            seenUnder[childSlot] = slot;
            children.push_back(childSlot);
        }
    }

    // Found again and written last: everything above may call app code.
    auto& node = index.Nodes[slot];
    node.Identity = std::move(identity);
    node.IsContext = computeRetention && IsFilterContext(item);
    node.ChildCount = childCount;
    node.Children = std::move(children);
    node.Trusted = trusted;
}

bool ShapedItemsSource::IndexHierarchySubtree(
    HierarchyIndex& index,
    winrt::IInspectable const& item,
    std::vector<uint32_t>& added,
    std::vector<HierarchyStructureCache::Children>& addedChildren)
{
    constexpr int32_t c_maxDepth = HierarchicalSourceAdapter::c_maxHierarchyDepth;
    auto& structure = *m_structureCache;

    // Post-order, like the build's phase 1, but only over objects the index does not hold yet: a
    // node it already holds is current (the change log names every one whose list moved), so its
    // subtree is referenced, not walked. A cycle or an over-deep chain within the new part fails
    // the update; one through existing nodes is caught by the caller's height check.
    struct Frame
    {
        winrt::IInspectable Item;
        HierarchyStructureCache::Children Children;
        size_t Next;
    };
    std::vector<Frame> stack;
    std::unordered_set<void*> onPath;

    auto open = [&](winrt::IInspectable const& next) -> bool
    {
        void* const abi = winrt::get_abi(next);
        if (index.ByItem.find(abi) != index.ByItem.end())
        {
            return true;
        }
        if (static_cast<int32_t>(stack.size()) >= c_maxDepth || !onPath.insert(abi).second)
        {
            return false;
        }
        auto children = structure.ChildrenOf(next, m_childrenSelector);
        stack.push_back(Frame{ next, std::move(children), 0 });
        return true;
    };

    if (!item)
    {
        return true;
    }
    if (!open(item))
    {
        return false;
    }

    while (!stack.empty())
    {
        winrt::IInspectable child{ nullptr };
        {
            auto& top = stack.back();
            if (top.Next < top.Children->size())
            {
                child = (*top.Children)[top.Next++];
                if (child && !open(child))
                {
                    return false;
                }
                continue;
            }
        }

        auto frame = std::move(stack.back());
        stack.pop_back();
        void* const abi = winrt::get_abi(frame.Item);
        onPath.erase(abi);

        const auto slot = static_cast<uint32_t>(index.Nodes.size());
        if (index.ByItem.emplace(abi, slot).second)
        {
            index.Nodes.emplace_back().Item = std::move(frame.Item);
            added.push_back(slot);
            addedChildren.push_back(std::move(frame.Children));
        }
    }
    return true;
}

bool ShapedItemsSource::IsHierarchyIndexWalkable(HierarchyIndex& index)
{
    constexpr int32_t c_maxDepth = HierarchicalSourceAdapter::c_maxHierarchyDepth;

    // Iterative DFS over the unshaped edges from every live node: 1 = on the current path, 2 = done
    // with its height known. Fails on a cycle or on any chain longer than the walk supports (the
    // bound the build enforces from every root), and otherwise leaves every live Height exact.
    auto& nodes = index.Nodes;
    std::vector<uint8_t> state(nodes.size(), 0);
    struct Frame
    {
        uint32_t Slot;
        size_t Next;
        int32_t Height;
    };
    std::vector<Frame> stack;

    for (uint32_t start = 0; start < static_cast<uint32_t>(nodes.size()); ++start)
    {
        if (state[start] != 0 || nodes[start].Dead)
        {
            continue;
        }

        state[start] = 1;
        stack.push_back(Frame{ start, 0, 1 });
        while (!stack.empty())
        {
            auto& top = stack.back();
            auto const& raw = nodes[top.Slot].Raw;
            if (top.Next < raw.size())
            {
                const auto child = raw[top.Next++];
                if (state[child] == 1)
                {
                    return false;
                }
                if (state[child] == 2)
                {
                    top.Height = std::max(top.Height, nodes[child].Height + 1);
                    continue;
                }
                if (static_cast<int32_t>(stack.size()) >= c_maxDepth)
                {
                    return false;
                }
                state[child] = 1;
                stack.push_back(Frame{ child, 0, 1 });
                continue;
            }

            const auto done = top;
            stack.pop_back();
            state[done.Slot] = 2;
            nodes[done.Slot].Height = done.Height;
            if (!stack.empty())
            {
                stack.back().Height = std::max(stack.back().Height, done.Height + 1);
            }
        }
    }
    return true;
}

bool ShapedItemsSource::UpdateHierarchyIndex(
    std::vector<winrt::IInspectable> const& roots,
    bool retention,
    bool reevaluateAll,
    std::vector<winrt::IInspectable>* changed)
{
    if (changed)
    {
        changed->clear();
    }
    auto& structure = EnsureStructureCache();
    auto const log = structure.TakeChangeLog();
    if (m_hierarchyIndex && log.Empty() &&
        m_hierarchyIndex->Cache == &structure &&
        m_hierarchyIndex->CacheVersion == structure.Version() &&
        m_hierarchyIndex->Filtered == retention &&
        m_filterRetentionActive == retention &&
        (!retention || (!reevaluateAll && m_hierarchyIndex->FilterGeneration == m_filterGeneration)))
    {
        // Nothing moved since the index was last current.
        return true;
    }

    bool rootsKeptChanged = false;
    if (!TryUpdateHierarchyIndex(roots, log, retention, reevaluateAll, rootsKeptChanged, changed))
    {
        if (changed)
        {
            changed->clear();
        }
        // The build derives retention afresh (dropping every list shaped by a kept set that turns
        // out to differ); without a filter there is no retention left to keep.
        if (!retention)
        {
            ClearFilterRetention();
        }
        m_hierarchyIndex = BuildHierarchyIndex(roots, retention);
        return false;
    }
    return true;
}

bool ShapedItemsSource::TryUpdateHierarchyIndex(
    std::vector<winrt::IInspectable> const& roots,
    HierarchyStructureChangeLog const& log,
    bool retention,
    bool reevaluateAll,
    bool& rootsKeptChanged,
    std::vector<winrt::IInspectable>* changed)
{
    using State = HierarchyIndex::State;
    constexpr int32_t c_maxDepth = HierarchicalSourceAdapter::c_maxHierarchyDepth;
    auto& structure = EnsureStructureCache();
    auto const index = m_hierarchyIndex;
    rootsKeptChanged = false;

    // In place, so only an index nobody is reading: the owner's reference, the adapter's, and the
    // local above. A walk pins it for its duration, and a re-entrant update finds the build flag.
    if (!index || log.Full || index->Cache != &structure || index->Filtered != m_filterRetentionActive ||
        m_buildingHierarchyIndex || index.use_count() > 3)
    {
        return false;
    }

    m_buildingHierarchyIndex = true;
    m_structureChangedDuringBuild = false;
    auto buildingGuard = wil::scope_exit([this]() noexcept { m_buildingHierarchyIndex = false; });

    // Not current while it is being mutated, whatever happens below; re-stamped only on success.
    // On failure the caller replaces it.
    index->Cache = nullptr;

    // Once the kept sets move, the cached shaped lists they filtered are right only where this
    // update re-shapes them; should it then fail, the build that replaces it must trust none.
    bool retentionMoved = false;
    auto dropShapes = wil::scope_exit([&]() noexcept
    {
        if (retentionMoved)
        {
            structure.InvalidateShapedChildren();
        }
    });

    // Slots are stable; element references are not held across indexing, which grows the vector.
    auto& nodes = index->Nodes;
    const bool transition = retention != index->Filtered;
    bool edgesChanged = false;

    auto unlink = [](std::vector<uint32_t>& from, uint32_t slot)
    {
        auto const it = std::find(from.begin(), from.end(), slot);
        if (it != from.end())
        {
            from.erase(it);
        }
    };

    // Released nodes first: an item the sweep dropped and the app has added back since is a new
    // node of the cache, and must be indexed afresh rather than found here. A released node is
    // unreachable, so every parent it still has is released too or has a list that moved.
    std::vector<uint32_t> released;
    std::vector<uint32_t> touched;
    for (auto const key : log.Released)
    {
        auto const found = index->ByItem.find(key);
        if (found == index->ByItem.end())
        {
            continue;
        }

        const auto slot = found->second;
        index->ByItem.erase(found);
        ++index->DeadSlots;
        released.push_back(slot);

        auto& node = nodes[slot];
        for (auto const child : node.Raw)
        {
            unlink(nodes[child].Parents, slot);
        }
        for (auto const parent : node.Parents)
        {
            unlink(nodes[parent].Raw, slot);
            touched.push_back(parent);
        }
        node.Raw.clear();
        node.Parents.clear();
        node.Children.clear();
        node.ChildCount = 0;
        node.Trusted = false;
        node.IsContext = false;
        node.Dead = true;
        edgesChanged = true;
    }

    if (static_cast<size_t>(index->DeadSlots) * 4 > nodes.size())
    {
        // Compacting is the build's job.
        return false;
    }

    // Index what is new: roots the index has never seen (the root set is the materialized rows),
    // and whatever is new beneath each list that moved.
    std::vector<uint32_t> added;
    std::vector<HierarchyStructureCache::Children> addedChildren;
    std::vector<std::pair<uint32_t, HierarchyStructureCache::Children>> dirty;
    try
    {
        for (auto const& root : roots)
        {
            if (root && index->ByItem.find(winrt::get_abi(root)) == index->ByItem.end() &&
                !IndexHierarchySubtree(*index, root, added, addedChildren))
            {
                return false;
            }
        }

        for (auto const key : log.DirtyLists)
        {
            auto const found = index->ByItem.find(key);
            if (found == index->ByItem.end())
            {
                // Not indexed: unreachable, or new and indexed through its parent.
                continue;
            }

            const auto slot = found->second;
            auto const item = nodes[slot].Item;
            auto children = structure.ChildrenOf(item, m_childrenSelector);
            for (auto const& child : *children)
            {
                if (child && index->ByItem.find(winrt::get_abi(child)) == index->ByItem.end() &&
                    !IndexHierarchySubtree(*index, child, added, addedChildren))
                {
                    return false;
                }
            }
            dirty.emplace_back(slot, std::move(children));
        }
    }
    catch (...)
    {
        // As in the build: a throwing selector is left to the walk to report.
        return false;
    }

    if (m_structureChangedDuringBuild)
    {
        return false;
    }

    std::vector<uint32_t> rootSlots;
    rootSlots.reserve(roots.size());
    for (auto const& root : roots)
    {
        if (root)
        {
            rootSlots.push_back(index->ByItem.find(winrt::get_abi(root))->second);
        }
    }

    // Re-links a node's unshaped children (sorted, unique) and their parent links to what the
    // cache reported, collecting the edges that are new when asked to.
    std::vector<std::pair<uint32_t, uint32_t>> newEdges;
    std::vector<uint32_t> raw;
    auto relink = [&](uint32_t slot, HierarchyStructureCache::Children const& children, bool collect)
    {
        raw.clear();
        if (children)
        {
            for (auto const& child : *children)
            {
                if (!child)
                {
                    continue;
                }
                auto const found = index->ByItem.find(winrt::get_abi(child));
                if (found != index->ByItem.end())
                {
                    raw.push_back(found->second);
                }
            }
        }
        std::sort(raw.begin(), raw.end());
        raw.erase(std::unique(raw.begin(), raw.end()), raw.end());

        auto& previous = nodes[slot].Raw;
        if (raw == previous)
        {
            return;
        }
        edgesChanged = true;

        size_t i = 0;
        size_t j = 0;
        while (i < previous.size() || j < raw.size())
        {
            if (j == raw.size() || (i < previous.size() && previous[i] < raw[j]))
            {
                unlink(nodes[previous[i++]].Parents, slot);
            }
            else if (i == previous.size() || raw[j] < previous[i])
            {
                const auto child = raw[j++];
                nodes[child].Parents.push_back(slot);
                if (collect)
                {
                    newEdges.emplace_back(slot, child);
                }
            }
            else
            {
                ++i;
                ++j;
            }
        }
        previous = raw;
    };

    // New nodes are linked, and their heights derived, children first (the order they were indexed
    // in). Then the lists that moved, whose new edges may close a cycle or lengthen a chain.
    for (size_t i = 0; i < added.size(); ++i)
    {
        nodes[added[i]].Retention = retention ? State::Out : State::Match;
        relink(added[i], addedChildren[i], false);
    }
    for (auto const slot : added)
    {
        int32_t height = 1;
        for (auto const child : nodes[slot].Raw)
        {
            height = std::max(height, nodes[child].Height + 1);
        }
        nodes[slot].Height = height;
        if (height > c_maxDepth)
        {
            return false;
        }
    }
    for (auto const& [slot, children] : dirty)
    {
        relink(slot, children, true);
    }

    // Every edge P -> C satisfies Height(P) > Height(C) (heights never fall below the truth), so a
    // new edge only has to push heights up P's ancestors while they rise. A new edge that closes
    // a cycle raises them around it without end, so a cycle surfaces as a chain over the limit --
    // the same verdict a chain that is really over it gets. Bounded: past the budget the whole
    // index is checked (and its heights re-derived) instead.
    if (!newEdges.empty())
    {
        size_t budget = std::max<size_t>(4096, nodes.size());
        std::vector<uint32_t> raised;
        auto raise = [&](uint32_t parent, uint32_t child) -> bool
        {
            const int32_t height = nodes[child].Height + 1;
            if (height <= nodes[parent].Height)
            {
                return true;
            }
            nodes[parent].Height = height;
            raised.push_back(parent);
            return height <= c_maxDepth;
        };

        for (auto const& [parent, child] : newEdges)
        {
            if (parent == child || !raise(parent, child))
            {
                return false;
            }
        }

        bool exhausted = false;
        while (!raised.empty())
        {
            if (budget-- == 0)
            {
                exhausted = true;
                break;
            }
            const auto slot = raised.back();
            raised.pop_back();
            for (auto const parent : nodes[slot].Parents)
            {
                if (!raise(parent, slot))
                {
                    return false;
                }
            }
        }

        if (exhausted && !IsHierarchyIndexWalkable(*index))
        {
            return false;
        }
    }

    // Filter verdicts: every node's when the filter itself may have changed, else only those of
    // new nodes and of nodes whose item reported a change. App code, so on copies.
    bool verdictMoved = false;
    if (retention)
    {
        const bool all = reevaluateAll || transition || index->FilterGeneration != m_filterGeneration;
        auto evaluate = [&](uint32_t slot)
        {
            auto const item = nodes[slot].Item;
            const bool passes = m_pipeline.PassesFilter(item);
            if (nodes[slot].Passes != passes)
            {
                nodes[slot].Passes = passes;
                verdictMoved = true;
            }
        };

        try
        {
            if (all)
            {
                for (uint32_t slot = 0; slot < static_cast<uint32_t>(nodes.size()); ++slot)
                {
                    if (!nodes[slot].Dead)
                    {
                        evaluate(slot);
                    }
                }
            }
            else
            {
                for (auto const slot : added)
                {
                    evaluate(slot);
                }
                for (auto const key : log.DirtyNodes)
                {
                    if (auto const found = index->ByItem.find(key); found != index->ByItem.end())
                    {
                        evaluate(found->second);
                    }
                }
            }
        }
        catch (...)
        {
            // The build calls it again, and lets it throw where it always has.
            return false;
        }

        if (m_structureChangedDuringBuild)
        {
            return false;
        }
    }

    const auto count = static_cast<uint32_t>(nodes.size());
    const bool rootsMoved = rootSlots != index->Roots;
    if (rootsMoved)
    {
        rootsKeptChanged = true;
    }
    std::vector<uint8_t> isRoot(count, 0);
    for (auto const slot : rootSlots)
    {
        isRoot[slot] = 1;
    }

    std::vector<uint32_t> toShape;
    try
    {
        // Retention is re-derived only when something it depends on moved, in one flat pass over
        // the index: a node reached from a root is a match when it passes, and context when it does
        // not but some match lies below it -- following every parent, since a node can have more
        // than one. The same rule the build's walk applies, without walking the cache.
        const bool restate = transition ||
            (retention && (verdictMoved || edgesChanged || rootsMoved || !added.empty() || !released.empty()));
        if (restate)
        {
            std::vector<State> next(count, State::Out);
            if (retention)
            {
                std::vector<uint8_t> reached(count, 0);
                std::vector<uint32_t> stack;
                for (auto const slot : rootSlots)
                {
                    if (!reached[slot])
                    {
                        reached[slot] = 1;
                        stack.push_back(slot);
                    }
                }
                while (!stack.empty())
                {
                    const auto slot = stack.back();
                    stack.pop_back();
                    for (auto const child : nodes[slot].Raw)
                    {
                        if (!reached[child])
                        {
                            reached[child] = 1;
                            stack.push_back(child);
                        }
                    }
                }

                for (uint32_t slot = 0; slot < count; ++slot)
                {
                    if (reached[slot] && nodes[slot].Passes)
                    {
                        next[slot] = State::Match;
                        stack.push_back(slot);
                    }
                }
                while (!stack.empty())
                {
                    const auto slot = stack.back();
                    stack.pop_back();
                    for (auto const parent : nodes[slot].Parents)
                    {
                        if (reached[parent] && next[parent] == State::Out)
                        {
                            next[parent] = State::Context;
                            stack.push_back(parent);
                        }
                    }
                }
            }
            else
            {
                for (uint32_t slot = 0; slot < count; ++slot)
                {
                    if (!nodes[slot].Dead)
                    {
                        next[slot] = State::Match;
                    }
                }
            }

            // The kept sets follow the index exactly, as if the build had derived them.
            retentionMoved = true;
            if (transition)
            {
                m_filterKept.clear();
                m_filterContext.clear();
                m_filterKeepAlive.clear();
            }
            else
            {
                for (auto const slot : released)
                {
                    void* const key = winrt::get_abi(nodes[slot].Item);
                    m_filterKept.erase(key);
                    m_filterContext.erase(key);
                }
                for (auto const slot : added)
                {
                    m_filterKeepAlive.push_back(nodes[slot].Item);
                }
            }
            m_filterRetentionActive = retention;

            // A node entering or leaving the kept set moves the membership of every list it is in
            // (only those are re-shaped), and a node entering it is shaped itself.
            for (uint32_t slot = 0; slot < count; ++slot)
            {
                auto& node = nodes[slot];
                if (node.Dead)
                {
                    continue;
                }

                const auto previous = node.Retention;
                const auto now = next[slot];
                const bool kept = now != State::Out;
                if (retention)
                {
                    void* const key = winrt::get_abi(node.Item);
                    if (transition)
                    {
                        m_filterKeepAlive.push_back(node.Item);
                    }
                    if (transition || previous != now)
                    {
                        if (kept)
                        {
                            m_filterKept.insert(key);
                        }
                        else
                        {
                            m_filterKept.erase(key);
                        }
                        if (now == State::Context)
                        {
                            m_filterContext.insert(key);
                        }
                        else
                        {
                            m_filterContext.erase(key);
                        }
                    }
                }

                if (previous == now)
                {
                    continue;
                }
                node.Retention = now;
                node.IsContext = now == State::Context;
                if ((previous != State::Out) == kept)
                {
                    // Kept either way, but shown as a match or as context: its own row moves.
                    toShape.push_back(slot);
                    continue;
                }

                if (isRoot[slot])
                {
                    rootsKeptChanged = true;
                }
                for (auto const parent : node.Parents)
                {
                    structure.InvalidateShapeOf(nodes[parent].Item);
                    toShape.push_back(parent);
                }
                if (kept)
                {
                    structure.InvalidateShapeOf(node.Item);
                    toShape.push_back(slot);
                }
                else
                {
                    // Never shown, so never shaped or trusted, as in the build.
                    node.Children.clear();
                    node.ChildCount = 0;
                    node.Trusted = false;
                }
            }
        }
        for (auto const slot : released)
        {
            nodes[slot].Retention = State::Out;
        }

        // Re-shape the lists that moved, the new nodes, and every list whose membership moved --
        // each once, and only where it can be shown.
        for (auto const& entry : dirty)
        {
            toShape.push_back(entry.first);
        }
        toShape.insert(toShape.end(), added.begin(), added.end());
        toShape.insert(toShape.end(), touched.begin(), touched.end());
        std::sort(toShape.begin(), toShape.end());
        toShape.erase(std::unique(toShape.begin(), toShape.end()), toShape.end());

        const ShapingHelpers::ShapeSiblingsFn shape = [this](std::vector<winrt::IInspectable>& siblings) { ShapeHierarchySiblings(siblings); };
        std::vector<uint32_t> seenUnder(count, HierarchyIndex::npos);
        for (auto const slot : toShape)
        {
            if (nodes[slot].Dead || (retention && nodes[slot].Retention == State::Out))
            {
                continue;
            }
            ShapeHierarchyIndexNode(*index, slot, retention, shape, seenUnder);
        }
    }
    catch (...)
    {
        // As in the build: a throwing sort or filter is left to the walk to report.
        return false;
    }

    if (m_structureChangedDuringBuild)
    {
        return false;
    }

    index->Filtered = retention;
    index->FilterGeneration = m_filterGeneration;
    index->Roots = std::move(rootSlots);
    index->Cache = &structure;
    index->CacheVersion = structure.Version();
    // As after a build: what was logged during the update is this update's own re-reads and
    // invalidations.
    structure.TakeChangeLog();
    dropShapes.release();

    if (changed)
    {
        changed->clear();
        changed->reserve(toShape.size());
        for (auto const slot : toShape)
        {
            if (!nodes[slot].Dead)
            {
                changed->push_back(nodes[slot].Item);
            }
        }
    }
    return true;
}

HierarchyStructureCache& ShapedItemsSource::EnsureStructureCache()
{
    if (!m_structureCache)
    {
        m_structureCache = std::make_shared<HierarchyStructureCache>();
        std::weak_ptr<ShapedItemsSource> weakThis = weak_from_this();
        m_structureCache->ChangedHandler(
            [weakThis]()
            {
                if (auto strongThis = weakThis.lock()) { strongThis->OnStructureChanged(); }
            });
        m_structureCache->ItemChangedHandler(
            [weakThis](winrt::IInspectable const& item, winrt::hstring const& propertyName)
            {
                if (auto strongThis = weakThis.lock()) { strongThis->OnHierarchyItemChanged(item, propertyName); }
            });
        m_structureCache->TrackItemChanges(m_liveShapingEnabled);
    }
    return *m_structureCache;
}

void ShapedItemsSource::ResetStructureCache()
{
    // Reset rather than dropped: a pass in flight holds the cache, and the adapter may still hold
    // it until the next declaration re-injects it.
    if (m_structureCache)
    {
        m_structureCache->Reset();
    }
    m_hierarchyIndex = nullptr;
}

void ShapedItemsSource::OnStructureChanged()
{
    if (m_buildingHierarchyIndex)
    {
        // The build may already have read the node that changed; it discards what it built.
        m_structureChangedDuringBuild = true;
        if (!m_isRefreshing)
        {
            // The build or update in flight is this method's own (below): it fails on the flag, and
            // its caller here takes the path that re-reads everything.
            return;
        }
    }

    // Coalesced: a burst of collection changes (a bulk add, a move as remove+insert, a handler
    // that edits several lists) accumulates in the cache's change log and is applied once, by a
    // single update posted ahead of the next layout. Reads that must observe it first flush it
    // (FlushPendingStructureChanges). Inside a refresh, or without a queue, it applies now as
    // before.
    if (m_hierarchicalAdapter && m_ownerQueue && !m_isRefreshing && !m_buildingHierarchyIndex)
    {
        m_structureFlushPending = true;
        ScheduleStructureFlush();
        if (m_structureFlushPosted || m_inStructureFlush)
        {
            // In a flush, one the post failed for is picked up by the flush's own loop.
            return;
        }
        // Not posted (the queue is shutting down): apply now.
        m_structureFlushPending = false;
    }

    ApplyStructureChange();
}

void ShapedItemsSource::ScheduleStructureFlush()
{
    if (m_structureFlushPosted)
    {
        return;
    }

    std::weak_ptr<ShapedItemsSource> weakThis = weak_from_this();
    try
    {
        m_structureFlushPosted = m_ownerQueue.TryEnqueue(
            winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::High,
            [weakThis]()
            {
                if (auto strongThis = weakThis.lock())
                {
                    strongThis->m_structureFlushPosted = false;
                    strongThis->FlushPendingStructureChanges();
                }
            });
    }
    catch (...)
    {
        m_structureFlushPosted = false;
    }
}

void ShapedItemsSource::FlushPendingStructureChanges()
{
    if (!m_structureFlushPending || m_inStructureFlush || m_isRefreshing || m_buildingHierarchyIndex)
    {
        return;
    }

    m_inStructureFlush = true;
    auto flushGuard = wil::scope_exit([this]() noexcept { m_inStructureFlush = false; });

    // A change raised while this applies (an app handler reacting to the projection's own
    // notifications) is posted as the next update rather than nested in this one; applied here
    // only when it could not be posted.
    do
    {
        m_structureFlushPending = false;
        ApplyStructureChange();
    } while (m_structureFlushPending && !m_structureFlushPosted);
}

void ShapedItemsSource::ApplyStructureChange()
{
    // Eager: bring the index current first -- only the lists that moved are re-read and re-shaped,
    // and under a filter only the verdicts that may have moved are re-read and only the lists whose
    // kept membership moved are re-shaped -- so the adapter reads the new structure, and patches
    // only the visible runs under the nodes the update reports.
    bool refresh = m_filterRetentionActive || !m_hierarchicalAdapter;
    if (IsEagerHierarchy() && m_hierarchicalAdapter && m_structureCache && !m_isRefreshing && !m_buildingHierarchyIndex &&
        (!m_filterRetentionActive || m_hierarchyIndex))
    {
        HierarchyStructureCache::Pass pass{ m_structureCache };
        auto const roots = m_structureCache->RootItems();
        std::vector<winrt::IInspectable> changed;
        bool incremental = false;
        if (!m_filterRetentionActive)
        {
            incremental = UpdateHierarchyIndex(roots, false /* retention */, false /* reevaluateAll */, &changed);
            refresh = false;
        }
        else
        {
            // Without live shaping nothing reports which items changed, so every verdict is re-read
            // (still without walking the cache). A root entering or leaving the kept set re-shapes
            // the root set, which only Refresh does.
            auto const log = m_structureCache->TakeChangeLog();
            bool rootsKeptChanged = false;
            if (TryUpdateHierarchyIndex(roots, log, true /* retention */, !m_liveShapingEnabled, rootsKeptChanged, &changed))
            {
                refresh = rootsKeptChanged;
                incremental = true;
            }
            else
            {
                m_hierarchyIndex = nullptr;
            }
        }

        if (!refresh)
        {
            m_hierarchicalAdapter->SetHierarchyIndex(m_hierarchyIndex);
            if (incremental)
            {
                if (m_hierarchicalAdapter->OnIndexUpdated(changed))
                {
                    if (auto const hook = m_rowsMetadataChangedHook)
                    {
                        hook();
                    }
                }
            }
            else
            {
                m_hierarchicalAdapter->OnStructureChanged();
            }
            return;
        }
    }

    // Otherwise retention is re-derived by Refresh, which rebuilds the adapter too. Without
    // retention only the visible rows can change, and the adapter alone re-walks them (its
    // subscriptions used to call it directly).
    if (refresh)
    {
        Refresh();
        return;
    }
    m_hierarchicalAdapter->OnStructureChanged();
}

bool ShapedItemsSource::IsShapeAffectedBy(winrt::hstring const& propertyName, bool isRoot) const
{
    // Predicates and group keys are opaque delegates, so any property may feed them.
    if (m_pipeline.HasFilter() || (isRoot && m_groupSelector))
    {
        return true;
    }

    const std::wstring_view name{ propertyName };
    for (auto const& axis : m_pipeline.ActiveSortAxes(-1, -1))
    {
        const std::wstring_view path{ axis.SortMemberPath };
        if (name.empty() || path.empty())
        {
            return true;
        }
        if (path.size() >= name.size() && path.compare(0, name.size(), name) == 0 &&
            (path.size() == name.size() || path[name.size()] == L'.'))
        {
            return true;
        }
    }
    return false;
}

void ShapedItemsSource::OnHierarchyItemChanged(winrt::IInspectable const& item, winrt::hstring const& propertyName)
{
    if (!m_liveShapingEnabled || !m_childrenSelector || !m_structureCache)
    {
        return;
    }

    if (m_buildingHierarchyIndex)
    {
        // The index being built or updated may already have shaped a list this change moves.
        m_structureChangedDuringBuild = true;
    }

    const bool isRoot = m_structureCache->IsRoot(item);
    if (IsShapeAffectedBy(propertyName, isRoot))
    {
        // The root set is derived by Refresh, and so is filter retention -- except in an eager
        // hierarchy with an index, whose update re-reads the verdict of exactly the items the cache
        // logged as changed. A deeper list only needs re-shaping, which the adapter's next walk does
        // once the list is invalidated.
        const bool incrementalRetention = m_filterRetentionActive && IsEagerHierarchy() && m_hierarchyIndex;
        if (isRoot || (m_filterRetentionActive && !incrementalRetention))
        {
            m_liveShapingNeedsRefresh = true;
        }
        if (m_structureCache->InvalidateShapeOfParents(item) || incrementalRetention)
        {
            m_liveShapingNeedsRebuild = true;
        }
    }

    // Always scheduled: the cache has also marked the node for selector revalidation.
    ScheduleLiveShapingFlush();
}

void ShapedItemsSource::ScheduleLiveShapingFlush()
{
    if (m_liveShapingFlushPosted)
    {
        return;
    }

    // One flush per burst of property changes: an edit that raises several notifications, or a
    // bulk update touching many items, re-projects once.
    bool queued = false;
    if (m_ownerQueue)
    {
        m_liveShapingFlushPosted = true;
        std::weak_ptr<ShapedItemsSource> weakThis = weak_from_this();
        try
        {
            queued = m_ownerQueue.TryEnqueue(
                [weakThis]()
                {
                    if (auto strongThis = weakThis.lock()) { strongThis->FlushLiveShaping(); }
                });
        }
        catch (...)
        {
            queued = false;
        }
    }

    if (!queued)
    {
        // No queue to defer to (shutting down, or no dispatcher on this thread): apply now.
        m_liveShapingFlushPosted = false;
        FlushLiveShaping();
    }
}

void ShapedItemsSource::FlushLiveShaping()
{
    m_liveShapingFlushPosted = false;
    const bool needsRefresh = std::exchange(m_liveShapingNeedsRefresh, false);
    bool needsRebuild = std::exchange(m_liveShapingNeedsRebuild, false);

    if (!m_liveShapingEnabled || !m_childrenSelector || !m_structureCache)
    {
        return;
    }

    // A changed item may now hand back a different children collection (its Children property was
    // replaced): that is a structure change, exactly like the old collection raising one.
    if (m_structureCache->RevalidateItemChanges(m_childrenSelector))
    {
        needsRebuild = true;
    }

    if (needsRefresh)
    {
        Refresh();
        return;
    }

    if (needsRebuild)
    {
        // Already deferred: applied now, together with any collection change still pending.
        m_structureFlushPending = true;
        FlushPendingStructureChanges();
        if (m_structureFlushPending && !m_structureFlushPosted && !m_inStructureFlush)
        {
            // Refused (a refresh or an index update is running): as before, through the change path.
            m_structureFlushPending = false;
            OnStructureChanged();
        }
    }
}

} // namespace cw5
