// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//
// Single-column sort for TableView.
//
// The control owns the ordering for every source it can display: an ItemsSource that is not
// already a TableViewSource is projected through one, so a header click installs a sort axis on
// that projection and the rows re-project. Sorting is raised first as a cancellable pre-event; an
// app that would rather order its own collection cancels it and owns both the rows and the
// column's sort state from there.
//
// A column supplies its key either as a property path (SortMemberPath, or the cell binding for a
// text column) or as a CustomSortComparer. A comparer cannot be pushed into the projection - the
// projection sorts by key - so comparer columns are ranked up front and the rank becomes the key.

#include "pch.h"
#include "common.h"
#include "GroupedEntry.h"
#include "TableView.h"
#include "TableViewColumn.h"
#include "TableViewSource.h"
#include "TableViewSortingEventArgs.h"
#include "TableViewSortedEventArgs.h"
#include "TableViewAutomationHelpers.h"
#include "ResourceAccessor.h"
#include "Utils.h"
#include "ShapingHelpers.h"
#include "SortMemberPathResolver.h"
#include "TVDiag.h"

#include <algorithm>
#include <numeric>
#include <unordered_map>

void TableViewSourceSortBinding::ResetCustomSort()
{
    if (CustomSortState)
    {
        CustomSortState->Reset();
    }
}

void TableViewSourceSortBinding::Clear()
{
    MemberPath = {};
    AxisToken = {};
    KeySelector = nullptr;
    // Dropped, not reset: the axis left on a source this table no longer owns still ranks through
    // it. The next sort mints a fresh adapter and selector.
    CustomSortState.reset();
}

bool TableView::RepositionCustomSortItem(const winrt::IInspectable& item, bool reshapePending)
{
    auto const& state = m_tableViewSourceSort.CustomSortState;
    if (!state || !state->HasComparer())
    {
        return false;
    }
    if (!item)
    {
        state->InvalidateRanks();
        return false;
    }
    return state->Reposition(item, reshapePending);
}

void TableView::ReleaseCustomSortRows(const winrt::IInspectable& item, const std::vector<winrt::IInspectable>& liveRows)
{
    auto const& state = m_tableViewSourceSort.CustomSortState;
    if (!state || !state->HasComparer())
    {
        return;
    }
    if (item)
    {
        state->Forget(item);
    }
    else
    {
        state->RetainOnly(liveRows);
    }
}

// Rank() starts from the current rows, so it drops ranks rooted while released and re-reads values
// edited meanwhile; re-declaring the axis makes the projection read the new keys.
void TableView::RerankCustomSortAfterReclaim()
{
    auto const& state = m_tableViewSourceSort.CustomSortState;
    if (!state || !state->HasComparer() || m_sortedColumns.size() != 1)
    {
        return;
    }
    auto const column = m_sortedColumns.front().get();
    if (!column || !column.CustomSortComparer())
    {
        return;
    }
    const auto direction = column.SortDirection();
    if (direction == winrt::SortDirection::None || !CanApplySortTo(direction))
    {
        return;
    }

    const auto preservedSelection = SelectedItemInternal();
    try
    {
        // A rerank after reclaim is control-driven, not a user sort, so it carries no TiP operation.
        if (SyncTableViewSourceSort(column, direction, 0) && preservedSelection)
        {
            ApplySelection(FindEntryIndexForDataItem(preservedSelection));
        }
    }
    catch (...)
    {
        // Dispatcher callback: best-effort, the previous order stands.
        TVDiag::LogRetailF(L"[TableView] Re-ranking the custom sort after reclaim failed.");
    }
}

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

    // Evaluates a property path against a row item; see SortMemberPathResolver.h. The same
    // evaluator backs TableViewSource's path-based Sort verb, so a column and a fluent sort on
    // the same path produce the same keys.

    bool HasResolvedSortMemberPath(const winrt::TableViewColumn& column)
    {
        // Dispatch on the projection, not winrt::get_self: GetSortMemberPathCore is `overridable`,
        // so get_self would call the C++ implementation's virtual directly and bypass WinRT
        // composition. A column authored in C# that overrides it to compute a sort path would then
        // be treated as having none, and silently become unsortable.
        return column && !column.GetSortMemberPathCore().empty();
    }

    bool CanSortColumn(const winrt::TableViewColumn& column)
    {
        if (!column || !column.CanSort())
        {
            return false;
        }

        return column.CustomSortComparer() || HasResolvedSortMemberPath(column);
    }

    // Identity-derived so two columns with the same SortMemberPath still own separate axes; falls
    // back to the path only if the column has no usable identity.
    winrt::hstring GetTableViewSourceSortAxisToken(const winrt::TableViewColumn& column, const winrt::hstring& sortMemberPath)
    {
        if (column)
        {
            if (auto unknown = column.try_as<::IUnknown>())
            {
                wchar_t buffer[48];
                swprintf_s(buffer, L"column:%p", unknown.get());
                return winrt::hstring{ buffer };
            }
        }

        return L"path:" + sortMemberPath;
    }
}

