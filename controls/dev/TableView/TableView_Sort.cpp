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
#include "ResourceAccessor.h"
#include "Utils.h"
#include "ShapingHelpers.h"

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
    ResetCustomSort();
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

    // Evaluates a property path against a row item. A one-time {Binding} on a throwaway
    // ContentControl is used rather than reflection because it is the same evaluator the cells use,
    // so a path that displays also sorts - including indexers and dotted paths.
    struct SortMemberPathResolver
    {
        explicit SortMemberPathResolver(const winrt::hstring& sortMemberPath) :
            SortMemberPath(sortMemberPath)
        {
        }

        winrt::IInspectable Resolve(const winrt::IInspectable& item)
        {
            if (!item || SortMemberPath.empty())
            {
                return nullptr;
            }

            try
            {
                EnsureBinding();
                Probe.DataContext(item);
                auto clearDataContext = wil::scope_exit([this]() noexcept
                {
                    ClearDataContext();
                });

                return Probe.Content();
            }
            catch (...)
            {
                ClearDataContext();
                return nullptr;
            }
        }

    private:
        void EnsureBinding()
        {
            if (!Probe)
            {
                Probe = winrt::ContentControl{};
            }

            if (BoundPath == SortMemberPath)
            {
                return;
            }

            ClearDataContext();
            Probe.ClearValue(winrt::ContentControl::ContentProperty());

            winrt::Binding binding;
            binding.Path(winrt::PropertyPath{ SortMemberPath });
            binding.Mode(winrt::BindingMode::OneTime);
            winrt::BindingOperations::SetBinding(
                Probe,
                winrt::ContentControl::ContentProperty(),
                binding);
            BoundPath = SortMemberPath;
        }

        // Never hold the last row item alive through the probe once the key has been read.
        void ClearDataContext() noexcept
        {
            try
            {
                if (Probe)
                {
                    Probe.DataContext(nullptr);
                }
            }
            catch (...)
            {
            }
        }

        winrt::hstring SortMemberPath;
        winrt::hstring BoundPath;
        winrt::ContentControl Probe{ nullptr };
    };

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

bool TableView::IsSortRequestStillValid(const winrt::TableViewColumn& column) const
{
    if (!column || !CanSortColumn(column))
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
    if (!CanSortColumn(column))
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

    // An open editor is showing a row at an index that is about to move. Close it first; if it
    // cannot close synchronously, replay this request once it does.
    if (!TryTerminateEditForControlInitiatedReshape())
    {
        if (m_editState == EditState::Ending && !m_isApplyingCoalescedEditReshape)
        {
            QueueCoalescedEditReshape([this, column, direction]()
            {
                SortByColumn(column, direction);
            });
        }
        return false;
    }
    if (!IsSortRequestStillValid(column))
    {
        return false;
    }

    if (RaiseSortingAndCheckCanceled(column, direction))
    {
        return false;
    }

    if (!IsSortRequestStillValid(column))
    {
        return false;
    }

    ApplySingleColumnSortState(column, direction);
    RecomputeSortDPsAndRaiseInternal(column);
    DrainCoalescedEditReshape();
    return true;
}

