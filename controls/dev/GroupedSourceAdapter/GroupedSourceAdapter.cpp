// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "GroupedSourceAdapter.h"
#include "GroupedEntry.h"
#include "RuntimeProfiler.h"
#include "ShapingHelpers.h"
#include "TabularRevoke.h"

#include <string_view>

#include <winrt/Microsoft.UI.Xaml.Interop.h>
#include <winrt/Microsoft.UI.Dispatching.h>

#include "GroupContract.h"

GroupedSourceAdapter::GroupedSourceAdapter()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_GroupedSourceAdapter);

    auto queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
    if (!queue)
    {
        throw winrt::hresult_error(RPC_E_WRONG_THREAD, L"GroupedSourceAdapter must be constructed on a UI thread.");
    }
    m_uiQueue = winrt::make_weak(queue);

    // One view over the stable entries vector, handed to every consumer.
    m_entriesView = winrt::ItemsSourceView{ m_entries };

    // Expansion intent lives in the model; the projection reacts to it in one place so a per-group
    // toggle and a programmatic ExpandAll reach the projection by exactly one path.
    m_expansion.SetChangedHandler(
        [this](TabularShapingHelpers::RowExpansionModel::Change const& change)
        {
            OnExpansionChanged(change);
        });
}

GroupedSourceAdapter::~GroupedSourceAdapter()
{
    DetachFromSource();
}

void GroupedSourceAdapter::Source(winrt::IInspectable const& value)
{
    if (m_source == value)
    {
        return;
    }

    DetachFromSource();
    m_source = value;
    AttachToSource();
    Rebuild();
}

void GroupedSourceAdapter::DetachSourceQuietly()
{
    DetachFromSource();
    m_source = nullptr;
}

void GroupedSourceAdapter::IncludeGroupHeaders(bool value)
{
    if (m_includeGroupHeaders == value)
    {
        return;
    }

    m_includeGroupHeaders = value;
    Rebuild();
}

// --- The one responsibility: materialize the flat list -----------------------------------------

void GroupedSourceAdapter::Rebuild()
{
    // Projection mutations are UI-thread-affine. Source notifications are required to arrive on the
    // owning UI thread and reach here synchronously; off-thread delivery is app misuse.
    AssertRebuildOnUiThread();

    if (m_rebuildInFlight)
    {
        // A change notification raised synchronously inside ReplaceAll re-entered Rebuild. Don't
        // drop it (that would leave m_entries stale) and don't nest it — remember it and run one
        // coalesced follow-up after the outer rebuild unwinds.
        m_pendingRebuild = true;
        return;
    }

    bool runPending = false;
    {
        m_rebuildInFlight = true;
        m_pendingRebuild = false;
        auto resetGuard = wil::scope_exit([this, &runPending]() noexcept
        {
            m_rebuildInFlight = false;
            runPending = m_pendingRebuild;
            m_pendingRebuild = false;
        });

        // Drop inner-group subscriptions before rebuilding; re-attach as we walk the (possibly new)
        // set of groups.
        UnsubscribeFromAllGroups();

        // Build into a local vector, then bulk-replace via a single Reset rather than firing N+1
        // VectorChanged events. ItemsRepeater treats one Reset as a wholesale rebuild; per-item Add
        // notifications during a known full rebuild force one layout invalidation per item.
        std::vector<winrt::IInspectable> built;
        std::unordered_set<winrt::hstring> liveGroupKeys;
        m_liveGroupsByIdentityKey.clear();
        m_ambiguousLiveGroupIdentityKeys.clear();

        const auto source = Source();
        if (source)
        {
            // Source must be iterable; each element is itself an iterable group.
            auto groups = TabularShapingHelpers::EnumerateInspectableItems(source);
            for (auto const& group : groups)
            {
                if (!group)
                {
                    continue;
                }

                // Compute the group's stable identity ONCE. A value-typed key (string department,
                // int year) yields a stable string that survives the key object being re-minted by
                // the next shaping pass; a non-value key has none.
                const auto identity = TabularShapingHelpers::GetGroupKeyIdentity(group);

                // The expansion-intent key: prefixed identity when the group has one, else a fall
                // back to the group object's own stable lookup key.
                const auto intentKey = !identity.empty()
                    ? winrt::hstring{ L"identity:" + identity }
                    : TabularShapingHelpers::TabularValueKey::ToObjectLookupKey(group, true);
                if (!intentKey.empty())
                {
                    liveGroupKeys.insert(intentKey);
                }

                // Track the live group by its declared identity so a control holding only a key can
                // resolve it. A key that resolves to two different objects is marked ambiguous and
                // resolves to neither.
                if (!identity.empty() && m_ambiguousLiveGroupIdentityKeys.find(identity) == m_ambiguousLiveGroupIdentityKeys.end())
                {
                    const auto [it, inserted] = m_liveGroupsByIdentityKey.emplace(identity, group);
                    if (!inserted && it->second != group)
                    {
                        m_liveGroupsByIdentityKey.erase(it);
                        m_ambiguousLiveGroupIdentityKeys.insert(identity);
                    }
                }

                // Materialize the group's items. Per-group sorting is applied upstream by
                // ShapedItemsSource::RebuildGrouped; the adapter reads the group as-is.
                //
                // An empty group is NOT dropped: group existence is source-owned (a bucket exists
                // only if the app authored it — a Kanban column, an "Uncategorized" bucket, an empty
                // drag target), so it must stay targetable.
                auto items = TabularShapingHelpers::EnumerateInspectableItems(group);
                const int32_t groupItemCount = static_cast<int32_t>(items.size());
                const bool isExpanded = m_expansion.IsExpanded(intentKey);

                // Collapsed groups still present a header (chrome stays targetable) but contribute no
                // data rows. The header carries the resolved count and expansion state so chevron
                // chrome and the automation peer read them without re-resolving against the adapter.
                if (m_includeGroupHeaders)
                {
                    built.push_back(winrt::make<::GroupedEntry>(group, groupItemCount, isExpanded));
                }

                if (isExpanded)
                {
                    // Data rows ARE the app items — no wrapper. This is the memory win the current
                    // stack already has.
                    for (auto const& item : items)
                    {
                        built.push_back(item);
                    }
                }

                // Subscribe so a change inside the group triggers a rebuild.
                SubscribeToGroup(group);
            }
        }

        // Drop intent for groups that no longer exist, or the store grows unbounded across changing
        // datasets. Pruning a dead key changes no live key's resolved state, so it is silent.
        m_expansion.RetainOnly(liveGroupKeys);

        // One Reset for the whole projection.
        m_entries.ReplaceAll(built);
    }

    // A re-entrant request that arrived while the guard was held runs exactly once here, after the
    // outer rebuild has fully unwound.
    if (runPending)
    {
        Rebuild();
    }
}