winrt::TableViewSource TableView::ShapingSourceInternal() const
{
    // AdoptItemsSource is the single place that decides what the source is; everything else
    // reads the answer here rather than re-deriving it from ItemsSource.
    return m_activeSource.get();
}

bool TableView::IsSortClearStillValid() const
{
    return !m_sortedColumns.empty();
}

// Refused a source another table owns: there is nothing to sort until it is released, and the
// columns keep their state for the reconcile that follows.
bool TableView::CanSortColumnNow(const winrt::TableViewColumn& column, bool requireCanSort) const
{
    if (m_awaitedSource.get())
    {
        return false;
    }
    return requireCanSort ? CanSortColumn(column) : static_cast<bool>(column);
}

bool TableView::CanApplySortTo(winrt::SortDirection direction) const
{
    if (direction == winrt::SortDirection::None)
    {
        return true;
    }
    auto const source = ShapingSourceInternal();
    return !source || winrt::get_self<::TableViewSource>(source)->CanApplySort();
}

bool TableView::IsSortRequestStillValid(const winrt::TableViewColumn& column, bool requireCanSort) const
{
    if (!column || !CanSortColumnNow(column, requireCanSort))
    {
        return false;
    }

    // A clear that skips the CanSort gate clears only the column's own sort. A replayed or
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
    // A column that opted out of sorting can still have the sort it holds cleared.
    const bool clearsOwnSort = direction == winrt::SortDirection::None &&
        column && column.SortDirection() != winrt::SortDirection::None;
    return SortByColumnInternal(column, direction, true, !clearsOwnSort);
}

bool TableView::SortByColumnInternal(
    const winrt::TableViewColumn& column, winrt::SortDirection direction, bool measureTip, bool requireCanSort)
{
    // Only a clear may skip the CanSort gate.
    requireCanSort = requireCanSort || direction != winrt::SortDirection::None;
    if (!CanSortColumnNow(column, requireCanSort))
    {
        return false;
    }

    if (direction == winrt::SortDirection::None && m_sortedColumns.empty())
    {
        return false;
    }

    if (m_sortedColumns.size() == 1)
    {
        if (auto sortedColumn = m_sortedColumns.front().get())
        {
            if (sortedColumn == column && sortedColumn.SortDirection() == direction)
            {
                return false;
            }
        }
    }

    // A null item or one object on two rows can't be sorted; refuse before any state changes.
    if (!CanApplySortTo(direction))
    {
        return false;
    }

    // An open editor is showing a row at an index that is about to move. Close it first; if it
    // cannot close synchronously, replay this request once it does.
    if (!TryTerminateEditForControlInitiatedReshape())
    {
        if (m_editState == EditState::Ending && !m_isApplyingCoalescedEditReshape)
        {
            QueueCoalescedEditReshape([this, column, direction, measureTip, requireCanSort]()
            {
                SortByColumnInternal(column, direction, measureTip, requireCanSort);
            });
        }
        return false;
    }
    if (!IsSortRequestStillValid(column, requireCanSort))
    {
        return false;
    }

    if (RaiseSortingAndCheckCanceled(column, direction))
    {
        return false;
    }

    // The edit commit and the Sorting handler are app code that can change the rows.
    if (!IsSortRequestStillValid(column, requireCanSort) || !CanApplySortTo(direction))
    {
        return false;
    }

    ApplySingleColumnSortState(column, direction);
    RecomputeSortDPsAndRaiseInternal(column, measureTip);
    DrainCoalescedEditReshape();
    return true;
}

