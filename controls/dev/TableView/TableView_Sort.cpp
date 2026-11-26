// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//
// Single-column sort for TableView.
//
// Every ItemsSource is shown through a view (TableView wraps a plain collection in an internal
// CollectionViewSource), and the view sorts, as WPF's DataGrid sorts its Items view. A header click
// writes the column's sort to the view's ICollectionViewShaping: a SortMemberPath becomes its only
// SortDescription, a CustomSortComparer its CustomSort. Sorting is raised first as a cancellable
// pre-event; an app that would rather order its own collection cancels it and owns both the rows
// and the column's sort state from there. A view without ICollectionViewShaping (an app's own
// ICollectionView) is not sorted by TableView: the header click raises Sorting, and the app sorts.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewColumn.h"
#include "TableViewColumnHeader.h"
#include "TableViewSortingEventArgs.h"
#include "TableViewSortedEventArgs.h"
#include "TableViewAutomationHelpers.h"
#include "ResourceAccessor.h"
#include "Utils.h"
#include "TVDiag.h"

#include <algorithm>
#include <numeric>
#include <unordered_map>

namespace
{
    winrt::hstring LocalizedOrFallback(std::wstring_view resourceName, std::wstring_view fallback) noexcept
    {
        try
        {
            if (auto const resolved = ResourceAccessor::GetLocalizedStringResource(resourceName); !resolved.empty())
            {
                return resolved;
            }
        }
        catch (...)
        {
        }

        return winrt::hstring{ fallback };
    }

    bool HasResolvedSortMemberPath(const winrt::TableViewColumn& column)
    {
        // Dispatch on the projection, not winrt::get_self: GetSortMemberPathCore is `overridable`,
        // so get_self would call the C++ implementation's virtual directly and bypass WinRT
        // composition. A column authored in C# that overrides it to compute a sort path would then
        // be treated as having none, and silently become unsortable.
        return column && !column.GetSortMemberPathCore().empty();
    }

    // CanUserSort gates user input only, so it is not checked here.
    bool CanSortColumn(const winrt::TableViewColumn& column)
    {
        if (!column)
        {
            return false;
        }

        return column.CustomSortComparer() || HasResolvedSortMemberPath(column);
    }

    struct SortKey
    {
        winrt::hstring MemberPath;
        winrt::ITableViewSortComparer Comparer{ nullptr };
    };

    // A comparer ignores the path, and is compared by identity.
    SortKey EffectiveSortKey(const winrt::TableViewColumn& column)
    {
        if (auto comparer = column.CustomSortComparer())
        {
            return { {}, std::move(comparer) };
        }
        return { column.GetSortMemberPathCore(), nullptr };
    }

    bool IsSameSortKey(const SortKey& key, const winrt::hstring& memberPath, const winrt::ITableViewSortComparer& comparer)
    {
        return key.Comparer ? key.Comparer == comparer : !comparer && key.MemberPath == memberPath;
    }
}

// Whether user input can sort this column: CanSortColumn does not check the CanUserSort gates.
bool TableView::IsColumnUserSortableInternal(const winrt::TableViewColumn& column)
{
    return column && CanUserSortColumns() && column.CanUserSort() && CanSortColumn(column);
}

bool TableView::IsSortClearStillValid() const
{
    return !m_sortedColumns.empty();
}

bool TableView::CanSortColumnNow(const winrt::TableViewColumn& column, bool requireCanSort) const
{
    return requireCanSort ? CanSortColumn(column) : static_cast<bool>(column);
}

bool TableView::IsSortRequestStillValid(const winrt::TableViewColumn& column, bool requireCanSort) const
{
    if (!column || !CanSortColumnNow(column, requireCanSort))
    {
        return false;
    }

    // A clear that skips the sort-key check clears only the column's own sort. A replayed or
    // re-entered one may find another column sorted since, and must not clear that sort.
    if (!requireCanSort && column.SortDirection() == winrt::SortDirection::None)
    {
        return false;
    }

    // The column must still be one of ours: a Sorting handler is free to remove it.
    if (auto columns = const_cast<TableView*>(this)->Columns())
    {
        for (auto const& ownedColumn : columns)
        {
            if (ownedColumn == column)
            {
                return true;
            }
        }
    }

    return false;
}

bool TableView::SortByColumn(const winrt::TableViewColumn& column, winrt::SortDirection direction)
{
    // A column that resolves no sort key can still have the sort it holds cleared.
    const bool clearsOwnSort = direction == winrt::SortDirection::None &&
        column && column.SortDirection() != winrt::SortDirection::None;
    return SortByColumnInternal(column, direction, true, !clearsOwnSort);
}

bool TableView::ToggleSortDirectionFromUserInput(const winrt::TableViewColumn& column)
{
    if (!column || !column.CanUserSort() || !CanUserSortColumns())
    {
        return false;
    }
    return ToggleSortDirection(column);
}

template <typename F>
bool TableView::RunSortTransition(F&& transition)
{
    const bool outermost = m_sortTransitionDepth == 0;
    bool result;
    {
        // Declared first so it runs after the depth is restored: a throwing handler must not
        // strand the changes made before it threw.
        auto reconcileOnThrow = wil::scope_exit([this, outermost]() noexcept
        {
            if (outermost)
            {
                ScheduleSortKeyReconcile(false);
            }
        });
        ++m_sortTransitionDepth;
        auto const leave = wil::scope_exit([this]() noexcept { --m_sortTransitionDepth; });
        result = transition();
        reconcileOnThrow.release();
    }

    if (outermost)
    {
        ScheduleSortKeyReconcile(true);
    }
    return result;
}

