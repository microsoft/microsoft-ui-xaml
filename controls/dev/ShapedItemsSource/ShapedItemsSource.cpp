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
#include <utility>
#include <winrt/Microsoft.UI.Dispatching.h>

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
}

void ShapedItemsSource::Start()
{
    // Not in the constructor: needs weak_from_this so an off-thread handler cannot outlive us.
    m_liveShaping->SetChangeHandler(
        [weakThis = weak_from_this(), queue = m_ownerQueue, ownerThreadId = m_ownerThreadId](winrt::IInspectable const& item, winrt::hstring const& propertyName)
        {
            if (queue && !queue.HasThreadAccess())
            {
                // Off-thread: all touched state belongs to the owning thread, so marshal it.
                queue.TryEnqueue([weakThis, item, propertyName]()
                    {
                        if (auto const strongThis = weakThis.lock())
                        {
                            strongThis->OnLiveShapedItemChanged(item, propertyName);
                        }
                    });
                return;
            }

            if (!queue && ::GetCurrentThreadId() != ownerThreadId)
            {
                // No queue to marshal to; drop it. The next owner-thread reshape re-reads items.
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

        // Untracked in-place edits are invisible to the fresh snapshots, so drop the retained
        // structure and post one restore to catch up.
        InvalidateRetainedHierarchyStructure();
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
    }
}

void ShapedItemsSource::SetFilter(ShapingHelpers::Predicate const& predicate)
{
    m_pipeline.SetFilter(predicate);
    ResetHierarchyFilterOverlay();
    ApplyShapingChange();
}

void ShapedItemsSource::SetFilter(winrt::hstring const& axisToken, ShapingHelpers::Predicate const& predicate)
{
    m_pipeline.SetFilter(axisToken, predicate);
    ResetHierarchyFilterOverlay();
    ApplyShapingChange();
}

void ShapedItemsSource::ClearFilter()
{
    m_pipeline.ClearFilter();
    ResetHierarchyFilterOverlay();
    ApplyShapingChange();
}

void ShapedItemsSource::ClearFilter(winrt::hstring const& axisToken)
{
    m_pipeline.ClearFilter(axisToken);
    ResetHierarchyFilterOverlay();
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

void ShapedItemsSource::SetParent(ShapingHelpers::KeySelector key, ShapingHelpers::KeySelector parentKey)
{
    m_keySelector = std::move(key);
    m_parentKeySelector = std::move(parentKey);
    // An in-flight index build (we ran from a selector) discards its result.
    ++m_parentDeclarationGeneration;

    // A different relation is a different tree: clear intent at next publish.
    m_parentRelationRedeclared = true;

    // The hierarchy axis is invisible to the spec diff; without this a parent verb is a no-op.
    m_hierarchyAxisDirty = true;

    // Grouping is NOT retracted: GroupBy buckets the roots (see RebuildGroupedHierarchical).
    ApplyShapingChange();
}

void ShapedItemsSource::ClearParentBy()
{
    // Nothing declared: no-op, avoid a needless Reset.
    if (!m_parentKeySelector)
    {
        // A deferred teardown may still be owed; finish it now.
        if (m_pendingHierarchyTeardown)
        {
            CompleteDeferredHierarchyTeardown();
        }
        return;
    }

    m_keySelector = nullptr;
    m_parentKeySelector = nullptr;
    ++m_parentDeclarationGeneration;
    m_parentRelationRedeclared = false;

    // Release now only if no publication (rebuild or re-slice) is on the stack; otherwise the outer
    // frame is still handing this adapter to consumers. The teardown stays owed until
    // non-hierarchical metadata publishes (the deferred rebuild may fail). Set before release.
    m_pendingHierarchyTeardown = true;
    if (!m_isRefreshing && !m_reslicingGroups)
    {
        ReleaseHierarchyProjection();
    }
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
    // hierarchy re-sorts every sibling set through a rebuilt parent-key index.
    if (m_groupSelector || m_projectedAsGrouped || m_parentKeySelector)
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

    // Full rebuild unless there is a live flat projection. A hierarchy always re-indexes: any
    // change can move rows anywhere in the tree.
    if (m_isRefreshing || m_groupSelector || m_parentKeySelector || !m_rows || m_kind == ProjectionKind::None)
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
    if (m_isRefreshing || m_groupSelector || m_parentKeySelector || HasActiveSort() || m_pipeline.HasFilter() ||
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
    return m_groupSelector || m_parentKeySelector || m_pipeline.HasFilter() || HasActiveSort() || IsSourceMutable();
}

bool ShapedItemsSource::HasAnyShapingVerb() const
{
    return m_pipeline.HasFilter() || m_groupSelector || m_parentKeySelector || HasActiveSort();
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
        auto rows = Materialize(authoritativeSource);
        RefreshLiveShapingSubscriptions(rows);

        // All snapshots were just recaptured, so any pending live restore is satisfied.
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
            // A hierarchy filters in its index build (needs the unfiltered parent chain).
            if (!m_parentKeySelector)
            {
                ApplyFilter(rows);
            }

            // A shaping verb is in force here (the branch above took the no-verb case), and a verb
            // always requires identity, so there is nothing to gate on.
            // A reused hierarchy structure was already validated.
            auto structure = m_parentKeySelector
                ? TryReuseHierarchyStructure(rows, m_parentDeclarationGeneration, m_refreshSourceStamp)
                : nullptr;
            wchar_t const* reason = nullptr;
            if (!structure && !ValidateRowIdentities(rows, reason))
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

            if (m_parentKeySelector && m_groupSelector)
            {
                RebuildGroupedHierarchical(rows, std::move(structure));
            }
            else if (m_parentKeySelector)
            {
                RebuildHierarchical(rows, std::move(structure));
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
    catch (...)
    {
        // A request that arrived during the failed pass describes newer state; post it (inline
        // would throw into the same app call). Only such requests replay, so bad data cannot loop.
        if (runPendingRefresh)
        {
            ScheduleRefreshReplay();
        }

        // An in-pass ClearParentBy must still take effect; its rebuild just failed.
        CompleteDeferredHierarchyTeardown();
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

    // Usually a no-op: the non-hierarchical rebuild already released the hierarchy.
    CompleteDeferredHierarchyTeardown();
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

    for (auto& bucket : keyedBuckets)
    {
        auto group = AcquireCachedGroup(bucket);
        group->SetItems(bucket.Items);
        groups.push_back(group.as<winrt::IInspectable>());
        liveKeys.insert(bucket.Identity);
    }

    PruneGroupCache(liveKeys);
    AttachGroupSource(groups);

    m_kind = ProjectionKind::Grouped;
    m_projectedAsGrouped = true;
    PublishProjection();
}

void ShapedItemsSource::RebuildHierarchical(std::vector<winrt::IInspectable>& rows, std::shared_ptr<const ShapingHelpers::ParentStructure> structure)
{
    // `rows` is UNFILTERED, in source order: the index build filters and sorts per sibling list.
    // Capture the generation before any selector (app code) can re-declare the relation.
    const uint64_t declarationGeneration = m_parentDeclarationGeneration;

    // May throw on invalid data; nothing is mutated yet.
    auto index = BuildHierarchyIndex(rows, std::move(structure), true /* sortRoots */, declarationGeneration);
    if (!index)
    {
        // Relation changed mid-build; the queued Refresh handles it.
        return;
    }

    // Rows come from an adapter now, so the in-place path must not re-sort stale layer-1 state.
    InvalidateShapingState();
    ClearFlatRowIdentityTracking();

    // Release any prior grouped projection so its adapter stops publishing headers.
    ReleaseGroupedProjection();
    m_hierarchyGroups.clear();
    ++m_hierarchyGeneration;
    m_rootBucketIndex.clear();

    // Ungrouped: one segment spanning every root.
    PublishHierarchyIndex(std::move(index), {});

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
void ShapedItemsSource::RebuildGroupedHierarchical(std::vector<winrt::IInspectable>& rows, std::shared_ptr<const ShapingHelpers::ParentStructure> structure)
{
    // Roots stay in source order for bucketing; sibling sets below are fully sorted.
    const uint64_t declarationGeneration = m_parentDeclarationGeneration;
    auto index = BuildHierarchyIndex(rows, std::move(structure), false /* sortRoots */, declarationGeneration);
    if (!index)
    {
        return;
    }

    // Bucket like RebuildGrouped: pre-GroupBy sorts set header order; later sorts apply per bucket.
    std::vector<winrt::IInspectable> roots = index->Roots;
    ApplySort(roots, -1, m_pipeline.GroupOrder());

    // Same fail-fast contract as RebuildGrouped.
    auto keyedBuckets = BucketizeGroupsOrThrow(roots);

    // Roots bucket by bucket. Each bucket's roots must be contiguous: the adapter walks one
    // segment per bucket and the re-slice cuts entries at depth-0 rows.
    std::vector<winrt::IInspectable> orderedRoots;
    orderedRoots.reserve(roots.size());
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

    index->Roots = std::move(orderedRoots);

    InvalidateShapingState();
    ClearFlatRowIdentityTracking();
    m_rootBucketIndex = std::move(rootBucketIndex);
    // Keep the publish below from re-slicing stale groups.
    m_hierarchyGroups.clear();
    ++m_hierarchyGeneration;

    PublishHierarchyIndex(std::move(index), std::move(segments));

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
    ResliceGroupsFromHierarchy();
    AttachGroupSource(groups);

    // Rows(): visible rows in group order, headers excluded.
    m_rows.ReplaceAll(VisibleHierarchicalRows());

    m_kind = ProjectionKind::GroupedHierarchical;
    // Presented via the grouped adapter, so header rows exist.
    m_projectedAsGrouped = true;
    PublishProjection();
}

std::shared_ptr<ShapingHelpers::ParentKeyIndex> ShapedItemsSource::BuildHierarchyIndex(
    std::vector<winrt::IInspectable> const& rows,
    std::shared_ptr<const ShapingHelpers::ParentStructure> structure,
    bool sortRoots,
    uint64_t declarationGeneration)
{
    // Relation changed while rows were prepared; selectors may be cleared, so do not run them.
    if (declarationGeneration != m_parentDeclarationGeneration)
    {
        m_pendingRefresh = true;
        return nullptr;
    }

    if (!structure)
    {
        // Selectors may re-declare the relation mid-build: copy them, then check the generation.
        auto const keySelector = m_keySelector;
        auto const parentKeySelector = m_parentKeySelector;

        auto built = std::make_shared<ShapingHelpers::ParentStructure>();
        winrt::hstring error;
        const bool valid = ShapingHelpers::BuildParentStructure(rows, keySelector, parentKeySelector, *built, error);

        if (declarationGeneration != m_parentDeclarationGeneration)
        {
            // Obsolete: drop the result and any error; ensure the replacing verb's Refresh runs.
            m_pendingRefresh = true;
            return nullptr;
        }

        if (!valid)
        {
            throw winrt::hresult_invalid_argument(Diagnostic(error));
        }

        structure = std::move(built);
        m_hierarchyStructure = structure;
        m_hierarchyStructureGeneration = declarationGeneration;
        m_hierarchyStructureSourceStamp = m_refreshSourceStamp;
    }

    ShapingHelpers::ParentKeyFilter filter;
    if (m_pipeline.HasFilter())
    {
        filter = [this](winrt::IInspectable const& item) { return m_pipeline.PassesFilter(item); };
    }

    ShapingHelpers::SiblingSorter sort;
    if (HasActiveSort())
    {
        sort = [this](std::vector<winrt::IInspectable>& siblings) { ApplySort(siblings); };
    }

    auto index = std::make_shared<ShapingHelpers::ParentKeyIndex>();
    ShapingHelpers::BuildParentKeyIndex(structure, filter, sort, sortRoots, *index);

    if (declarationGeneration != m_parentDeclarationGeneration)
    {
        // A filter or sort key selector re-declared the relation; as above.
        m_pendingRefresh = true;
        return nullptr;
    }

    return index;
}

std::shared_ptr<const ShapingHelpers::ParentStructure> ShapedItemsSource::TryReuseHierarchyStructure(
    std::vector<winrt::IInspectable> const& rows,
    uint64_t declarationGeneration,
    uint64_t sourceChangeStamp) const
{
    // Strong ref: selectors below may re-declare the relation and drop the member.
    auto const structure = m_hierarchyStructure;
    if (!structure ||
        m_hierarchyStructureGeneration != declarationGeneration ||
        m_hierarchyStructureSourceStamp != sourceChangeStamp ||
        structure->Rows.size() != rows.size())
    {
        return nullptr;
    }

    // Silent sources can change too; O(n) pointer compares are cheaper than selector calls.
    for (size_t i = 0; i < rows.size(); ++i)
    {
        if (winrt::get_abi(structure->Rows[i]) != winrt::get_abi(rows[i]))
        {
            return nullptr;
        }
    }

    // Live shaping already compared every edge this Refresh; otherwise re-read the keys (cheaper
    // than a rebuild). Copy selectors: a re-declaring selector reassigns the members.
    auto const keySelector = m_keySelector;
    auto const parentKeySelector = m_parentKeySelector;
    if (!IsLiveShapingEnabled() &&
        !ShapingHelpers::ParentStructureStillMatches(*structure, keySelector, parentKeySelector))
    {
        return nullptr;
    }

    // Re-declared while selectors ran; let the rebuild queue the replay.
    if (declarationGeneration != m_parentDeclarationGeneration)
    {
        return nullptr;
    }
    return structure;
}

void ShapedItemsSource::PublishHierarchyIndex(std::shared_ptr<ShapingHelpers::ParentKeyIndex> index, std::vector<size_t> rootSegments)
{
    if (!m_hierarchicalAdapter)
    {
        m_hierarchicalAdapter = std::make_shared<HierarchicalSourceAdapter>();
    }

    // Drop the old callback first: it would re-slice groups this rebuild is replacing.
    m_hierarchicalAdapter->ProjectionChanged(nullptr);

    // A re-declared relation is a different tree: its intent starts from the collapsed baseline.
    if (std::exchange(m_parentRelationRedeclared, false))
    {
        m_hierarchicalAdapter->ResetIntentQuietly();
    }

    m_hierarchicalAdapter->SetIndex(std::move(index), std::move(rootSegments));

    // Under grouping, node toggles splice only the hierarchy adapter; this carries them to the
    // groups. Uses the COHERENT edge: raw vector events fire per row, mid-repair.
    if (m_groupSelector)
    {
        std::weak_ptr<ShapedItemsSource> weakThis = weak_from_this();
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

void ShapedItemsSource::ResetHierarchyFilterOverlay()
{
    // New filter: context rows return to the default (expanded) state.
    if (m_hierarchicalAdapter)
    {
        m_hierarchicalAdapter->ResetFilterOverlay();
    }
}

void ShapedItemsSource::ReleaseHierarchyProjection()
{
    if (m_hierarchicalAdapter)
    {
        // Replacement may fail: keep visible rows for the inert fallback (see
        // CompleteDeferredHierarchyTeardown). Grouped trees keep rows in the slices.
        if (m_pendingHierarchyTeardown && m_kind == ProjectionKind::Hierarchical)
        {
            m_releasedHierarchyRows = VisibleHierarchicalRows();
        }

        // Callback first: nothing below may drive a re-slice on the way out.
        m_hierarchicalAdapter->ProjectionChanged(nullptr);
        // Also detaches: we may be inside its own notification, which must stop here.
        m_hierarchicalAdapter->ClearIndex();

        // A still-bound provider must not act on retracted intent.
        m_hierarchicalAdapter->ResetIntentQuietly();

        // Don't pin rows; a still-bound provider shares ownership until the swap.
        m_hierarchicalAdapter.reset();
    }

    m_hierarchyGroups.clear();
    ++m_hierarchyGeneration;
    m_rootBucketIndex.clear();
    // Holds every source row alive; a later relation rebuilds it anyway.
    m_hierarchyStructure.reset();
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
    // Pin/snapshot state up front: SetItems below runs app handlers that may tear down or rebuild.
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

        // On a throwing handler, still complete deferred work: teardown now, Refresh posted.
        auto completion = wil::scope_exit([this]() noexcept
        {
            m_reslicingGroups = false;
            if (!m_isRefreshing && !m_isApplyingIncrementalChange)
            {
                if (std::exchange(m_pendingRefresh, false))
                {
                    try { ScheduleRefreshReplay(); } catch (...) {}
                }
                CompleteDeferredHierarchyTeardown();
            }
        });

        // A generation bump (rebuild/teardown) ends the publish below.
        auto const groups = m_hierarchyGroups;
        const uint64_t generation = m_hierarchyGeneration;

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
            // Stale: an earlier SetItems handler replaced the hierarchy.
            if (generation != m_hierarchyGeneration)
            {
                break;
            }

            // Count before items: SetItems re-mints the header from the count.
            groups[i]->GroupChildCount(rootCounts[i]);
            groups[i]->SetItems(slices[i]);
        }

        completion.release();
    }

    // Replay a deferred Refresh unless an outer Refresh/incremental pass will.
    if (!m_isRefreshing && !m_isApplyingIncrementalChange)
    {
        if (std::exchange(m_pendingRefresh, false))
        {
            Refresh();
        }
        else
        {
            CompleteDeferredHierarchyTeardown();
        }
    }
}

void ShapedItemsSource::CompleteDeferredHierarchyTeardown()
{
    // An in-flight publication calls back here as it unwinds.
    if (!m_pendingHierarchyTeardown || m_isRefreshing || m_reslicingGroups)
    {
        return;
    }

    // Publication runs app code, so guard it like a rebuild; nested verbs coalesce or defer. On
    // failure the obligation stays for the next ClearParentBy/Refresh.
    bool const refreshAlreadyPending = m_pendingRefresh;
    {
        m_isRefreshing = true;
        auto guard = wil::scope_exit([this]() noexcept { m_isRefreshing = false; });

        TryCompleteDeferredHierarchyTeardown();
    }

    // Post (not run) a Refresh deferred during publication: often reached from an unwind.
    if (!refreshAlreadyPending && m_pendingRefresh && !m_isApplyingIncrementalChange)
    {
        m_pendingRefresh = false;
        try { ScheduleRefreshReplay(); } catch (...) {}
    }
}

void ShapedItemsSource::TryCompleteDeferredHierarchyTeardown()
{
    // Done only once non-hierarchical metadata was actually published (a rebuild may stage a flat
    // kind then fail), or the relation was re-declared.
    if (!m_pendingHierarchyTeardown || m_parentKeySelector || !m_hierarchyPublished)
    {
        m_pendingHierarchyTeardown = false;
        m_releasedHierarchyRows.clear();
        return;
    }

    // The replacing rebuild failed: republish the previous rows without the tree. Reached from
    // unwinds, so nothing escapes; the flag stays set for one retry per later call (no loop).
    try
    {
        if (m_kind == ProjectionKind::GroupedHierarchical)
        {
            // Same headers and slices; only the hierarchy goes.
            ReleaseHierarchyProjection();
            m_kind = ProjectionKind::Grouped;
        }
        else if (m_kind == ProjectionKind::Hierarchical)
        {
            if (m_hierarchicalAdapter)
            {
                m_releasedHierarchyRows = VisibleHierarchicalRows();
            }
            auto const visibleRows = std::move(m_releasedHierarchyRows);
            m_releasedHierarchyRows.clear();
            ReleaseHierarchyProjection();
            m_releasedHierarchyRows.clear();
            InvalidateShapingState();
            ClearFlatRowIdentityTracking();
            m_rows.ReplaceAll(visibleRows);
            // Degraded: later changes re-shape through Refresh.
            m_kind = ProjectionKind::Unshaped;
        }
        // Else a staged non-hierarchical projection failed to publish; republishing tears down.

        PublishProjection();
        m_pendingHierarchyTeardown = false;
        m_releasedHierarchyRows.clear();
    }
    catch (...)
    {
    }
}

void ShapedItemsSource::PublishProjection()
{
    // Marked "published" before the callback: a consumer may throw after swapping providers. Only
    // a completed non-hierarchical publication clears it.
    bool const hierarchical =
        m_kind == ProjectionKind::Hierarchical || m_kind == ProjectionKind::GroupedHierarchical;
    if (hierarchical)
    {
        m_hierarchyPublished = true;
    }

    RaiseProjectionRebuilt();

    if (!hierarchical)
    {
        m_hierarchyPublished = false;
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
            try { snapshot.GroupKey = winrt::hstring{ ShapingHelpers::MakeNodeKey(groupKey) }; } catch (...) {}
        }
    }

    // Keyed as the tree keys it; display strings can collide and hide a reparent.
    if (m_parentKeySelector)
    {
        auto const nodeKeyOf = [&item](ShapingHelpers::KeySelector const& selector) -> std::wstring
        {
            try { return ShapingHelpers::MakeNodeKey(selector(item)); } catch (...) { return {}; }
        };
        snapshot.NodeKey = nodeKeyOf(m_keySelector);
        snapshot.ParentKey = nodeKeyOf(m_parentKeySelector);
    }

    snapshot.PassesFilter = m_pipeline.PassesFilter(item);

    return snapshot;
}

void ShapedItemsSource::InvalidateRetainedHierarchyStructure() noexcept
{
    // Drops only the cache; an in-flight Refresh holds its own reference.
    m_hierarchyStructure.reset();
}

bool ShapedItemsSource::LiveShapeSnapshotsDiffer(
    LiveShapeSnapshot const& left,
    LiveShapeSnapshot const& right)
{
    return left.PassesFilter != right.PassesFilter ||
        left.GroupKey != right.GroupKey ||
        left.NodeKey != right.NodeKey ||
        left.ParentKey != right.ParentKey ||
        left.SortKeys != right.SortKeys;
}

void ShapedItemsSource::RefreshLiveShapingSubscriptions(
    std::vector<winrt::IInspectable> const& items)
{
    if (!IsLiveShapingEnabled())
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

    // Comparing fresh edge keys with previous ones validates the retained structure for free,
    // even for edits without PropertyChanged. Arrivals are covered by the source change stamp.
    bool edgeMoved = false;
    bool const checkEdges = m_parentKeySelector && m_hierarchyStructure;

    for (auto const& item : items)
    {
        if (!item)
        {
            continue;
        }

        auto const key = LiveShapingKeyFor(item);
        live.insert(key);
        m_liveShaping->Subscribe(item);
        auto const& snapshot = snapshots[key] = CaptureLiveShapeSnapshot(item);

        if (checkEdges && !edgeMoved)
        {
            // Object-keyed edges prove nothing (see IsObjectNodeKey); always rebuild.
            if (ShapingHelpers::IsObjectNodeKey(snapshot.NodeKey) || ShapingHelpers::IsObjectNodeKey(snapshot.ParentKey))
            {
                edgeMoved = true;
            }
            else if (auto const previous = m_liveShapeSnapshots.find(key); previous != m_liveShapeSnapshots.end())
            {
                edgeMoved = previous->second.NodeKey != snapshot.NodeKey || previous->second.ParentKey != snapshot.ParentKey;
            }
        }
    }

    if (edgeMoved)
    {
        InvalidateRetainedHierarchyStructure();
    }

    m_liveShaping->RetainOnly(live);
    m_liveShapeSnapshots = std::move(snapshots);
}

void ShapedItemsSource::AddLiveShapingSubscription(winrt::IInspectable const& item)
{
    if (!IsLiveShapingEnabled() || !item)
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
    // Not gated on IsLiveShapingEnabled: pruning leftovers is always correct.
    if (!item)
    {
        return;
    }

    m_liveShaping->Unsubscribe(item);
    m_liveShapeSnapshots.erase(LiveShapingKeyFor(item));
}

void ShapedItemsSource::ApplyLiveShapingDelta(
    winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
{
    if (!IsLiveShapingEnabled())
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
    // Nothing tracked, so a posted restore must not reshape.
    m_liveShapingDirty = false;
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
    if (!IsLiveShapingEnabled() || !item)
    {
        return;
    }

    // Before the coalescing early-out: the owner must see every changed item.
    if (m_liveItemChangedHook && m_liveItemChangedHook(item, m_liveShapingDirty))
    {
        MarkLiveShapingDirty();
        return;
    }

    // A full restore is already posted; skip snapshot capture so bulk edits cost one capture.
    if (m_liveShapingDirty)
    {
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
    if (m_liveShapingDirty)
    {
        return;
    }
    m_liveShapingDirty = true;

    // Post, don't reshape inline: coalesces a turn's writes into one rebuild and stays out of the
    // app's PropertyChanged handler.
    PostLiveShapingRestore();
}

void ShapedItemsSource::PostLiveShapingRestore()
{
    auto weakThis = weak_from_this();
    if (auto const queue = winrt::DispatcherQueue::GetForCurrentThread())
    {
        if (queue.TryEnqueue([weakThis]()
            {
                if (auto const strongThis = weakThis.lock())
                {
                    try
                    {
                        strongThis->RestoreLiveShaping();
                    }
                    catch (...)
                    {
                        // No app call to receive this; as in ScheduleRefreshReplay.
                    }
                }
            }))
        {
            return;
        }
    }

    // Couldn't post: restore eagerly rather than leave the projection stale.
    RestoreLiveShaping();
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

    // Posted: caller is closing an edit, often inside app callbacks or layout.
    PostLiveShapingRestore();
}