bool TableView::ToggleSortDirection(const winrt::TableViewColumn& column)
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

    if (!CanApplySortTo(next))
    {
        return false;
    }

    if (!TryTerminateEditForControlInitiatedReshape())
    {
        if (m_editState == EditState::Ending && !m_isApplyingCoalescedEditReshape)
        {
            QueueCoalescedEditReshape([this, column]()
            {
                ToggleSortDirection(column);
            });
        }
        return false;
    }
    if (!IsSortRequestStillValid(column))
    {
        return false;
    }

    if (RaiseSortingAndCheckCanceled(column, next))
    {
        return false;
    }

    if (!IsSortRequestStillValid(column) || !CanApplySortTo(next))
    {
        return false;
    }

    ApplySingleColumnSortState(column, next);
    RecomputeSortDPsAndRaiseInternal(column);
    DrainCoalescedEditReshape();
    return true;
}

bool TableView::ClearSort()
{
    return ClearSortInternal(true, true);
}

bool TableView::ClearSortInternal(bool measureTip, bool explicitClear)
{
    if (m_sortedColumns.empty())
    {
        return false;
    }

    if (!TryTerminateEditForControlInitiatedReshape())
    {
        if (m_editState == EditState::Ending && !m_isApplyingCoalescedEditReshape)
        {
            QueueCoalescedEditReshape([this, measureTip, explicitClear]()
            {
                ClearSortInternal(measureTip, explicitClear);
            });
        }
        return false;
    }
    if (!IsSortClearStillValid())
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

    for (auto const& weakColumn : m_sortedColumns)
    {
        if (auto column = weakColumn.get())
        {
            winrt::get_self<TableViewColumn>(column)->SetSortStateInternal(winrt::SortDirection::None);
        }
    }
    m_sortedColumns.clear();

    // Null trigger: Sorted fires with Column=null to signal a full clear.
    RecomputeSortDPsAndRaiseInternal(nullptr, measureTip, explicitClear);
    DrainCoalescedEditReshape();
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
    if (m_sortedColumns.empty() && m_tableViewSourceSort.AxisToken.empty())
    {
        return;
    }

    for (auto const& weakColumn : m_sortedColumns)
    {
        if (auto column = weakColumn.get())
        {
            winrt::get_self<TableViewColumn>(column)->SetSortStateInternal(winrt::SortDirection::None);
        }
    }

    m_sortedColumns.clear();

    // The axis token and key selector belong to the projection being discarded, so carrying them
    // into the next one would address an axis that no longer exists.
    m_tableViewSourceSort.Clear();
}

// v1 is single-column sort, so applying a direction always clears every other column first.
void TableView::ApplySingleColumnSortState(const winrt::TableViewColumn& column, winrt::SortDirection direction)
{
    for (auto const& weakColumn : m_sortedColumns)
    {
        if (auto sortedColumn = weakColumn.get(); sortedColumn && sortedColumn != column)
        {
            winrt::get_self<TableViewColumn>(sortedColumn)->SetSortStateInternal(winrt::SortDirection::None);
        }
    }
    m_sortedColumns.clear();

    winrt::get_self<TableViewColumn>(column)->SetSortStateInternal(direction);
    if (direction != winrt::SortDirection::None)
    {
        m_sortedColumns.push_back(tracker_ref<winrt::TableViewColumn>{ this, column });
    }
}