// Posted rather than run inline so a follow-up re-sort never runs inside, or recurses from, the
// sort whose handler changed the key. A handler that changes the key on every Sorted keeps
// re-sorting once per turn; that is an app error. Both passes run app Sorting/Sorted handlers that
// can throw: the synchronous pass throws to the caller, and the posted one through the dispatcher
// to Application.UnhandledException, as any posted sort does (the spec's Sorting/Sorted section).
void TableView::ScheduleSortKeyReconcile(bool allowSynchronous)
{
    if (!m_sortKeysDirty || m_sortTransitionDepth || m_reconcilingSortKeys || m_sortKeyReconcileQueued)
    {
        return;
    }

    try
    {
        if (auto const queue = winrt::DispatcherQueue::GetForCurrentThread())
        {
            m_sortKeyReconcileQueued = queue.TryEnqueue([weakThis = get_weak()]()
            {
                if (auto const strongThis = weakThis.get())
                {
                    strongThis->m_sortKeyReconcileQueued = false;
                    // A transition in progress schedules it again when it ends.
                    if (!strongThis->m_sortTransitionDepth)
                    {
                        strongThis->ReconcileSortKeys();
                    }
                }
            });
            if (m_sortKeyReconcileQueued)
            {
                return;
            }
        }
    }
    catch (...)
    {
    }

    // No dispatcher: one pass right after the outermost transition. Changes made by its own
    // handlers stay dirty for the next one.
    if (allowSynchronous)
    {
        ReconcileSortKeys();
    }
}

void TableView::ReconcileSortKeys()
{
    m_sortKeysDirty = false;
    m_reconcilingSortKeys = true;
    auto const reschedule = wil::scope_exit([this]() noexcept
    {
        m_reconcilingSortKeys = false;
        ScheduleSortKeyReconcile(false);
    });

    for (auto const& column : SnapshotSortedColumns())
    {
        // An earlier re-sort in this pass may have moved it, or unsorted it.
        if (winrt::get_self<TableViewColumn>(column)->GetOwningTableView() == *this &&
            column.SortDirection() != winrt::SortDirection::None)
        {
            ReconcileSortKey(column);
        }
    }
}

bool TableView::IsInstalledSortKeyFor(const winrt::TableViewColumn& column) const
{
    // The app's own sort on the view (it wrote SortDescriptions or CustomSort itself) is not keyed on a column,
    // and neither is a direction the app only displayed: that sort is the app's, so a key change never re-sorts it.
    if (!column || !m_collectionViewShaping || !m_collectionViewSortByTable || !m_installedSortKey || m_sortDisplaySetByApp ||
        m_installedSortKey->Column.get() != column)
    {
        return false;
    }

    // The view still sorts with what this table wrote. An app that set the view's CustomSort itself raised no
    // SortDescriptions change, so this is checked here rather than tracked.
    try
    {
        auto const customSort = m_collectionViewShaping.CustomSort();
        if (m_collectionViewCustomSort)
        {
            return customSort == m_collectionViewCustomSort;
        }
        if (customSort)
        {
            return false;
        }
        auto const descriptions = m_collectionViewShaping.SortDescriptions();
        return descriptions.Size() == 1 && descriptions.GetAt(0) && descriptions.GetAt(0).PropertyName() == m_installedSortKey->MemberPath;
    }
    catch (...)
    {
        return false;
    }
}

void TableView::ReconcileSortKey(const winrt::TableViewColumn& column)
{
    // Only a sort this table wrote to the view for the column is keyed on it.
    if (!IsInstalledSortKeyFor(column))
    {
        return;
    }

    auto const key = EffectiveSortKey(column);
    if (IsSameSortKey(key, m_installedSortKey->MemberPath, m_installedSortKey->Comparer))
    {
        ForgetDeclinedSortKeys(column);
        return;
    }
    if (IsSortKeyStillDeclined(column))
    {
        return;
    }

    if (!CanSortColumn(column))
    {
        SortByColumnInternal(column, winrt::SortDirection::None, false, false, true /* resort */);
        return;
    }

    // One Sorting/Sorted pair in the current direction; a cancelled Sorting keeps the current order.
    SortByColumnInternal(column, column.SortDirection(), true, true, true /* resort */);
}

// The key the handler left: a later change to the same key is not a new request.
void TableView::RecordDeclinedSortKey(const winrt::TableViewColumn& column)
{
    if (!column || column.SortDirection() == winrt::SortDirection::None)
    {
        return;
    }
    auto key = EffectiveSortKey(column);
    ForgetDeclinedSortKeys(column);
    DeclinedSortKey declined{ winrt::make_weak(column), std::move(key.MemberPath) };
    declined.Generation = winrt::get_self<TableViewColumn>(column)->SortKeyGenerationInternal();
    if (key.Comparer)
    {
        try
        {
            declined.Comparer = winrt::make_weak(key.Comparer);
        }
        catch (...)
        {
            declined.StrongComparer = key.Comparer;
        }
        declined.HasComparer = true;
    }
    m_declinedSortKeys.push_back(std::move(declined));
}

bool TableView::IsSortKeyStillDeclined(const winrt::TableViewColumn& column)
{
    auto const it = std::find_if(m_declinedSortKeys.begin(), m_declinedSortKeys.end(),
        [&column](auto const& declined) { return declined.Column.get() == column; });
    if (it == m_declinedSortKeys.end())
    {
        return false;
    }

    auto const key = EffectiveSortKey(column);
    const bool stillDeclined =
        it->Generation == winrt::get_self<TableViewColumn>(column)->SortKeyGenerationInternal() &&
        (it->HasComparer
            ? key.Comparer && key.Comparer == (it->StrongComparer ? it->StrongComparer : it->Comparer.get())
            : !key.Comparer && key.MemberPath == it->MemberPath);
    if (!stillDeclined)
    {
        m_declinedSortKeys.erase(it);
    }
    return stillDeclined;
}

// Null forgets every column's.
void TableView::ForgetDeclinedSortKeys(const winrt::TableViewColumn& column)
{
    if (!column)
    {
        m_declinedSortKeys.clear();
        return;
    }
    m_declinedSortKeys.erase(std::remove_if(m_declinedSortKeys.begin(), m_declinedSortKeys.end(),
        [&column](auto const& declined)
        {
            auto const declinedColumn = declined.Column.get();
            return !declinedColumn || declinedColumn == column;
        }), m_declinedSortKeys.end());
}