bool GroupedSourceAdapter::OnUiThread() const
{
    auto const queue = m_uiQueue.get();
    return queue && queue.HasThreadAccess();
}

void GroupedSourceAdapter::AssertRebuildOnUiThread() const
{
    // A source bound off the UI thread captures no queue, so affinity can't be proven either way
    // and the assert stands down rather than firing on something it cannot judge.
    MUX_ASSERT(!m_uiQueue.get() || OnUiThread());
}

// --- Expansion ---------------------------------------------------------------------------------

bool GroupedSourceAdapter::IsGroupExpanded(winrt::IInspectable const& group)
{
    return m_expansion.IsExpanded(GetGroupIntentKey(group));
}

void GroupedSourceAdapter::SetGroupExpanded(winrt::IInspectable const& group, bool isExpanded)
{
    if (!group)
    {
        return;
    }

    const auto key = GetGroupIntentKey(group);
    if (m_expansion.IsExpanded(key) == isExpanded)
    {
        return;  // No projection change.
    }

    // Through the model, not around it, so a programmatic ExpandAll and a per-group toggle share a
    // path. The model raises Changed -> OnExpansionChanged -> Rebuild.
    m_expansion.SetExpanded(key, isExpanded);
}

void GroupedSourceAdapter::ExpandAll()
{
    // Moves the BASELINE, so groups that do not exist yet also arrive expanded — "expand all" is an
    // intent, not a loop over the groups that happen to be live now.
    m_expansion.SetAllExpanded(true);
}

void GroupedSourceAdapter::CollapseAll()
{
    m_expansion.SetAllExpanded(false);
}

void GroupedSourceAdapter::OnExpansionChanged(TabularShapingHelpers::RowExpansionModel::Change const& change)
{
    AssertRebuildOnUiThread();

    if (m_rebuildInFlight)
    {
        // A change raised synchronously while a Rebuild/splice is unwinding. m_expansion already
        // reflects the new intent, so remember it and let the outer operation run one coalesced
        // Rebuild afterward rather than nesting.
        m_pendingRebuild = true;
        return;
    }

    // Only a single-group toggle can be a ranged splice. A baseline move (ExpandAll/CollapseAll)
    // or a multi-key batch shifts the whole visible-row set, for which a full Rebuild ending in one
    // Reset is both correct and cheapest.
    if (change.AffectsAllKeys || change.Keys.size() != 1)
    {
        Rebuild();
        return;
    }

    bool runPending = false;
    bool applied = false;
    {
        m_rebuildInFlight = true;
        m_pendingRebuild = false;
        auto resetGuard = wil::scope_exit([this, &runPending]() noexcept
        {
            m_rebuildInFlight = false;
            runPending = m_pendingRebuild;
            m_pendingRebuild = false;
        });

        applied = TryApplyExpansionSplice(change.Keys.front(), change.IsExpanded);
    }

    // A re-entrant request, or an incremental splice that couldn't resolve the group, resolves to a
    // single authoritative Rebuild after the guard unwinds.
    if (runPending || !applied)
    {
        Rebuild();
    }
}