bool TableView::ToggleSortDirection(const winrt::TableViewColumn& column)
{
    if (!CanSortColumn(column))
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

    if (!IsSortRequestStillValid(column))
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
    if (m_sortedColumns.empty())
    {
        return false;
    }

    if (!TryTerminateEditForControlInitiatedReshape())
    {
        if (m_editState == EditState::Ending && !m_isApplyingCoalescedEditReshape)
        {
            QueueCoalescedEditReshape([this]()
            {
                ClearSort();
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
    RecomputeSortDPsAndRaiseInternal(nullptr);
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
    try
    {
        m_sortingEventSource(*this, *args);
    }
    catch (...)
    {
    }

    return args->Cancel();
}

winrt::TableViewKeySelector TableView::GetTableViewSourceSortKeySelector(
    const winrt::hstring& sortMemberPath)
{
    const bool sortMemberPathChanged = m_tableViewSourceSort.MemberPath != sortMemberPath;
    if (!m_tableViewSourceSort.CustomSortState)
    {
        m_tableViewSourceSort.CustomSortState = std::make_shared<TabularShapingHelpers::CustomSortRankAdapter>();
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
    winrt::SortDirection direction)
{
    auto tableViewSource = ShapingSourceInternal();
    if (!tableViewSource)
    {
        return false;
    }

    winrt::TableViewKeySelector keySelector{ nullptr };
    winrt::hstring sortAxisToken;
    if (trigger)
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
                    TabularShapingHelpers::TabularPairwiseComparer comparer =
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
            const auto sortMemberPath = trigger.GetSortMemberPathCore();
            if (sortMemberPath.empty())
            {
                return false;
            }
            keySelector = GetTableViewSourceSortKeySelector(sortMemberPath);
            sortAxisToken = GetTableViewSourceSortAxisToken(trigger, sortMemberPath);
        }
    }
    else
    {
        // Clear-all: reuse whatever axis is currently installed so it can be removed by token.
        keySelector = m_tableViewSourceSort.KeySelector;
        sortAxisToken = m_tableViewSourceSort.AxisToken;
        if (direction == winrt::SortDirection::None)
        {
            m_tableViewSourceSort.ResetCustomSort();
        }
    }

    if (!keySelector)
    {
        return false;
    }

    // Limitation: this reshapes on explicit sort and on collection notifications only.
    // INotifyPropertyChanged-only mutations of the active sort key do not live re-sort until the
    // next collection change.
    auto tableViewSourceImpl = winrt::get_self<::TableViewSource>(tableViewSource);
    if (direction == winrt::SortDirection::None && !sortAxisToken.empty())
    {
        tableViewSourceImpl->ClearSort(sortAxisToken);
    }
    else
    {
        // Replacing rather than adding: re-sorting a column must swap its axis, not stack a
        // second one on top of the first.
        tableViewSourceImpl->SortReplacing(m_tableViewSourceSort.AxisToken, sortAxisToken, keySelector, direction);
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
    const winrt::TableViewColumn& trigger)
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

    // Reshapes through whichever source is active. A cancelled Sorting never reaches this point,
    // so anything that does gets both the reshape and the published state.
    const auto requestedDirection = trigger ? trigger.SortDirection() : winrt::SortDirection::None;

    // Selection is index-based, and a re-sort moves every index. Capture the item, then re-find it
    // afterwards so the selection follows the row rather than the slot.
    const auto preservedSelection = SelectedItemInternal();
    const bool reshaped = SyncTableViewSourceSort(trigger, requestedDirection);

    if (reshaped && preservedSelection)
    {
        // A row that fell out of the projection surfaces as a real deselect, which is the honest
        // answer - the selected item is no longer displayed.
        ApplySelection(FindEntryIndexForDataItem(preservedSelection));
    }

    if (m_sortedEventSource)
    {
        const auto appliedDirection = trigger ? trigger.SortDirection() : winrt::SortDirection::None;
        auto args = winrt::make_self<TableViewSortedEventArgs>(trigger, appliedDirection);
        try
        {
            m_sortedEventSource(*this, *args);
        }
        catch (...)
        {
        }
    }

    // A programmatic or header-driven re-sort has no input event behind it, so without an
    // announcement a screen-reader user has no way to learn the order changed.
    if (trigger)
    {
        winrt::hstring header;
        if (auto const headerContent = trigger.Header())
        {
            if (auto const headerString = headerContent.try_as<winrt::IPropertyValue>();
                headerString && headerString.Type() == winrt::PropertyType::String)
            {
                header = headerString.GetString();
            }
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

    // The chevrons are already current: SetSortStateInternal republished them through
    // RefreshSortIndicators as each column's DP was written.
}

void TableView::QueueClearSortAfterColumnRemoval()
{
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
                strongThis->RecomputeSortDPsAndRaiseInternal(nullptr);
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
