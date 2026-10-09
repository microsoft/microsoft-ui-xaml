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
    // Installed here rather than in the constructor because it needs weak_from_this: a handler
    // still running on another thread while this source is destroyed must not touch it.
    m_liveShaping->SetChangeHandler(
        [weakThis = weak_from_this(), queue = m_ownerQueue, ownerThreadId = m_ownerThreadId](winrt::IInspectable const& item, winrt::hstring const& propertyName)
        {
            if (queue && !queue.HasThreadAccess())
            {
                // Raised off the owning thread. Everything the change touches (snapshots, the
                // dirty flag, the bound rows) belongs to the owning thread, so hand it over whole.
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
                // The owning thread has no queue to hand the change to, and acting on it here
                // would reshape from the wrong thread. It is dropped; the next reshape on the
                // owning thread re-reads every item.
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
        // With live shaping off, edges edited in place were never tracked, and the snapshots
        // about to be captured would already hold the new values -- nothing could tell the
        // retained structure is stale. Live shaping trusts its snapshots from here on, so start
        // it from a structure the next Refresh rebuilds.
        InvalidateRetainedHierarchyStructure();
        ResubscribeLiveShapingFromSource();

        // For the same reason, any item edited while untracked is still shaped by its old values,
        // and its fresh snapshot already agrees with the new ones -- no later change would notice.
        // One posted restore brings the projection up to date; it coalesces with whatever else
        // changes in this turn.
        MarkLiveShapingDirty();
    }
    else
    {
        ClearLiveShapingSubscriptions();
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
    // An index build in flight (this ran from inside one of the selectors) discards its result.
    ++m_parentDeclarationGeneration;

    // Last writer wins, and a different relation is a different tree: intent recorded against the
    // previous one is cleared at the next publish.
    m_parentRelationRedeclared = true;

    // The hierarchy axis lives here, NOT in the pipeline spec: it does not filter, bucket or sort,
    // it re-projects. So the spec diff cannot see it, and without this flag a parent verb declared
    // against an otherwise unchanged shape commits a no-op delta and returns.
    m_hierarchyAxisDirty = true;

    // Grouping is NOT retracted. The two axes compose: GroupBy buckets the roots, and each root
    // still expands into its own subtree beneath its bucket's header. See
    // RebuildGroupedHierarchical for the row stream that produces.
    ApplyShapingChange();
}

void ShapedItemsSource::ClearParentBy()
{
    // Nothing declared: genuinely a no-op, so do not fire a Reset that would drop every realized
    // row for a shape that is already flat.
    if (!m_parentKeySelector)
    {
        // Already cleared, but a teardown deferred behind a publication may still be owed. Once no
        // publication is on the stack a repeat call finishes it rather than leaving the tree live.
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

    // Torn down now only when no publication of this engine is on the stack (a rebuild -- which
    // includes a teardown completion's publication -- or a group re-slice). From inside one (an app
    // handler of a notification the rebuild or group re-slice raised) the outer frame is still
    // publishing THIS hierarchy and will hand its adapter to consumers; releasing it underneath would
    // hand them none. The Refresh requested below is deferred behind that frame, and every
    // non-hierarchical rebuild path releases the hierarchy itself, so the teardown still happens --
    // one coherent publication later. That rebuild can fail (invalid data), so the teardown is also
    // owed separately -- until non-hierarchical metadata is actually published -- and completed
    // when the publication unwinds either way. Set before the release so it snapshots the rows.
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

    // Consumed here regardless of the spec delta: the hierarchy axis is invisible to the diff, so
    // this flag is the only record that the projection kind must change.
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

    // The in-place paths splice the EXISTING projection; none of them can turn a flat projection
    // into a hierarchical one or back. A hierarchy change therefore always takes the rebuild.
    if (!hierarchyChanged && TryApplyShapingDeltaInPlace(delta))
    {
        // This path deliberately skips Refresh(), which is where live-shaping keys are normally
        // re-captured. The committed spec just changed, so every cached key describes the OLD
        // shape; re-capture against the new one. Subscriptions themselves are unaffected (the
        // reconcile is mark-and-sweep), so this costs one pass over the source and no COM churn.
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
        auto guard = wil::scope_exit([this]() noexcept { m_isApplyingIncrementalChange = false; });
        ApplyIncrementalChange(args);
    }
    catch (...)
    {
        // The application (or the full rebuild it fell back to) failed. A request deferred behind it
        // -- a verb issued from a publication callback, say -- would otherwise wait for the next
        // unrelated change. Posted, never run inline: the caller is already receiving this error.
        PostPendingRefresh();
        throw;
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
        // The application (or the full rebuild it fell back to) failed. A request deferred behind it
        // -- a verb issued from a publication callback, say -- would otherwise wait for the next
        // unrelated change. Posted, never run inline: the caller is already receiving this error.
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
    // Subscription maintenance is a DELTA, derived from the notification itself. Doing it here
    // rather than at each splice site below keeps it correct for every branch -- the sorted
    // fast-path, the flat splice, and the Refresh() fallbacks alike -- and costs work proportional
    // to the items that actually changed instead of to the size of the source.
    ApplyLiveShapingDelta(args);

    // Every path below either mutates m_rows without going through Refresh or falls back to
    // Refresh. The first leaves the retained layer-1 membership describing a projection that no
    // longer exists, so drop it up front rather than at each of the mutation sites; Refresh
    // repopulates it, and the in-place shaping path refuses to run without it.
    InvalidateShapingState();

    using winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;

    // A rebuild in flight, a grouped or hierarchical projection, or a not-yet-materialized
    // projection -> full rebuild (the incremental paths need a live flat projection + its
    // view/metadata). A hierarchy is re-indexed from the whole source on every change: an added
    // root, a reparented child or a removed parent can move rows anywhere in the tree.
    if (m_isRefreshing || m_groupSelector || m_parentKeySelector || !m_rows || m_kind == ProjectionKind::None)
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
    // Unlike NotifyCollectionChangedEventArgs, VectorChanged carries no items -- only a verb and
    // an index -- so the subscription delta cannot be derived up front. It is applied at each
    // splice site below, where the affected object is in hand. Every other path here ends in
    // Refresh(), which reconciles subscriptions itself.

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
        // Remove before add: when the slot was reassigned to the SAME object, adding first would
        // be a no-op and the removal would then drop the live subscription entirely.
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
    PublishProjection();
}

void ShapedItemsSource::Refresh()
{

    // Re-entrancy guard: a source notification that arrives while a rebuild is in flight
    // (e.g. an app mutating the source from a filter/sort/group callback) must not re-enter
    // ReplaceAll on the projection. Remember it and run one coalesced rebuild after the outer
    // rebuild unwinds so changes after materialization are not lost.
    //
    // A group re-slice is part of the same guarded publication: it mutates the grouped adapter's
    // groups one by one, and a rebuild nested inside it would replace those groups under its loop.
    // It is deferred the same way and replayed by the re-slice once it finishes. A teardown
    // completion publishes under m_isRefreshing too, so one requested from there is deferred as
    // well (see CompleteDeferredHierarchyTeardown).
    if (m_isRefreshing || m_reslicingGroups)
    {
        m_pendingRefresh = true;
        return;
    }

    bool const wasProjectedAsGrouped = m_projectedAsGrouped;
    bool runPendingRefresh = false;
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

        // Every snapshot was just recaptured from the current source against the committed spec,
        // so whatever a live-shaping change was waiting for has now happened -- whether this
        // refresh was the posted restore or something else that got here first. A restore that
        // arrives after this finds nothing to do and returns.
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
            // A hierarchy filters inside its index build, not here: keeping a match's ancestors
            // needs the unfiltered parent chain.
            if (!m_parentKeySelector)
            {
                ApplyFilter(rows);
            }

            // A shaping verb is in force here (the branch above took the no-verb case), and a verb
            // always requires identity, so there is nothing to gate on.
            //
            // A reshape over an unchanged source reuses the tree the last pass validated instead:
            // the same objects in the same order already passed this check.
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
        // The pass failed (invalid data) and its error must still reach the caller, with the
        // previous projection intact. But a request that arrived DURING the pass -- a selector or
        // handler that fixed the data, say -- describes newer state than the one that failed, and
        // the guard above has already consumed it. Replaying it synchronously would throw into the
        // same app call again, so it is posted instead. Only a request that arrived during the
        // failing pass is replayed, so persistently bad data cannot loop.
        if (runPendingRefresh)
        {
            ScheduleRefreshReplay();
        }

        // A ClearParentBy from inside the pass must still take effect: the rebuild that would have
        // released the hierarchy is the one that just failed.
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

    // Normally already done by the non-hierarchical rebuild that just published; this only acts if
    // the pass ended without releasing a hierarchy whose relation was cleared.
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

    // Kept when nothing could be posted (no dispatcher): the obligation then stays with the next
    // change, which sees the pending flag and rebuilds in full.
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

    // No dispatcher means no UI thread (test host): there is no later turn to post to, and the
    // next reshape re-materializes the source anyway.
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
            // No app call is on the stack to receive this. The data is still invalid; the previous
            // projection stays, and the next verb or source change re-validates and throws to the
            // app as usual.
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

    std::vector<ShapingHelpers::KeyedBucket> keyedBuckets;
    wchar_t const* rejectReason = nullptr;
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
        // Reported by throwing, not by a debug assert: this is caller data, not an internal
        // invariant, and an assert would abort the pass without its unwind (guards, deferred work)
        // in checked builds -- leaving a different engine state than the one apps get.
        LogIdentityProjectionDisabled(rejectReason);
        winrt::hstring message = Diagnostic(L"GroupBy key selector produced an invalid group identity");
        if (rejectReason)
        {
            message = message + L": " + winrt::hstring{ rejectReason };
        }
        throw winrt::hresult_invalid_argument(message);
    }

    // Plain grouping: no hierarchy axis, so any adapter a previous grouped+hierarchical projection
    // left behind must go before the group slices are rebuilt from the flat rows. Not before the
    // bucketize above: that can still throw, and the previous projection -- hierarchy included --
    // must then stay whole for the error contract (and for an owed teardown to republish).
    ReleaseHierarchyProjection();

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
    PublishProjection();
}

void ShapedItemsSource::RebuildHierarchical(std::vector<winrt::IInspectable>& rows, std::shared_ptr<const ShapingHelpers::ParentStructure> structure)
{
    // `rows` arrives UNFILTERED and in source order: the filter runs inside the index build, which
    // needs the whole parent chain to keep a match's ancestors, and so does the sort, which orders
    // each sibling set among its own peers only.
    //
    // The declaration generation is captured before any selector runs: sort key selectors are app
    // code too, and a ParentBy/ClearParentBy issued from one must make this pass obsolete.
    const uint64_t declarationGeneration = m_parentDeclarationGeneration;

    // May throw on invalid data. Nothing has been mutated yet, so the previous projection stays
    // intact.
    auto index = BuildHierarchyIndex(rows, std::move(structure), true /* sortRoots */, declarationGeneration);
    if (!index)
    {
        // The relation was re-declared or retracted mid-build; the queued Refresh projects that.
        return;
    }

    // Same reasoning as RebuildGrouped: the presented rows are materialized by an adapter, not by
    // the retained layer-1 state, so leaving that state live would let the in-place path re-sort a
    // projection that is no longer the one being shown.
    InvalidateShapingState();
    ClearFlatRowIdentityTracking();

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
    ++m_hierarchyGeneration;
    m_rootBucketIndex.clear();

    // Ungrouped: one segment spanning every root.
    PublishHierarchyIndex(std::move(index), {});

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
    PublishProjection();
}

// Both axes. The row stream is:
//
//   [Header A] [rootA1] [rootA1's expanded subtree...] [rootA2] ... [Header B] [rootB1] ...
//
// The group key is applied to the ROOTS only. A descendant is never re-bucketed: its depth is what
// makes it a descendant, and a row cannot simultaneously be at depth 3 under its parent and at
// depth 0 under a header. So headers exist at the top level and nowhere else, which is also what a
// user means by "group the tree". Under a filter, a context root is bucketed like any other root.
void ShapedItemsSource::RebuildGroupedHierarchical(std::vector<winrt::IInspectable>& rows, std::shared_ptr<const ShapingHelpers::ParentStructure> structure)
{
    // Every sibling set below the roots is sorted by the full sort, exactly as when ungrouped. The
    // roots are left in source order: they are bucketed from there below.
    // Generation captured before any selector runs, as in RebuildHierarchical.
    const uint64_t declarationGeneration = m_parentDeclarationGeneration;
    auto index = BuildHierarchyIndex(rows, std::move(structure), false /* sortRoots */, declarationGeneration);
    if (!index)
    {
        // Obsolete relation, as in RebuildHierarchical.
        return;
    }

    // The roots are bucketed exactly as RebuildGrouped buckets rows: in SOURCE order with only the
    // sorts declared BEFORE GroupBy applied, so those alone establish the group (header) order.
    // Sorts declared after GroupBy are applied within each bucket below.
    std::vector<winrt::IInspectable> roots = index->Roots;
    ApplySort(roots, -1, m_pipeline.GroupOrder());

    std::vector<ShapingHelpers::KeyedBucket> keyedBuckets;
    wchar_t const* degradeReason = nullptr;
    const bool grouped = ShapingHelpers::BucketizeToGroups(
        roots,
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
        LogIdentityProjectionDisabled(degradeReason);
        winrt::hstring message = Diagnostic(L"GroupBy key selector produced an invalid group identity");
        if (degradeReason)
        {
            message = message + L": " + winrt::hstring{ degradeReason };
        }
        throw winrt::hresult_invalid_argument(message);
    }

    // Roots in final presentation order: bucket by bucket, sorted within each. Contiguity is
    // load-bearing -- the adapter walks one segment per bucket (so a root's sibling position is per
    // bucket), and the re-slice below finds each bucket's rows by cutting the adapter's entries at
    // every depth-0 row, which only works if a bucket's roots are adjacent.
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
            // Keyed by raw COM pointer, not by identity string: this map is consulted once per
            // depth-0 entry during the re-slice, and the entries ARE the same objects.
            rootBucketIndex[winrt::get_abi(root)] = bucketIndex;
            orderedRoots.push_back(root);
        }
        segments.push_back(bucket.Items.size());
    }

    // The bucket-ordered roots replace the index's own root order before it is handed over.
    index->Roots = std::move(orderedRoots);

    InvalidateShapingState();
    ClearFlatRowIdentityTracking();
    m_rootBucketIndex = std::move(rootBucketIndex);
    // Stale slices must not be re-sliced by the publish below; they are rebuilt right after it.
    m_hierarchyGroups.clear();
    ++m_hierarchyGeneration;

    PublishHierarchyIndex(std::move(index), std::move(segments));

    if (!m_groupSource)
    {
        m_groupSource = winrt::single_threaded_observable_vector<winrt::IInspectable>();
    }

    // One ShapedGroup per bucket, reusing the cache on the same terms as RebuildGrouped so a
    // reshape does not churn the object a group publishes as Group().
    std::vector<winrt::IInspectable> groups;
    groups.reserve(keyedBuckets.size());
    std::unordered_set<winrt::hstring> liveKeys;
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
    PublishProjection();
}

std::shared_ptr<ShapingHelpers::ParentKeyIndex> ShapedItemsSource::BuildHierarchyIndex(
    std::vector<winrt::IInspectable> const& rows,
    std::shared_ptr<const ShapingHelpers::ParentStructure> structure,
    bool sortRoots,
    uint64_t declarationGeneration)
{
    // The relation changed while the caller was still preparing rows. The selectors below may
    // already be cleared, so do not run them at all.
    if (declarationGeneration != m_parentDeclarationGeneration)
    {
        m_pendingRefresh = true;
        return nullptr;
    }

    if (!structure)
    {
        // The selectors run app code, which may re-declare or retract the relation mid-build.
        // Copies keep the ones this pass started with alive, and the generation says whether they
        // are still the declared ones afterwards.
        auto const keySelector = m_keySelector;
        auto const parentKeySelector = m_parentKeySelector;

        auto built = std::make_shared<ShapingHelpers::ParentStructure>();
        winrt::hstring error;
        const bool valid = ShapingHelpers::BuildParentStructure(rows, keySelector, parentKeySelector, *built, error);

        if (declarationGeneration != m_parentDeclarationGeneration)
        {
            // Obsolete: whatever this pass built or rejected describes a relation that is no longer
            // declared, so neither the structure nor its validation error may surface. The verb
            // that replaced it requested a Refresh, which the one on the stack replays; making sure
            // of that here keeps the latest declaration from being dropped.
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
    // A strong reference: the check below runs app selectors, which may re-declare or clear the
    // relation and drop the member.
    auto const structure = m_hierarchyStructure;
    if (!structure ||
        m_hierarchyStructureGeneration != declarationGeneration ||
        m_hierarchyStructureSourceStamp != sourceChangeStamp ||
        structure->Rows.size() != rows.size())
    {
        return nullptr;
    }

    // A source that raises no notifications (or one raised off the contract) can still change
    // between passes. Comparing objects is O(n) pointer reads against the selector calls it saves.
    for (size_t i = 0; i < rows.size(); ++i)
    {
        if (winrt::get_abi(structure->Rows[i]) != winrt::get_abi(rows[i]))
        {
            return nullptr;
        }
    }

    // Same rows; are they still related the same way? Under live shaping, this Refresh compared
    // every row's fresh node and parent key with the last pass and dropped the structure if any
    // moved, so reaching here already answers it. Without live shaping nothing has looked, so
    // re-read the keys: a reshape picks up an edge edited in place exactly as it picks up a sort or
    // filter key, while skipping the key table, validation and cycle check a rebuild would redo.
    // Copies, as in BuildHierarchyIndex: a selector that re-declares the relation reassigns the
    // members while they run.
    auto const keySelector = m_keySelector;
    auto const parentKeySelector = m_parentKeySelector;
    if (!IsLiveShapingEnabled() &&
        !ShapingHelpers::ParentStructureStillMatches(*structure, keySelector, parentKeySelector))
    {
        return nullptr;
    }

    // A selector may have re-declared the relation while running. Rebuilding sees that and queues
    // the replay, so leave it to the rebuild.
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

    // Drop the previous projection callback before the publish: it is re-established below only
    // when this projection actually needs it, and a stale one would re-slice groups that this
    // rebuild is in the middle of replacing.
    m_hierarchicalAdapter->ProjectionChanged(nullptr);

    // A re-declared relation is a different tree: its intent starts from the collapsed baseline.
    if (std::exchange(m_parentRelationRedeclared, false))
    {
        m_hierarchicalAdapter->ResetIntentQuietly();
    }

    m_hierarchicalAdapter->SetIndex(std::move(index), std::move(rootSegments));

    // Under grouping the adapter's entries are NOT the presented axis -- the grouped adapter's
    // are -- so a node toggle, which splices the hierarchy adapter in place, would otherwise be
    // invisible. This callback is what carries it across to the groups. It is the adapter's
    // COHERENT edge rather than the raw vector notification: a multi-row splice raises the latter
    // once per row and from inside the mutation, so a re-slice driven by it would read the
    // descriptor side-table mid-repair and would do it N times.
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
    // A changed filter is a new question: context rows the user collapsed under the previous one
    // go back to the default (expanded) for the new one. The next refresh publishes.
    if (m_hierarchicalAdapter)
    {
        m_hierarchicalAdapter->ResetFilterOverlay();
    }
}

void ShapedItemsSource::ReleaseHierarchyProjection()
{
    if (m_hierarchicalAdapter)
    {
        // A cleared relation whose replacement may yet fail: keep what the tree was showing, so an
        // inert fallback can still be published for the rows on screen (see
        // CompleteDeferredHierarchyTeardown). Grouped trees keep their rows in the group slices.
        if (m_pendingHierarchyTeardown && m_kind == ProjectionKind::Hierarchical)
        {
            m_releasedHierarchyRows = VisibleHierarchicalRows();
        }

        // Callback first: nothing below may drive a re-slice on the way out.
        m_hierarchicalAdapter->ProjectionChanged(nullptr);
        // Also detaches it: this can run from inside one of its own notifications (an app handler
        // calling ClearParentBy), and the publication still on the stack must stop there.
        m_hierarchicalAdapter->ClearIndex();

        // Inert from here on: a row-metadata provider still bound to it until the swap must not
        // act on intent recorded against the retracted relation.
        m_hierarchicalAdapter->ResetIntentQuietly();

        // Drop the engine's reference so the last visible rows (and their descriptors) are not kept
        // alive by a projection that is no longer presented. The previous row-metadata provider
        // shares ownership, so the adapter stays valid for anything still bound to it until the
        // projection swap replaces that provider; the next declaration creates a fresh adapter.
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
    // Everything this pass reads is pinned or snapshotted up front. The SetItems calls below notify
    // the grouped adapter and through it app handlers, and a handler may retract or rebuild the
    // hierarchy (ClearParentBy tears it down directly; a Refresh is deferred, see below).
    auto const adapter = m_hierarchicalAdapter;
    if (!adapter || m_hierarchyGroups.empty())
    {
        return;
    }

    // Setting a group's Items notifies the grouped adapter, which rebuilds; that rebuild must not
    // re-enter here. Plain bool, matching the adapters: this whole stack is UI-thread-affine.
    if (m_reslicingGroups)
    {
        return;
    }

    {
        m_reslicingGroups = true;
        auto guard = wil::scope_exit([this]() noexcept { m_reslicingGroups = false; });

        // A handler of one of the SetItems below can throw. The re-slice guard is released on the
        // way out, and what the pass deferred behind itself -- a Refresh, a ClearParentBy teardown --
        // must not be lost with it: the teardown completes now, the Refresh is posted (replaying it
        // synchronously would throw into the same app call).
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

        // The groups this pass publishes into, and the generation they belong to. Any rebuild or
        // teardown that replaces them bumps the generation, which ends the publish below.
        auto const groups = m_hierarchyGroups;
        const uint64_t generation = m_hierarchyGeneration;

        std::vector<std::vector<winrt::IInspectable>> slices(groups.size());
        // Roots per bucket, tracked separately from the slice size: the slice also carries every
        // visible descendant, and a header must count the children it owns, not the rows it spans.
        std::vector<int32_t> rootCounts(groups.size(), 0);

        auto const entries = adapter->Entries();
        const int32_t count = entries ? entries.Count() : 0;

        // One pass. Every depth-0 row opens the bucket it was assigned at build time and every row
        // after it belongs to that bucket until the next depth-0 row -- which is exactly the
        // adapter's own emission order, a root immediately followed by its visible subtree.
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
                    // A root the bucket map does not know. Only reachable if the source mutated
                    // between the bucketize and this pass, in which case a full rebuild is already
                    // queued; drop this subtree rather than charge it to the previous bucket.
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
            // Stale: a handler of an earlier SetItems retracted or replaced the hierarchy. The rest
            // of these slices describe a projection that is no longer the one presented.
            if (generation != m_hierarchyGeneration)
            {
                break;
            }

            // Count before items: SetItems notifies the grouped adapter, which re-mints the header
            // entry, and the header must never be built from a stale count.
            groups[i]->GroupChildCount(rootCounts[i]);
            groups[i]->SetItems(slices[i]);
        }

        completion.release();
    }

    // A Refresh requested while re-slicing was deferred (see Refresh). Replay it now, unless an
    // outer Refresh or incremental application is on the stack; each of those replays it itself.
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
    // A publication on the stack completes the teardown itself: Refresh and the group re-slice both
    // call back here as they unwind, on success and failure alike.
    if (!m_pendingHierarchyTeardown || m_isRefreshing || m_reslicingGroups)
    {
        return;
    }

    // The teardown publication runs app code (the consumer's swap raises property-change,
    // selection and collection notifications), so it is guarded exactly like a rebuild. A
    // ClearParentBy from there coalesces into this attempt instead of publishing again from inside
    // it; a Refresh or source change is deferred behind it. If this attempt fails, the obligation
    // stays for the next ClearParentBy or Refresh.
    bool const refreshAlreadyPending = m_pendingRefresh;
    {
        m_isRefreshing = true;
        auto guard = wil::scope_exit([this]() noexcept { m_isRefreshing = false; });

        TryCompleteDeferredHierarchyTeardown();
    }

    // A Refresh requested from inside the publication (a verb or source change in a handler) was
    // deferred behind it. Posted rather than run here: this is often reached from an unwind, where
    // nothing may throw. An outer incremental application replays a pending Refresh itself.
    if (!refreshAlreadyPending && m_pendingRefresh && !m_isApplyingIncrementalChange)
    {
        m_pendingRefresh = false;
        try { ScheduleRefreshReplay(); } catch (...) {}
    }
}

void ShapedItemsSource::TryCompleteDeferredHierarchyTeardown()
{
    // Done once the consumer has actually been handed non-hierarchical metadata. Neither a released
    // adapter nor a non-hierarchical m_kind is enough: a rebuild can release the adapter and stage a
    // flat kind, then fail before or during publication, leaving the previous provider (and the
    // detached adapter it owns) in place. Re-declared since: the new relation's rebuild owns the
    // projection.
    if (!m_pendingHierarchyTeardown || m_parentKeySelector || !m_hierarchyPublished)
    {
        m_pendingHierarchyTeardown = false;
        m_releasedHierarchyRows.clear();
        return;
    }

    // The rebuild that would have replaced this projection failed, so the error contract keeps the
    // previous rows on screen. They stay -- but no longer as a tree: the relation is gone, so the
    // adapter is released and the rows are republished without it, not expandable and not bound to
    // intent recorded against the retracted relation. The next successful Refresh re-shapes them.
    //
    // Reached from an unwind (a failed pass, a scope guard, a repeat ClearParentBy), so nothing here
    // may escape: the pass's own error is the one the app hears. The flag stays set until a
    // publication succeeds, so a repeat ClearParentBy or the next Refresh can retry it; each of those
    // makes at most one attempt, so a publication that keeps failing cannot loop.
    try
    {
        if (m_kind == ProjectionKind::GroupedHierarchical)
        {
            // The grouped adapter still presents the same headers and slices; only the hierarchy
            // reading of them goes.
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
            // Degraded: a verb is still configured, so every later change re-shapes through
            // Refresh rather than splicing into these rows.
            m_kind = ProjectionKind::Unshaped;
        }
        // Otherwise a non-hierarchical projection is already staged (a rebuild or an earlier
        // fallback got that far) but its publication failed: publishing it again is the teardown.

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
    // Conservatively "published" before the callback: a consumer that throws part-way may already
    // have swapped to the hierarchical provider. Only a completed non-hierarchical publication
    // retires it. An app handler reached from the callback can throw (the event sources used by
    // the consumer propagate handler exceptions), which is why this is tracked apart from m_kind.
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

    // Selectors are app code. The rebuild treats a throwing selector as having returned null, so
    // the snapshot does too -- otherwise the throw would surface from the app's property setter.
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
        // Compared as the grouping compares it: by group identity, not display text. Two group
        // key objects usually print alike (often just their type name), so the text would hide a
        // regroup. A key with no identity is rejected by the rebuild; its object form still tells
        // a change apart.
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

    // The edge is keyed exactly as the tree keys it: a display string can collide (an object
    // key's IStringable is often just its type name), and would hide a reparent the tree sees.
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
    // Only the cache is dropped; a Refresh already on the stack holds its own reference. The next
    // Refresh re-runs the key and parent selectors and retains the structure they produce.
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

    // Mark-and-sweep against the complete source. Clearing and rebuilding would revoke and re-add
    // every subscription on every refresh -- two COM calls plus a QI per row to arrive back at the
    // subscription set we already had. Here an item that is still present keeps its existing
    // revoker untouched, so only arrivals subscribe and only departures revoke.
    std::unordered_set<void const*> live;
    live.reserve(items.size());

    // Snapshots are rebuilt rather than reconciled: a refresh is also where the committed shaping
    // spec can have changed (a new sort axis, a different filter), which invalidates every cached
    // key. Building into a fresh map drops entries for departed items as a side effect.
    std::unordered_map<void const*, LiveShapeSnapshot> snapshots;
    snapshots.reserve(items.size());

    // The fresh snapshots already hold every row's node and parent key, so comparing them with the
    // previous ones tells whether the retained tree structure is still true at no selector cost.
    // This catches an edge edited without a PropertyChanged too. Rows with no previous snapshot
    // are arrivals, which the source change stamp already accounts for.
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
            // An object-keyed edge proves nothing (see IsObjectNodeKey), so it always rebuilds, as
            // it does with live shaping off.
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
}

void ShapedItemsSource::RemoveLiveShapingSubscription(winrt::IInspectable const& item)
{
    // Deliberately not gated on IsLiveShapingEnabled: pruning an entry left over from a mode that
    // has since been turned off is always correct, and never pruning would strand it.
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
        // Membership is unchanged; only ordering moved, which no subscription depends on.
        break;
    case NotifyCollectionChangedAction::Reset:
    default:
        // A reset says nothing about which items survived. Every caller funnels a reset into
        // Refresh(), whose mark-and-sweep reconcile is the cheapest correct answer.
        break;
    }
}

void ShapedItemsSource::ClearLiveShapingSubscriptions()
{
    m_liveShaping->UnsubscribeAll();
    m_liveShapeSnapshots.clear();
    // Nothing is tracked any more, so there is no stale shape to restore. Leaving this set would
    // hand a posted restore a reason to re-shape after live shaping was switched off.
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

    // Ahead of the coalescing early-out: every changed item has to reach the owner, or derived
    // state the restore reads would miss items that changed after the first.
    if (m_liveItemChangedHook && m_liveItemChangedHook(item, m_liveShapingDirty))
    {
        MarkLiveShapingDirty();
        return;
    }

    // A restore is already posted, and it re-shapes from the source in full. A second changed item
    // cannot add anything to that, so there is nothing to learn by pricing its snapshot. This is
    // what makes a bulk mutation cost one snapshot capture rather than one per changed row --
    // without it the early-out below still runs every sort selector, the group selector and the
    // filter predicate for each notification.
    if (m_liveShapingDirty)
    {
        return;
    }

    // The subscription is deliberately blanket: the source raises PropertyChanged for properties
    // no active verb reads, and this is where those are discarded. The snapshot is compared, not
    // stored -- until the restore runs, the cached snapshot is the shape the projection actually
    // reflects, and overwriting it here would claim a reshape that has not happened.
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

    // Posting rather than re-shaping inline is the whole point. An app that writes several
    // properties, or several rows, does so within one turn; re-shaping on each write would rebuild
    // the projection once per write and publish a Reset the UI has to absorb each time. Deferring
    // to the queue collapses the whole turn into a single rebuild, and it also keeps the reshape
    // out of the app's own PropertyChanged handler, where re-entering the projection would be
    // hostile.
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
                        // No app call is on the stack to receive this (e.g. a ParentBy key edited
                        // into a duplicate or a cycle). As with ScheduleRefreshReplay: the previous
                        // projection stays, and the next verb or source change re-validates and
                        // throws to the app.
                    }
                }
            }))
        {
            return;
        }
    }

    // No queue on this thread, or the queue is shutting down and refused the work. Degrade to the
    // eager behaviour: slower, but a stale projection that never restores would be a correctness
    // bug, and silently dropping the change is worse than paying for it now.
    RestoreLiveShaping();
}