bool TableView::RaiseSortingAndCheckCanceled(
    const winrt::TableViewColumn& trigger,
    winrt::SortDirection direction)
{
    if (!m_sortingEventSource)
    {
        return false;
    }

    auto args = winrt::make_self<TableViewSortingEventArgs>(trigger, direction);
    m_sortingEventSource(*this, *args);

    return args->Cancel();
}

winrt::TableViewKeySelector TableView::GetTableViewSourceSortKeySelector(
    const winrt::hstring& sortMemberPath)
{
    const bool sortMemberPathChanged = m_tableViewSourceSort.MemberPath != sortMemberPath;
    if (!m_tableViewSourceSort.CustomSortState)
    {
        m_tableViewSourceSort.CustomSortState = std::make_shared<ShapingHelpers::CustomSortRankAdapter>();
    }
    m_tableViewSourceSort.MemberPath = sortMemberPath;
    m_tableViewSourceSort.ResetCustomSort();

    // Reused across re-sorts of the same path so the projection is not handed a new delegate
    // identity every time. The selector closes over the state by shared_ptr, which is what lets
    // ResetCustomSort swap the comparer underneath a live selector.
    if (!m_tableViewSourceSort.KeySelector || sortMemberPathChanged)
    {
        auto customSortState = m_tableViewSourceSort.CustomSortState;
        auto sortMemberPathResolver = std::make_shared<SortMemberPathResolver>(sortMemberPath);
        m_tableViewSourceSort.KeySelector = winrt::TableViewKeySelector{
            [sortMemberPathResolver, customSortState](const winrt::IInspectable& item) -> winrt::IInspectable
            {
                if (customSortState && customSortState->HasComparer())
                {
                    return customSortState->KeyFor(item);
                }
                return sortMemberPathResolver->Resolve(item);
            }
        };
    }

    return m_tableViewSourceSort.KeySelector;
}