void TableView::RefreshHeaderSortability(const winrt::TableViewColumn& column)
{
    // The chevron and the click handler are stamped at header-build time.
    auto const host = m_headerHost.get();
    if (!host || !column)
    {
        return;
    }

    for (auto const& child : host.Children())
    {
        auto const header = child.try_as<winrt::TableViewColumnHeader>();
        auto const headerImpl = header ? winrt::get_self<TableViewColumnHeader>(header) : nullptr;
        if (headerImpl && headerImpl->Column() == column)
        {
            if (headerImpl->IsSortableInternal() != IsColumnUserSortableInternal(column))
            {
                QueueRebuildHeaders();
            }
            return;
        }
    }
}

void TableView::OnColumnSortKeyChanged(const winrt::TableViewColumn& column)
{
    // Queued first so it still runs when a Sorting or Sorted handler throws.
    RefreshHeaderSortability(column);

    // Checked on every change, also a deferred one: moving off the declined key ends the decline.
    if (column)
    {
        IsSortKeyStillDeclined(column);
    }

    if (!column || column.SortDirection() == winrt::SortDirection::None)
    {
        return;
    }

    // A posted reconcile covers it too.
    if (m_sortTransitionDepth || m_sortKeyReconcileQueued || m_reconcilingSortKeys)
    {
        m_sortKeysDirty = true;
        return;
    }

    ReconcileSortKey(column);
}

bool TableView::SortByColumnInternal(
    const winrt::TableViewColumn& column, winrt::SortDirection direction, bool measureTip, bool requireCanSort, bool resort)
{
    return RunSortTransition([&]()
    {
        // Only a clear may skip the sort-key check.
        requireCanSort = requireCanSort || direction != winrt::SortDirection::None;
        if (!CanSortColumnNow(column, requireCanSort))
        {
            return false;
        }

        if (direction == winrt::SortDirection::None && m_sortedColumns.empty())
        {
            return false;
        }

        // A direction the app only displayed has not ordered the rows yet.
        if (m_sortedColumns.size() == 1 && !m_sortDisplaySetByApp && !resort)
        {
            if (auto sortedColumn = m_sortedColumns.front().get())
            {
                if (sortedColumn == column && sortedColumn.SortDirection() == direction)
                {
                    // Re-sorts in place on a key the view's sort was not written with, e.g. a declined one.
                    if (!IsInstalledSortKeyFor(column) ||
                        IsSameSortKey(EffectiveSortKey(column), m_installedSortKey->MemberPath, m_installedSortKey->Comparer))
                    {
                        return false;
                    }
                    resort = true;
                }
            }
        }

        if (RaiseSortingAndCheckCanceled(column, direction, resort))
        {
            return false;
        }

        // A view that cannot take the sort through ICollectionViewShaping owns its order: the Sorting handler
        // is where the app sorts it.
        if (m_rowFlattener && !CanSortThroughCollectionView(column, direction))
        {
            return false;
        }

        // The Sorting handler is app code that can change the rows.
        if (!IsSortRequestStillValid(column, requireCanSort))
        {
            return false;
        }

        return RecomputeSortDPsAndRaiseInternal(column, measureTip, false, direction);
    });
}

bool TableView::ToggleSortDirection(const winrt::TableViewColumn& column)
{
    return RunSortTransition([&]() { return ToggleSortDirectionCore(column); });
}

bool TableView::ToggleSortDirectionCore(const winrt::TableViewColumn& column)
{
    if (!CanSortColumnNow(column))
    {
        return false;
    }

    // The column's SortCycle names both halves of the policy: which direction an unsorted column
    // opens in, and whether a trailing unsorted step exists. Read from the column being clicked,
    // so columns in one table can carry different cycle policies.
    //
    // Opening Ascending:  None -> Ascending -> Descending -> (None | Ascending)
    // Opening Descending: None -> Descending -> Ascending -> (None | Descending)
    const auto cycle = column.SortCycle();
    const bool opensDescending =
        cycle == winrt::TableViewSortCycle::DescendingAscending ||
        cycle == winrt::TableViewSortCycle::DescendingAscendingNone;
    const bool cyclesToUnsorted =
        cycle == winrt::TableViewSortCycle::AscendingDescendingNone ||
        cycle == winrt::TableViewSortCycle::DescendingAscendingNone;

    const auto opening = opensDescending
        ? winrt::SortDirection::Descending
        : winrt::SortDirection::Ascending;
    const auto second = opensDescending
        ? winrt::SortDirection::Ascending
        : winrt::SortDirection::Descending;

    // Compared against the cycle's own directions rather than switching on Ascending/Descending,
    // so the same walk serves both opening directions.
    const auto current = column.SortDirection();
    winrt::SortDirection next;
    if (current == opening)
    {
        next = second;
    }
    else if (current == second)
    {
        next = cyclesToUnsorted ? winrt::SortDirection::None : opening;
    }
    else
    {
        next = opening;
    }

    if (RaiseSortingAndCheckCanceled(column, next))
    {
        return false;
    }

    if (m_rowFlattener && !CanSortThroughCollectionView(column, next))
    {
        return false;
    }

    if (!IsSortRequestStillValid(column))
    {
        return false;
    }

    return RecomputeSortDPsAndRaiseInternal(column, true, false, next);
}

bool TableView::ClearSort()
{
    return ClearSortInternal(true, true);
}

bool TableView::IsSortLitOnlyByApp() const
{
    return m_sortDisplaySetByApp;
}

bool TableView::ClearSortInternal(bool measureTip, bool explicitClear)
{
    return RunSortTransition([&]() { return ClearSortCore(measureTip, explicitClear); });
}