void ShapedItemsSource::RestoreLiveShaping()
{
    if (!m_liveShapingDirty)
    {
        // A refresh ran for another reason between the mark and this callback -- a shaping verb, a
        // collection change -- and it recaptured every snapshot. The projection is already true.
        return;
    }

    if (!IsLiveShapingEnabled())
    {
        // Live shaping was turned off while this was in flight. Whatever was stale is no longer
        // anyone's concern: with the flags down the projection is not expected to track items.
        m_liveShapingDirty = false;
        return;
    }

    if (m_liveShapingHold && m_liveShapingHold())
    {
        // Moving rows now would end the user's edit. Stay dirty: further changes coalesce into
        // this restore, and the owner resumes it once the edit closes.
        m_liveShapingHeld = true;
        return;
    }

    Refresh();

    // Nothing else tells a UIA client the rows moved: no verb was called, so the owner hears of no
    // shaping change. Any row may have been regrouped or filtered, not just reordered.
    RaiseShapingChanged(false /* reorderOnly */);
}

void ShapedItemsSource::ResumeHeldLiveShaping()
{
    if (!std::exchange(m_liveShapingHeld, false))
    {
        return;
    }

    // Posted, not run here: the caller is closing an edit, often from inside app callbacks or a
    // layout pass. If the hold is back on by the time it runs, the restore simply parks again.
    PostLiveShapingRestore();
}