bool TableView::SyncTableViewSourceSort(
    const winrt::TableViewColumn& trigger,
    winrt::SortDirection direction,
    uint64_t tipGeneration,
    bool explicitClear)
{
    auto tableViewSource = ShapingSourceInternal();
    if (!tableViewSource)
    {
        return false;
    }

    // Released on Unloaded or before the first Loaded. While another table owns the source, this
    // table changes only its own state; a path axis left behind re-lights the column on reclaim.
    if (m_activeSourceReleased && winrt::get_self<::TableViewSource>(tableViewSource)->IsOwnedByOther(*this))
    {
        if (direction == winrt::SortDirection::None)
        {
            m_tableViewSourceSort.Clear();
        }
        return false;
    }

    winrt::TableViewKeySelector keySelector{ nullptr };
    winrt::hstring sortAxisToken;
    // What this axis sorts on, purely descriptive. Stays empty for a CustomSortComparer: the
    // ordering is the comparer's, not the path's, so nothing else may claim the axis sorts by a
    // property.
    winrt::hstring axisSortMemberPath;
    // A clear removes the installed axis by its token, so it must not depend on the trigger's sort
    // path, which a Sorting handler may have changed or emptied since the axis was installed.
    if (trigger && direction != winrt::SortDirection::None)
    {
        if (const auto customComparer = trigger.CustomSortComparer())
        {
            keySelector = GetTableViewSourceSortKeySelector(L"");
            sortAxisToken = GetTableViewSourceSortAxisToken(trigger, L"");

            if (direction != winrt::SortDirection::None)
            {
                auto rowsView = m_rowsItemsSourceView;
                if (!rowsView)
                {
                    rowsView = winrt::get_self<::TableViewSource>(tableViewSource)->GetItemsSourceView();
                }

                // Snapshot the rows for the engine, which ranks over a plain vector and never sees
                // an ItemsSourceView. The comparer is wrapped into a neutral pairwise functor; the
                // engine catches a throw and degrades it to "equal", so this lambda stays trivial.
                std::vector<winrt::IInspectable> rows;
                if (rowsView)
                {
                    const auto count = rowsView.Count();
                    rows.reserve(count > 0 ? static_cast<size_t>(count) : 0);
                    for (int32_t i = 0; i < count; ++i)
                    {
                        auto const row = rowsView.GetAt(i);
                        // In a grouped projection the headers are the only synthesized rows; data
                        // rows are the app's own items. Rank the data items only - a header is not
                        // something the app's comparer has ever seen.
                        if (row && !TryGetGroupedEntry(row))
                        {
                            rows.push_back(row);
                        }
                    }
                }

                if (auto const& rankAdapter = m_tableViewSourceSort.CustomSortState)
                {
                    ShapingHelpers::PairwiseComparer comparer =
                        [customComparer](winrt::IInspectable const& a, winrt::IInspectable const& b)
                        {
                            return static_cast<int>(customComparer.Compare(a, b));
                        };
                    rankAdapter->Rank(comparer, rows);
                }
            }
        }
        else
        {
            // Projected dispatch - see the note in HasResolvedSortMemberPath.
            AppCalloutTip callout{ *this, TableViewTip::Stage::Sort, TableViewTip::AppFaultKind::Callback };
            winrt::hstring sortMemberPath;
            try { sortMemberPath = trigger.GetSortMemberPathCore(); }
            catch (...) { callout.Fault(winrt::to_hresult()); throw; }
            if (sortMemberPath.empty())
            {
                return false;
            }
            keySelector = GetTableViewSourceSortKeySelector(sortMemberPath);
            sortAxisToken = GetTableViewSourceSortAxisToken(trigger, sortMemberPath);
            axisSortMemberPath = sortMemberPath;
        }
    }
    else
    {
        // A clear: reuse whatever axis is currently installed so it can be removed by token.
        keySelector = m_tableViewSourceSort.KeySelector;
        sortAxisToken = m_tableViewSourceSort.AxisToken;
        if (direction == winrt::SortDirection::None)
        {
            m_tableViewSourceSort.ResetCustomSort();
        }
    }

    // Limitation: this reshapes on explicit sort and on collection notifications only.
    // INotifyPropertyChanged-only mutations of the active sort key do not live re-sort until the
    // next collection change.
    auto tableViewSourceImpl = winrt::get_self<::TableViewSource>(tableViewSource);
    // An explicit clear needs no key selector: an app-declared axis lit on a column was never
    // installed by this control, yet clearing that column must still remove it. Any other clear
    // removes only the axis this control installed.
    const bool clearsAppAxes = direction == winrt::SortDirection::None && explicitClear;
    if (clearsAppAxes
        ? !keySelector && tableViewSourceImpl->ActiveSortAxisInfos().empty()
        : !keySelector || (direction == winrt::SortDirection::None && sortAxisToken.empty()))
    {
        return false;
    }
    // The control is the last writer here, so its axis becomes the only one. Without this an
    // app-declared fluent Sort axis would survive alongside it and, being declared earlier, would
    // outrank it as the primary sort while the chevron advertised this column.
    auto const reconcileGuard = BeginControlInitiatedSortScope(tipGeneration);
    if (clearsAppAxes)
    {
        // An explicit clear means "nothing is sorted", so it clears EVERY axis, not just the one
        // this control installed. Clearing only its own token would leave an app-declared fluent
        // axis ordering the rows while every chevron said the sort was off.
        tableViewSourceImpl->ClearSort();
    }
    else if (direction == winrt::SortDirection::None)
    {
        tableViewSourceImpl->ClearSort(sortAxisToken);
    }
    else
    {
        // Replacing rather than adding: re-sorting a column must swap its axis, not stack a
        // second one on top of the first.
        try
        {
            tableViewSourceImpl->SortReplacing(m_tableViewSourceSort.AxisToken, sortAxisToken, keySelector, axisSortMemberPath, direction);
        }
        catch (...)
        {
            // The axis is declared before the projection fails; keep owning it so the next sort
            // replaces it instead of stacking on an orphan.
            m_tableViewSourceSort.AxisToken = sortAxisToken;
            throw;
        }
        tableViewSourceImpl->ClearSortsExcept(sortAxisToken);
    }

    if (direction == winrt::SortDirection::None)
    {
        m_tableViewSourceSort.Clear();
    }
    else
    {
        m_tableViewSourceSort.AxisToken = sortAxisToken;
    }

    // The verb mutated the active source in place, so the source lifetime is unchanged - just push
    // its rewritten projection into the repeater.
    RefreshRowsPipeline();
    UpdateEmptyState();
    return true;
}