bool TableView::ClearSortCore(bool measureTip, bool explicitClear)
{
    CancelPendingSortRestore();
    if (!IsSortClearStillValid())
    {
        return false;
    }

    if (!explicitClear && IsSortLitOnlyByApp())
    {
        return false;
    }

    if (RaiseSortingAndCheckCanceled(nullptr, winrt::SortDirection::None))
    {
        return false;
    }

    if (!IsSortClearStillValid())
    {
        return false;
    }

    {
        auto const update = BeginSortStateUpdate();
        auto const lit = SnapshotSortedColumns();
        m_sortedColumns.clear();
        m_sortDisplaySetByApp = false;
        for (auto const& column : lit)
        {
            winrt::get_self<TableViewColumn>(column)->SetSortStateInternal(winrt::SortDirection::None);
        }
    }
    ForgetDeclinedSortKeys();

    // Null trigger: Sorted fires with Column=null to signal a full clear.
    RecomputeSortDPsAndRaiseInternal(nullptr, measureTip, explicitClear);
    return true;
}

void TableView::ResetSortStateForNewItemsSource()
{
    // A new data set invalidates the sort, and the projection the sort was applied to is discarded
    // with it. Left alone, the control keeps each column reporting a SortDirection and drawing the
    // chevron for an order the new rows are not actually in. WPF DataGrid does exactly this in
    // ClearSortDescriptionsOnItemsSourceChange, down to clearing each column's SortDirection.
    //
    // Silent, unlike ClearSort: there is no reshape to perform because the source is being rebuilt
    // from scratch, and the data set is already gone by the time a handler could react, so this is
    // not a cancellable Sorting nor a Sorted the app could act on.
    CancelPendingSortRestore();
    ForgetDeclinedSortKeys();
    // The installed key belongs to the view being discarded.
    m_installedSortKey.reset();
    if (m_sortedColumns.empty())
    {
        m_sortDisplaySetByApp = false;
        return;
    }

    auto const update = BeginSortStateUpdate();
    auto const lit = SnapshotSortedColumns();
    m_sortedColumns.clear();
    m_sortDisplaySetByApp = false;
    for (auto const& column : lit)
    {
        winrt::get_self<TableViewColumn>(column)->SetSortStateInternal(winrt::SortDirection::None);
    }
}

// Display only: never reshapes. Single-column, so lighting one column darkens every other.
void TableView::OnColumnSortDirectionSetByApp(const winrt::TableViewColumn& column, winrt::SortDirection direction)
{
    // A write from an app callback inside one of the control's own updates is applied once that
    // update has finished, so the update never iterates state the write changed.
    if (m_sortStateUpdateDepth > 0)
    {
        m_deferredAppSortWrites.push_back(winrt::make_weak(column));
        return;
    }

    if (direction == winrt::SortDirection::None)
    {
        m_sortedColumns.erase(
            std::remove_if(m_sortedColumns.begin(), m_sortedColumns.end(),
                [&column](auto const& weakColumn) { auto const c = weakColumn.get(); return !c || c == column; }),
            m_sortedColumns.end());
        if (m_sortedColumns.empty())
        {
            m_sortDisplaySetByApp = false;
        }
        return;
    }

    std::vector<winrt::TableViewColumn> toDarken;
    for (auto const& weakColumn : m_sortedColumns)
    {
        if (auto const sortedColumn = weakColumn.get(); sortedColumn && sortedColumn != column)
        {
            toDarken.push_back(sortedColumn);
        }
    }
    if (auto const columns = Columns())
    {
        for (auto const& other : columns)
        {
            if (other && other != column && other.SortDirection() != winrt::SortDirection::None &&
                std::find(toDarken.begin(), toDarken.end(), other) == toDarken.end())
            {
                toDarken.push_back(other);
            }
        }
    }

    m_sortedColumns.clear();
    m_sortedColumns.push_back(tracker_ref<winrt::TableViewColumn>{ this, column });
    m_sortDisplaySetByApp = true;

    auto const update = BeginSortStateUpdate();
    for (auto const& other : toDarken)
    {
        winrt::get_self<TableViewColumn>(other)->SetSortStateInternal(winrt::SortDirection::None);
    }
}

std::vector<winrt::TableViewColumn> TableView::SnapshotSortedColumns() const
{
    std::vector<winrt::TableViewColumn> snapshot;
    for (auto const& weakColumn : m_sortedColumns)
    {
        if (auto const column = weakColumn.get())
        {
            snapshot.push_back(column);
        }
    }
    return snapshot;
}

void TableView::EndSortStateUpdate() noexcept
{
    if (--m_sortStateUpdateDepth > 0)
    {
        return;
    }
    try
    {
        while (!m_deferredAppSortWrites.empty())
        {
            auto const column = m_deferredAppSortWrites.front().get();
            m_deferredAppSortWrites.erase(m_deferredAppSortWrites.begin());
            if (column && winrt::get_self<TableViewColumn>(column)->GetOwningTableView() == *this)
            {
                OnColumnSortDirectionSetByApp(column, column.SortDirection());
            }
        }
    }
    catch (...)
    {
        m_deferredAppSortWrites.clear();
        TVDiag::LogRetailF(L"[TableView] Applying a deferred SortDirection write failed.");
    }
}

// v1 is single-column sort, so applying a direction always clears every other column first.
void TableView::ApplySingleColumnSortState(const winrt::TableViewColumn& column, winrt::SortDirection direction)
{
    CancelPendingSortRestore();
    ForgetDeclinedSortKeys();
    auto const update = BeginSortStateUpdate();
    auto const lit = SnapshotSortedColumns();
    m_sortedColumns.clear();
    m_sortDisplaySetByApp = false;
    if (direction != winrt::SortDirection::None)
    {
        m_sortedColumns.push_back(tracker_ref<winrt::TableViewColumn>{ this, column });
    }

    for (auto const& sortedColumn : lit)
    {
        if (sortedColumn != column)
        {
            winrt::get_self<TableViewColumn>(sortedColumn)->SetSortStateInternal(winrt::SortDirection::None);
        }
    }
    winrt::get_self<TableViewColumn>(column)->SetSortStateInternal(direction);
}