bool GroupedSourceAdapter::TryApplyExpansionSplice(winrt::hstring const& intentKey, bool expand)
{
    // A headerless projection has no anchor to splice against (and can't show a collapse); the
    // intent is still stored, but the visible rows must come from a full Rebuild.
    if (!m_includeGroupHeaders)
    {
        return false;
    }

    const uint32_t count = m_entries.Size();
    for (uint32_t i = 0; i < count; ++i)
    {
        auto const entry = TryGetGroupedEntry(m_entries.GetAt(i));
        if (!entry)
        {
            continue;
        }

        auto const group = entry->Group();
        if (GetGroupIntentKey(group) != intentKey)
        {
            continue;
        }

        // Header found. If its stored state already matches the target, the projection is already
        // coherent for this group; nothing to splice.
        if (entry->IsExpanded() == expand)
        {
            return true;
        }

        const int32_t groupItemCount = entry->GroupItemCount();

        // Re-mint the header with the new expansion flag (GroupedEntry is immutable). One Replace at
        // i refreshes the chevron/automation without touching any other row.
        m_entries.SetAt(i, winrt::make<::GroupedEntry>(group, groupItemCount, expand));

        if (expand)
        {
            // Materialize this group's data rows and insert them immediately after the header. The
            // stored count must agree with the live enumeration; a mismatch means the group content
            // changed without a notification we processed, so defer to Rebuild for coherence.
            auto items = TabularShapingHelpers::EnumerateInspectableItems(group);
            if (static_cast<int32_t>(items.size()) != groupItemCount)
            {
                return false;
            }

            uint32_t insertAt = i + 1;
            for (auto const& item : items)
            {
                m_entries.InsertAt(insertAt++, item);
            }
        }
        else
        {
            // Collapse: remove exactly this group's data rows, which occupy the slots right after
            // the header up to the next header (or the end). If a slot isn't a data row where one is
            // expected, the projection isn't shaped the way this fast path assumes -> Rebuild.
            const uint32_t firstData = i + 1;
            for (int32_t removed = 0; removed < groupItemCount; ++removed)
            {
                if (firstData >= m_entries.Size() || TryGetGroupedEntry(m_entries.GetAt(firstData)))
                {
                    return false;
                }
                m_entries.RemoveAt(firstData);
            }
        }

        return true;
    }

    // The header for this key isn't materialized in the projection (mid-reshape); let Rebuild
    // reconcile from source.
    return false;
}

winrt::IInspectable GroupedSourceAdapter::ResolveLiveGroupByIdentity(winrt::hstring const& groupKey) const
{
    if (groupKey.empty() || m_ambiguousLiveGroupIdentityKeys.find(groupKey) != m_ambiguousLiveGroupIdentityKeys.end())
    {
        return nullptr;
    }

    const auto it = m_liveGroupsByIdentityKey.find(groupKey);
    return it != m_liveGroupsByIdentityKey.end() ? it->second : nullptr;
}

winrt::hstring GroupedSourceAdapter::GetGroupIntentKey(winrt::IInspectable const& group)
{
    // A value-typed key (a string department, an int year) yields a stable string that survives the
    // key object being re-minted by the next shaping pass.
    const auto identity = TabularShapingHelpers::GetGroupKeyIdentity(group);
    if (!identity.empty())
    {
        return L"identity:" + identity;
    }

    // A group whose key is not a value type has no stable string form; fall back to the group
    // object's own identity, stable across rebuilds because the producer reuses one instance per bucket.
    return TabularShapingHelpers::TabularValueKey::ToObjectLookupKey(group, true);
}

// --- Source / group subscription ---------------------------------------------------------------