bool TableView::PurgeColumnFromSortState(const winrt::TableViewColumn& removedColumn)
{
    // Clear the leaving column's own SortDirection DP explicitly. The walk below drops it from
    // m_sortedColumns, but the DP on the instance is sticky - a consumer who kept a strong
    // reference and re-attached the column to another TableView would carry a phantom sort state
    // across with it.
    if (removedColumn != nullptr)
    {
        winrt::get_self<TableViewColumn>(removedColumn)->SetSortStateInternal(winrt::SortDirection::None);
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

void TableView::RecomputeSortDPsAndRaiseInternal(
    const winrt::TableViewColumn& trigger, bool measureTip, bool explicitClear)
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
        return;
    }

    const auto invalidatedSort = !measureTip && m_tip.operation == TableViewTip::Operation::Sort
        ? m_tip.operationGeneration : 0;
    auto const tipGeneration = BeginTipOperation(TableViewTip::Operation::Sort, measureTip);
    m_controlSortPublishedGeneration = 0;
    bool completed = false;
    auto tipCompletion = wil::scope_exit([this, tipGeneration, measureTip, &completed]() noexcept
    {
        if (measureTip && !completed)
        {
            FailTipOperation(tipGeneration, TableViewTip::Stage::Sort, {});
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

    // Reshapes through whichever source is active. A cancelled Sorting never reaches this point,
    // so anything that does gets both the reshape and the published state.
    const auto requestedDirection = trigger ? trigger.SortDirection() : winrt::SortDirection::None;

    // Selection is index-based, and a re-sort moves every index. Capture the item, then re-find it
    // afterwards so the selection follows the row rather than the slot.
    const auto preservedSelection = SelectedItemInternal();
    const bool reshaped = SyncTableViewSourceSort(trigger, requestedDirection, tipGeneration, explicitClear || static_cast<bool>(trigger));
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

    // An implicit clear that left the app's sort on the source is not a clear: the rows keep their
    // order and the reconcile below re-lights the column.
    bool keptSourceSort = false;
    if (!trigger && !explicitClear && !reshaped && !m_activeSourceReleased)
    {
        if (auto const source = ShapingSourceInternal())
        {
            keptSourceSort = !winrt::get_self<::TableViewSource>(source)->ActiveSortAxisInfos().empty();
        }
    }

    if (m_sortedEventSource && !keptSourceSort)
    {
        const auto appliedDirection = trigger ? trigger.SortDirection() : winrt::SortDirection::None;
        auto args = winrt::make_self<TableViewSortedEventArgs>(trigger, appliedDirection);
        AppCalloutTip callout{ *this, TableViewTip::Stage::Sort, TableViewTip::AppFaultKind::Callback };
        try
        {
            m_sortedEventSource(*this, *args);
        }
        catch (...)
        {
            callout.Fault(winrt::to_hresult());
            throw;
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

            switch (trigger.SortDirection())
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

    // The chevrons are already current: SetSortStateInternal republished them through
    // RefreshSortIndicators as each column's DP was written.
    completed = true;

    // An app-declared axis that survived the clear re-lights its column (for example a moved one).
    if (!trigger && !explicitClear)
    {
        QueueReconcileSortStateWithSource();
    }
}

void TableView::QueueReconcileSortStateWithSource()
{
    // Deferred, never inline: the notification that brings us here is raised from inside the
    // engine's own reshape, and reconciling clears an axis, which reshapes again.
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
                strongThis->m_sortReconcileQueued = false;
                strongThis->ReconcileSortStateWithSource();
            }
        });
    }
    else
    {
        m_sortReconcileQueued = false;
    }
}

void TableView::ReconcileSortStateWithSource()
{
    auto tableViewSource = ShapingSourceInternal();
    // Released on Unloaded: another table may own it now, and Loaded reconciles on reclaim.
    if (!tableViewSource || m_activeSourceReleased)
    {
        return;
    }
    const bool rerankCustomSort = std::exchange(m_rerankCustomSortOnReconcile, false);

    auto const& ownToken = m_tableViewSourceSort.AxisToken;
    auto tableViewSourceImpl = winrt::get_self<::TableViewSource>(tableViewSource);
    auto axes = tableViewSourceImpl->ActiveSortAxisInfos();

    // A comparer axis another table (or this one, before an ItemsSource change) installed ranks
    // through an adapter nothing here can update, so it would order rows on stale ranks.
    bool clearedForeignComparerAxis = false;
    for (auto const& axis : axes)
    {
        if (axis.AxisToken != ownToken && axis.SortMemberPath.empty() &&
            std::wstring_view{ axis.AxisToken }.starts_with(L"column:"))
        {
            auto const reconcileGuard = BeginControlInitiatedSortScope();
            tableViewSourceImpl->ClearSort(axis.AxisToken);
            clearedForeignComparerAxis = true;
        }
    }
    if (clearedForeignComparerAxis)
    {
        axes = tableViewSourceImpl->ActiveSortAxisInfos();
    }

    bool ownAxisPresent = false;
    // Infos arrive in precedence order, so the first foreign axis is the one that outranks ours
    // and is therefore the only one a single-column chevron could honestly describe.
    ::TableViewSource::ActiveSortAxisInfo const* primaryForeignAxis = nullptr;
    for (auto const& axis : axes)
    {
        if (!ownToken.empty() && axis.AxisToken == ownToken)
        {
            ownAxisPresent = true;
        }
        else if (primaryForeignAxis == nullptr)
        {
            primaryForeignAxis = &axis;
        }
    }

    if (ownAxisPresent && primaryForeignAxis == nullptr)
    {
        // The control's axis is still the only sort: chevrons already describe it.
        if (rerankCustomSort)
        {
            RerankCustomSortAfterReclaim();
        }
        return;
    }

    if (primaryForeignAxis == nullptr && ownToken.empty() && m_sortedColumns.empty())
    {
        // Nothing sorted anywhere and no chevron lit. Reached on every shaping notification -
        // a filter or group change also lands here - so it must not raise a spurious Sorted.
        return;
    }

    if (primaryForeignAxis != nullptr && ownAxisPresent)
    {
        // The app declared a sort while ours was installed. Last writer wins, so ours goes: left
        // alone it would keep whichever axis was declared FIRST as the primary sort, which is the
        // hidden-precedence case this reconciliation exists to prevent.
        auto const reconcileGuard = BeginControlInitiatedSortScope();
        tableViewSourceImpl->ClearSort(ownToken);
    }

    // A path-declared axis names the property it sorts on, so the chevron can follow the app's own
    // sort instead of going dark. A key-selector axis names nothing, and guessing which column it
    // meant would be worse than showing no chevron at all.
    winrt::TableViewColumn matchedColumn{ nullptr };
    if (primaryForeignAxis != nullptr && !primaryForeignAxis->SortMemberPath.empty())
    {
        if (auto const columns = Columns())
        {
            for (auto const& column : columns)
            {
                if (column && column.GetSortMemberPathCore() == primaryForeignAxis->SortMemberPath)
                {
                    matchedColumn = column;
                    break;
                }
            }
        }
    }

    // matchedColumn can only be set from a non-null primaryForeignAxis, but that coupling is not
    // something static analysis can prove at each use. Reading the direction once here keeps the
    // dereference next to the null check it depends on.
    auto const foreignDirection = primaryForeignAxis != nullptr
        ? primaryForeignAxis->Direction
        : winrt::SortDirection::None;

    // Already reconciled to exactly this state. Reached whenever an unrelated shaping change - a
    // filter or a group - renotifies while an app-owned sort is standing; re-raising Sorted for it
    // would report a sort change that did not happen.
    if (ownToken.empty() && matchedColumn && m_sortedColumns.size() == 1)
    {
        if (auto const lit = m_sortedColumns[0].get();
            lit == matchedColumn && lit.SortDirection() == foreignDirection)
        {
            return;
        }
    }

    // Silent, like ResetSortStateForNewItemsSource: the app already owns the ordering by the time
    // we get here, so there is nothing to cancel and no reshape of our own to perform. Sorted is
    // still raised so an app tracking sort state sees the chevrons move.
    for (auto const& weakColumn : m_sortedColumns)
    {
        if (auto column = weakColumn.get(); column && column != matchedColumn)
        {
            winrt::get_self<TableViewColumn>(column)->SetSortStateInternal(winrt::SortDirection::None);
        }
    }
    m_sortedColumns.clear();

    // m_tableViewSourceSort stays cleared even when a column is lit: the axis belongs to the app,
    // and recording it as ours would make the next control-initiated sort try to replace an axis
    // it never installed.
    m_tableViewSourceSort.Clear();

    if (matchedColumn)
    {
        winrt::get_self<TableViewColumn>(matchedColumn)->SetSortStateInternal(foreignDirection);
        m_sortedColumns.push_back(tracker_ref<winrt::TableViewColumn>{ this, matchedColumn });
    }

    // Raised directly rather than through RecomputeSortDPsAndRaiseInternal: that path would reshape
    // through SyncTableViewSourceSort and re-declare the axis as the control's own, undoing the
    // app's ownership - and its stale-clear guard would swallow the event outright now that
    // m_sortedColumns can be non-empty here.
    if (m_sortedEventSource)
    {
        auto args = winrt::make_self<TableViewSortedEventArgs>(
            matchedColumn,
            matchedColumn ? foreignDirection : winrt::SortDirection::None);
        m_sortedEventSource(*this, *args);
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
                // RecomputeSortDPsAndRaiseInternal's stale-clear guard drops this if the app
                // applied a new sort on a different column in the meantime.
                strongThis->RecomputeSortDPsAndRaiseInternal(nullptr, false);
            }
        });
    }
    else
    {
        m_clearSortAfterColumnRemovalQueued = false;
    }
}

void TableView::AnnounceSortChange(const winrt::hstring& announcement){
    if (announcement.empty())
    {
        return;
    }

    if (!winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::Notification))
    {
        return;
    }

    try
    {
        // RaiseNotificationEvent rather than a live region: there is no announcement-only element
        // in the template, and a notification carries its own text without one.
        if (auto const peer = winrt::FrameworkElementAutomationPeer::FromElement(*this))
        {
            peer.RaiseNotificationEvent(
                winrt::AutomationNotificationKind::ActionCompleted,
                winrt::AutomationNotificationProcessing::MostRecent,
                announcement,
                L"TableViewSortChanged");
        }
    }
    catch (...)
    {
        // No peer, or UIA is unavailable on this host: the sort itself already applied.
    }
}