bool TableView::RaiseSortingAndCheckCanceled(
    const winrt::TableViewColumn& trigger,
    winrt::SortDirection direction,
    bool offersKey)
{
    if (!m_sortingEventSource)
    {
        return false;
    }

    // The keys this Sorting is about, to tell a key the handler changed from one deferred earlier.
    std::vector<std::pair<winrt::TableViewColumn, SortKey>> keysBefore;
    if (trigger)
    {
        keysBefore.emplace_back(trigger, EffectiveSortKey(trigger));
    }
    else
    {
        for (auto const& column : SnapshotSortedColumns())
        {
            keysBefore.emplace_back(column, EffectiveSortKey(column));
        }
    }

    auto args = winrt::make_self<TableViewSortingEventArgs>(trigger, direction);
    try { m_sortingEventSource(*this, *args); }
    catch (...) { TableViewTip::RethrowAppFault(TableViewTip::AppFaultKind::Callback); }

    const bool canceled = args->Cancel();
    if (canceled)
    {
        // At any depth: the handler's own key change must not re-sort what it just cancelled.
        // Any other pending change stays dirty for the reconcile.
        for (auto const& [column, before] : keysBefore)
        {
            if ((offersKey && column == trigger) ||
                !IsSameSortKey(EffectiveSortKey(column), before.MemberPath, before.Comparer))
            {
                RecordDeclinedSortKey(column);
            }
        }
    }
    return canceled;
}

bool TableView::CanSortThroughCollectionView(const winrt::TableViewColumn& column, winrt::SortDirection direction) const
{
    // A SortMemberPath becomes a SortDescription; a CustomSortComparer becomes the view's CustomSort.
    return m_collectionViewShaping && m_collectionViewShaping.CanSort() && (direction == winrt::SortDirection::None || CanSortColumn(column));
}

bool TableView::SyncCollectionViewSort(const winrt::TableViewColumn& trigger, winrt::SortDirection direction, bool explicitClear)
{
    if (!m_collectionViewShaping || !m_collectionViewShaping.CanSort())
    {
        return false;
    }

    // An implicit clear (a column leaving) removes only a sort this table wrote, not the app's.
    if (!trigger && !explicitClear && !m_collectionViewSortByTable)
    {
        return false;
    }

    namespace data = winrt::Microsoft::UI::Xaml::Data;
    data::SortDescription description{ nullptr };
    data::CollectionViewComparer customSort{ nullptr };
    winrt::ITableViewSortComparer installedComparer{ nullptr };
    auto const faults = std::make_shared<TableViewTip::ContainedFaults>();
    if (trigger && direction != winrt::SortDirection::None)
    {
        const bool descending = direction == winrt::SortDirection::Descending;
        if (auto const comparer = trigger.CustomSortComparer())
        {
            installedComparer = comparer;
            customSort = data::CollectionViewComparer{ [comparer, descending, faults](winrt::IInspectable const& x, winrt::IInspectable const& y) -> int32_t
            {
                try
                {
                    const int32_t result = comparer.Compare(x, y);
                    const int32_t sign = (result > 0) - (result < 0);
                    return descending ? -sign : sign;
                }
                catch (...)
                {
                    if (faults->count++ == 0)
                    {
                        faults->firstError = winrt::to_hresult();
                    }
                    throw;
                }
            } };
        }
        else
        {
            winrt::hstring sortMemberPath;
            try { sortMemberPath = trigger.GetSortMemberPathCore(); }
            catch (...)
            {
                if (TableViewDetails::IsAppDerivedColumn(trigger)) { TableViewTip::RethrowAppFault(TableViewTip::AppFaultKind::Callback); }
                throw;
            }
            if (sortMemberPath.empty())
            {
                return false;
            }
            description = data::SortDescription{ sortMemberPath,
                descending ? data::CollectionViewSortDirection::Descending : data::CollectionViewSortDirection::Ascending };
        }
    }

    // Last writer wins, as with the control's own sort: the column's sort becomes the view's only sort.
    // The view reshapes in place, once for the whole change, and raises Reset, which the projection
    // over it follows.
    auto const sortDescriptions = m_collectionViewShaping.SortDescriptions();
    auto const previousCustomSort = m_collectionViewShaping.CustomSort();
    std::vector<data::SortDescription> previousDescriptions;
    for (auto const& previous : sortDescriptions)
    {
        previousDescriptions.push_back(previous);
    }

    m_updatingCollectionViewSort = true;
    auto const resetUpdating = wil::scope_exit([this]() noexcept { m_updatingCollectionViewSort = false; });
    try
    {
        auto const deferral = m_collectionViewShaping.DeferRefresh();
        auto closeOnThrow = wil::scope_exit([&deferral]() noexcept { try { deferral.Close(); } catch (...) {} });
        if (customSort)
        {
            // Replaces the SortDescriptions.
            m_collectionViewShaping.CustomSort(customSort);
        }
        else
        {
            if (!description)
            {
                sortDescriptions.Clear();
            }
            else if (sortDescriptions.Size() == 1)
            {
                sortDescriptions.SetAt(0, description);
            }
            else
            {
                sortDescriptions.Clear();
                sortDescriptions.Append(description);
            }
            m_collectionViewShaping.CustomSort(nullptr);
        }
        closeOnThrow.release();
        deferral.Close();
    }
    catch (...)
    {
        if (!faults->count)
        {
            throw;
        }

        // A comparer that threw leaves the view's rows as they were; put its sort back too, so the
        // previous sort and arrow stay, as on any other source.
        try
        {
            auto const deferral = m_collectionViewShaping.DeferRefresh();
            sortDescriptions.Clear();
            for (auto const& previous : previousDescriptions)
            {
                sortDescriptions.Append(previous);
            }
            m_collectionViewShaping.CustomSort(previousCustomSort);
            deferral.Close();
        }
        catch (...)
        {
            TVDiag::LogRetailF(L"[TableView] Restoring the view's sort after a comparer fault failed.");
        }
        m_sortAbortedByComparerFault = TableViewTip::ContainedFaults{ faults->count, faults->firstError };
        return false;
    }

    m_collectionViewCustomSort = customSort;
    m_collectionViewSortByTable = static_cast<bool>(customSort) || static_cast<bool>(description);
    if (m_collectionViewSortByTable)
    {
        // The key the view now sorts with: a later change to the column's key is reconciled against it.
        m_installedSortKey = InstalledSortKey{ winrt::make_weak(trigger),
            description ? description.PropertyName() : winrt::hstring{}, installedComparer,
            winrt::get_self<TableViewColumn>(trigger)->SortKeyGenerationInternal() };
    }
    else
    {
        m_installedSortKey.reset();
    }
    RefreshRowsPipeline();
    UpdateEmptyState();
    OnRowsReorderedBySort();
    return true;
}
// Lights the column whose sort path is the view's primary SortDescription, for a sort the app (or
// another consumer of the view) applied. Raises Sorted when the lit column or direction changes.
void TableView::ReconcileSortStateWithSource()
{
    if (!m_collectionViewShaping || m_sortDisplaySetByApp)
    {
        return;
    }

    namespace data = winrt::Microsoft::UI::Xaml::Data;
    winrt::TableViewColumn matchedColumn{ nullptr };
    auto direction = winrt::SortDirection::None;
    auto const sortDescriptions = m_collectionViewShaping.SortDescriptions();
    if (sortDescriptions.Size() == 0 && m_collectionViewCustomSort && m_collectionViewShaping.CustomSort() == m_collectionViewCustomSort)
    {
        // The view still sorts with the comparer this table installed; its column stays lit.
        return;
    }
    if (sortDescriptions.Size() > 0)
    {
        if (auto const primary = sortDescriptions.GetAt(0))
        {
            auto const path = primary.PropertyName();
            if (auto const columns = Columns(); columns && !path.empty())
            {
                for (auto const& column : columns)
                {
                    if (column && column.GetSortMemberPathCore() == path)
                    {
                        matchedColumn = column;
                        direction = primary.Direction() == data::CollectionViewSortDirection::Descending
                            ? winrt::SortDirection::Descending : winrt::SortDirection::Ascending;
                        break;
                    }
                }
            }
        }
    }

    if (m_sortedColumns.size() == (matchedColumn ? 1u : 0u))
    {
        winrt::TableViewColumn lit{ nullptr };
        if (!m_sortedColumns.empty())
        {
            lit = m_sortedColumns.front().get();
        }
        if (lit == matchedColumn && (!lit || lit.SortDirection() == direction))
        {
            return;
        }
    }

    CancelPendingSortRestore();
    {
        auto const update = BeginSortStateUpdate();
        auto const lit = SnapshotSortedColumns();
        m_sortedColumns.clear();
        for (auto const& column : lit)
        {
            if (column != matchedColumn)
            {
                winrt::get_self<TableViewColumn>(column)->SetSortStateInternal(winrt::SortDirection::None);
            }
        }
        if (matchedColumn)
        {
            winrt::get_self<TableViewColumn>(matchedColumn)->SetSortStateInternal(direction);
            m_sortedColumns.push_back(tracker_ref<winrt::TableViewColumn>{ this, matchedColumn });
        }
    }

    if (m_sortedEventSource)
    {
        auto args = winrt::make_self<TableViewSortedEventArgs>(matchedColumn, direction);
        m_sortedEventSource(*this, *args);
    }
}