void GroupedSourceAdapter::AttachToSource()
{
    auto src = Source();
    if (!src)
    {
        return;
    }

    m_attachedSourceForRevocation = src;

    // Notifications are required on the owning UI thread and applied synchronously as a full
    // Rebuild + Reset. INotifyCollectionChanged first — typed CLR ObservableCollection<T> won't
    // surface as IObservableVector.
    if (auto incc = src.try_as<winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged>())
    {
        m_outerCollectionChangedToken = incc.CollectionChanged(
            [weakThis = weak_from_this()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.lock()) { strongThis->Rebuild(); }
            });
    }
    else if (auto obs = src.try_as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>())
    {
        m_outerVectorChangedToken = obs.VectorChanged(
            [weakThis = weak_from_this()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.lock()) { strongThis->Rebuild(); }
            });
    }
    else if (auto bobs = src.try_as<winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector>())
    {
        m_outerBindableVectorChangedToken = bobs.VectorChanged(
            [weakThis = weak_from_this()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.lock()) { strongThis->Rebuild(); }
            });
    }
}

void GroupedSourceAdapter::DetachFromSource()
{
    auto attached = m_attachedSourceForRevocation;
    if (attached)
    {
        if (m_outerCollectionChangedToken.value != 0)
        {
            if (auto incc = attached.try_as<winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged>())
            {
                TabularShapingHelpers::SafeRevokeWith([&] { incc.CollectionChanged(m_outerCollectionChangedToken); });
            }
        }
        if (m_outerVectorChangedToken.value != 0)
        {
            if (auto obs = attached.try_as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>())
            {
                TabularShapingHelpers::SafeRevokeWith([&] { obs.VectorChanged(m_outerVectorChangedToken); });
            }
        }
        if (m_outerBindableVectorChangedToken.value != 0)
        {
            if (auto bobs = attached.try_as<winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector>())
            {
                TabularShapingHelpers::SafeRevokeWith([&] { bobs.VectorChanged(m_outerBindableVectorChangedToken); });
            }
        }
    }
    m_outerCollectionChangedToken = {};
    m_outerVectorChangedToken = {};
    m_outerBindableVectorChangedToken = {};
    m_attachedSourceForRevocation = nullptr;

    UnsubscribeFromAllGroups();
}

void GroupedSourceAdapter::SubscribeToGroup(winrt::IInspectable const& group)
{
    if (!group)
    {
        return;
    }

    InnerGroupSubscription sub;
    sub.GroupForRevocation = group;

    // Any change inside a group is a full Rebuild — no fast path, so no weak_ref to the group is
    // needed inside the callback (it captures only weak_from_this, never a strong back-ref).
    if (auto incc = group.try_as<winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged>())
    {
        sub.CollectionToken = incc.CollectionChanged(
            [weakThis = weak_from_this()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.lock()) { strongThis->Rebuild(); }
            });
    }
    else if (auto obs = group.try_as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>())
    {
        sub.Token = obs.VectorChanged(
            [weakThis = weak_from_this()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.lock()) { strongThis->Rebuild(); }
            });
    }
    else if (auto bobs = group.try_as<winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector>())
    {
        sub.BindableToken = bobs.VectorChanged(
            [weakThis = weak_from_this()](auto&&, auto&&)
            {
                if (auto strongThis = weakThis.lock()) { strongThis->Rebuild(); }
            });
    }

    if (sub.CollectionToken.value != 0 || sub.Token.value != 0 || sub.BindableToken.value != 0)
    {
        m_innerSubscriptions.push_back(std::move(sub));
    }
}

void GroupedSourceAdapter::UnsubscribeFromAllGroups()
{
    // Teardown is UI-thread-guaranteed by the owning ReferenceTrackers (TableViewSource / TableView),
    // so revoking here never crosses threads. SafeRevokeWith + the weak_from_this callbacks remain
    // for re-entrancy / GC-safety.
    for (auto& sub : m_innerSubscriptions)
    {
        auto strong = sub.GroupForRevocation;
        if (!strong)
        {
            continue;
        }
        if (sub.CollectionToken.value != 0)
        {
            if (auto incc = strong.try_as<winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged>())
            {
                TabularShapingHelpers::SafeRevokeWith([&] { incc.CollectionChanged(sub.CollectionToken); });
            }
        }
        if (sub.Token.value != 0)
        {
            if (auto obs = strong.try_as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>())
            {
                TabularShapingHelpers::SafeRevokeWith([&] { obs.VectorChanged(sub.Token); });
            }
        }
        if (sub.BindableToken.value != 0)
        {
            if (auto bobs = strong.try_as<winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector>())
            {
                TabularShapingHelpers::SafeRevokeWith([&] { bobs.VectorChanged(sub.BindableToken); });
            }
        }
    }
    m_innerSubscriptions.clear();
}