bool TableView::PurgeColumnFromSortState(const winrt::TableViewColumn& removedColumn)
{
    // Clear the leaving column's own SortDirection DP explicitly. The walk below drops it from
    // m_sortedColumns, but the DP on the instance is sticky - a consumer who kept a strong
    // reference and re-attached the column to another TableView would carry a phantom sort state
    // across with it.
    if (removedColumn != nullptr)
    {
        auto const update = BeginSortStateUpdate();
        winrt::get_self<TableViewColumn>(removedColumn)->SetSortStateInternal(winrt::SortDirection::None);
        ForgetDeclinedSortKeys(removedColumn);
    }

    bool removedAny = false;
    auto it = m_sortedColumns.begin();
    while (it != m_sortedColumns.end())
    {
        auto const column = it->get();
        const bool isDead = (column == nullptr);
        const bool isRemoved = (removedColumn != nullptr && column == removedColumn);
        if (isDead || isRemoved)
        {
            it = m_sortedColumns.erase(it);
            removedAny = true;
        }
        else
        {
            ++it;
        }
    }
    if (m_sortedColumns.empty())
    {
        m_sortDisplaySetByApp = false;
    }

    return removedAny;
}

int32_t TableView::FindEntryIndexForDataItem(const winrt::IInspectable& item) const
{
    if (!item || !m_rowsItemsSourceView)
    {
        return -1;
    }

    // Identity, not equality: the projection holds the app's own objects, and a value-based match
    // would re-select the wrong row whenever two rows compare equal.
    auto const target = item.try_as<::IUnknown>();
    if (!target)
    {
        return -1;
    }

    const auto count = m_rowsItemsSourceView.Count();
    for (int32_t i = 0; i < count; ++i)
    {
        if (auto const candidate = m_rowsItemsSourceView.GetAt(i).try_as<::IUnknown>();
            candidate && candidate.get() == target.get())
        {
            return i;
        }
    }

    return -1;
}

bool TableView::RecomputeSortDPsAndRaiseInternal(
    const winrt::TableViewColumn& trigger, bool measureTip, bool explicitClear, std::optional<winrt::SortDirection> applyDirection)
{
    // Stale-deferred-clear guard. A null trigger means "clear", and every legitimate null-trigger
    // caller - ClearSort, SortByColumn(col, None), ToggleSortDirection cycling to None - clears
    // m_sortedColumns BEFORE calling this. So a null trigger arriving with a non-empty
    // m_sortedColumns can only be a clear that was queued earlier and overtaken by a newer sort;
    // running it would tear down the sort the app just asked for. If a future edit adds a
    // null-trigger path that does not clear first, this guard will silently swallow a legitimate
    // clear - update the invariant before adding one.
    if (!trigger && !m_sortedColumns.empty())
    {
        return false;
    }

    const auto invalidatedSort = !measureTip && m_tip.operation == TableViewTip::Operation::Sort
        ? m_tip.operationGeneration : 0;
    auto const tipGeneration = BeginTipOperation(TableViewTip::Operation::Sort, measureTip);
    m_controlSortPublishedGeneration = 0;
    bool completed = false;
    std::optional<TableViewTip::Fault> failure;
    auto tipCompletion = wil::scope_exit([this, tipGeneration, measureTip, &completed, &failure]() noexcept
    {
        if (measureTip && !completed)
        {
            FailTipOperation(tipGeneration, TableViewTip::Stage::Sort, failure);
        }
        else if (measureTip && m_controlSortPublishedGeneration == tipGeneration)
        {
            CompleteTipOperation(tipGeneration);
        }
        else if (measureTip)
        {
            TableViewTip::IgnoreOperation(m_tip, tipGeneration, TableViewTip::IgnoreReason::DeferredOrUnproven);
        }
        EndTipOperation();
    });

    try
    {
        // Sorts the view. A cancelled Sorting never reaches this point, so anything that does gets both
        // the sort and the published state.
        const auto requestedDirection = applyDirection ? *applyDirection
            : trigger ? trigger.SortDirection() : winrt::SortDirection::None;

        // Selection is index-based, and a re-sort moves every index. Capture the item, then re-find it
        // afterwards so the selection follows the row rather than the slot.
        const auto preservedSelection = SelectedItemInternal();
        m_sortAbortedByComparerFault.reset();
        const bool reshaped = SyncCollectionViewSort(trigger, requestedDirection, explicitClear || static_cast<bool>(trigger));
        if (reshaped)
        {
            // The view reordered the rows synchronously: the sort is published.
            m_controlSortPublishedGeneration = tipGeneration;
        }
        if (auto const aborted = std::exchange(m_sortAbortedByComparerFault, std::nullopt))
        {
            TableViewTelemetry::ReportError(m_telemetrySession, 0, TableViewTip::Operation::Sort, TableViewTip::Stage::Sort,
                true, TableViewTip::Attribution::Application, aborted->firstError, TableViewTip::AppFaultKind::Callback,
                aborted->count);
            if (measureTip)
            {
                TableViewTip::IgnoreOperation(m_tip, tipGeneration, TableViewTip::IgnoreReason::ApplicationFault,
                    aborted->firstError, TableViewTip::AppFaultKind::Callback);
            }
            completed = true;
            return false;
        }
        // The arrow moves only once the source accepted the sort.
        if (applyDirection && trigger)
        {
            ApplySingleColumnSortState(trigger, *applyDirection);
        }
        if (reshaped && invalidatedSort)
        {
            TableViewTip::IgnoreOperation(m_tip, invalidatedSort, TableViewTip::IgnoreReason::Stale);
        }
        if (!reshaped && tipGeneration == m_tip.operationGeneration)
        {
            TableViewTip::IgnoreOperation(m_tip, tipGeneration, TableViewTip::IgnoreReason::Stale);
        }

        if (reshaped && preservedSelection)
        {
            // A row that fell out of the projection surfaces as a real deselect, which is the honest
            // answer - the selected item is no longer displayed.
            ApplySelection(FindEntryIndexForDataItem(preservedSelection));
        }

        // The app's sort remains on the source, so an implicit clear is not reported as a clear.
        bool keptSourceSort = false;
        if (!trigger && !explicitClear && m_collectionViewShaping)
        {
            keptSourceSort = m_collectionViewShaping.SortDescriptions().Size() > 0 || m_collectionViewShaping.CustomSort();
        }

        if (m_sortedEventSource && !keptSourceSort)
        {
            // The requested direction: an app write made from a SortDirection callback must not change what is reported.
            const auto appliedDirection = trigger ? requestedDirection : winrt::SortDirection::None;
            auto args = winrt::make_self<TableViewSortedEventArgs>(trigger, appliedDirection);
            try
            {
                m_sortedEventSource(*this, *args);
            }
            catch (...)
            {
                TableViewTip::RethrowAppFault(TableViewTip::AppFaultKind::Callback);
            }
        }

        if (!keptSourceSort && winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::Notification))
        {
            // A programmatic or header-driven re-sort has no input event behind it, so without an
            // announcement a screen-reader user has no way to learn the order changed.
            if (trigger)
            {
                // The realized header peer also names template headers, which carry no string.
                winrt::hstring header;
                if (auto const headerPeer = GetRealizedColumnHeaderPeer(*this, trigger))
                {
                    header = headerPeer.GetName();
                }
                if (header.empty())
                {
                    header = TableViewDetails::GetColumnHeaderText(trigger);
                }

                switch (requestedDirection)
                {
                case winrt::SortDirection::Ascending:
                    AnnounceSortChange(StringUtil::FormatString(
                        LocalizedOrFallback(SR_TableViewSortedAscending, L"Sorted by %1!s! ascending."), header.c_str()));
                    break;
                case winrt::SortDirection::Descending:
                    AnnounceSortChange(StringUtil::FormatString(
                        LocalizedOrFallback(SR_TableViewSortedDescending, L"Sorted by %1!s! descending."), header.c_str()));
                    break;
                case winrt::SortDirection::None:
                default:
                    AnnounceSortChange(StringUtil::FormatString(
                        LocalizedOrFallback(SR_TableViewSortCleared, L"Sorting cleared for %1!s!."), header.c_str()));
                    break;
                }
            }
            else
            {
                AnnounceSortChange(LocalizedOrFallback(SR_TableViewSortClearedAll, L"All sorting cleared."));
            }
        }
    }
    catch (...)
    {
        // App code's exceptions (the collection, its contract, a sort callout) carry their kind.
        failure = TableViewTip::CurrentFault();
        throw;
    }

    // The chevrons are already current: SetSortStateInternal republished them through
    // RefreshSortIndicators as each column's DP was written.
    completed = true;

    // An app-declared axis that survived the clear re-lights its column (for example a moved one).
    if (!trigger && !explicitClear)
    {
        QueueReconcileSortStateWithSource();
    }
    return true;
}

void TableView::QueueReconcileSortStateWithSource()
{
    // Deferred, never inline: the notification that brings us here is raised from inside the
    // view's own reshape, and reconciling can write the view's sort again.
    if (m_sortReconcileQueued)
    {
        return;
    }
    m_sortReconcileQueued = true;

    auto weakThis = get_weak();
    if (auto const queue = winrt::DispatcherQueue::GetForCurrentThread())
    {
        queue.TryEnqueue([weakThis]()
        {
            if (auto strongThis = weakThis.get())
            {
                // An opt-out replayed in the meantime may have reconciled already.
                if (std::exchange(strongThis->m_sortReconcileQueued, false))
                {
                    strongThis->ReconcileSortStateWithSource();
                }
            }
        });
    }
    else
    {
        m_sortReconcileQueued = false;
    }
}

void TableView::QueueClearSortAfterColumnRemoval(){
    if (m_clearSortAfterColumnRemovalQueued)
    {
        return;
    }
    m_clearSortAfterColumnRemovalQueued = true;

    auto weakThis = get_weak();
    if (auto const queue = winrt::DispatcherQueue::GetForCurrentThread())
    {
        queue.TryEnqueue([weakThis]()
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->m_clearSortAfterColumnRemovalQueued = false;
                strongThis->m_sortedColumnPendingRemoval = nullptr;
                // RecomputeSortDPsAndRaiseInternal's stale-clear guard drops this if the app
                // applied a new sort on a different column in the meantime.
                strongThis->RecomputeSortDPsAndRaiseInternal(nullptr, false);
            }
        });
    }
    else
    {
        m_clearSortAfterColumnRemovalQueued = false;
        m_sortedColumnPendingRemoval = nullptr;
    }
}

void TableView::PurgeLeavingColumnFromSortState(const winrt::TableViewColumn& column)
{
    const bool wasSorted = std::any_of(m_sortedColumns.begin(), m_sortedColumns.end(),
        [&column](auto const& sorted) { return sorted.get() == column; });
    const auto direction = column.SortDirection();
    auto const declined = std::find_if(m_declinedSortKeys.begin(), m_declinedSortKeys.end(),
        [&column](auto const& entry) { return entry.Column.get() == column; });
    std::optional<DeclinedSortKey> pendingDeclined;
    if (declined != m_declinedSortKeys.end())
    {
        pendingDeclined = *declined;
    }
    const bool wasAppLit = wasSorted && m_sortDisplaySetByApp;
    if (!PurgeColumnFromSortState(column))
    {
        return;
    }

    if (wasSorted && direction != winrt::SortDirection::None)
    {
        m_sortedColumnPendingRemoval = winrt::make_weak(column);
        m_sortedColumnPendingRemovalDirection = direction;
        m_sortedColumnPendingRemovalDeclined = std::move(pendingDeclined);
        m_sortedColumnPendingRemovalAppLit = wasAppLit;
    }
    QueueClearSortAfterColumnRemoval();
}

void TableView::RestoreSortOfMovedColumn(const winrt::TableViewColumn& column)
{
    if (!m_clearSortAfterColumnRemovalQueued || !column || m_sortedColumnPendingRemoval.get() != column)
    {
        return;
    }
    m_sortedColumnPendingRemoval = nullptr;
    auto declined = std::exchange(m_sortedColumnPendingRemovalDeclined, std::nullopt);

    // A sort applied since the removal replaced this one. A column that lost its sort key while
    // detached never told the table; one lit only by the app keeps it, as when attached.
    const bool wasAppLit = std::exchange(m_sortedColumnPendingRemovalAppLit, false);
    if (!m_sortedColumns.empty() || (!CanSortColumn(column) && !wasAppLit))
    {
        return;
    }

    winrt::get_self<TableViewColumn>(column)->SetSortStateInternal(m_sortedColumnPendingRemovalDirection);
    m_sortedColumns.push_back(tracker_ref<winrt::TableViewColumn>{ this, column });
    // The table's own sort stays the table's (AdoptColumnSortDirection skips a column already lit), and a
    // direction the app only displayed stays display only.
    m_sortDisplaySetByApp = wasAppLit;
    if (declined)
    {
        m_declinedSortKeys.push_back(std::move(*declined));
    }

    // A key change while detached reached no table: apply it on the next turn, unless still declined.
    if (!IsInstalledSortKeyFor(column) || IsSortKeyStillDeclined(column))
    {
        return;
    }
    if (winrt::get_self<TableViewColumn>(column)->SortKeyGenerationInternal() != m_installedSortKey->Generation ||
        !IsSameSortKey(EffectiveSortKey(column), m_installedSortKey->MemberPath, m_installedSortKey->Comparer))
    {
        m_sortKeysDirty = true;
        ScheduleSortKeyReconcile(false);
    }
}

// A notification rather than a live region: there is no announcement-only element in the template.
void TableView::AnnounceSortChange(const winrt::hstring& announcement)
{
    RaiseTableViewNotification(*this, winrt::AutomationNotificationKind::ActionCompleted, [&announcement]() { return announcement; }, L"TableViewSortChanged");
}
