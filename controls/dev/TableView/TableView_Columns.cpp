// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewColumn.h"
#include "TableViewColumnHeader.h"
#include "TableViewRow.h"
#include "TableViewCellsPanel.h"
#include "TableViewToolTipHelpers.h"
#include "TVDiag.h"

#include <algorithm>

// Column DP wiring, per-column updates, and frozen-column layout live here.
// The column-width layout engine (Pixel/Auto/Star resolution) lives in TableView_Layout.cpp.

namespace
{
    bool IsColumnOwnedBy(winrt::TableView const& owner, winrt::TableViewColumn const& column)
    {
        return column && winrt::get_self<TableViewColumn>(column)->GetOwningTableView() == owner;
    }

    bool TrySetColumnOwnerForTracking(winrt::TableView const& owner, winrt::TableViewColumn const& column)
    {
        if (!column)
        {
            return false;
        }

        if (winrt::get_self<TableViewColumn>(column)->SetOwningTableViewInternal(owner))
        {
            return true;
        }

        TVDiag::LogRetailF(L"[TableView] A column already owned by another TableView was ignored.");
        return false;
    }
}

// Leading-frozen columns must be a contiguous prefix so the pinned band aligns.
double TableView::ComputeLeadingFrozenWidth()
{
    double width = 0.0;
    if (auto columns = Columns())
    {
        // Only a contiguous leading prefix (from column 0) is frozen. A Leading flag on a
        // non-prefix column is ignored so it cannot corrupt the pinned-band layout.
        for (auto const& column : columns)
        {
            if (!IsColumnOwnedBy(*this, column))
            {
                continue;
            }
            if (column.FrozenEdge() != winrt::TableViewFrozenEdge::Leading)
            {
                break;
            }
            width += column.Visibility() == winrt::Visibility::Visible ? std::max(0.0, column.ActualWidth()) : 0.0;
        }
    }
    return width;
}

void TableView::RefreshFrozenColumns()
{
    const double leadingFrozenWidth = ComputeLeadingFrozenWidth();

    // No active frozen columns; avoid walking realized rows on every scroll.
    if (leadingFrozenWidth <= 0.0 && !m_frozenColumnsActive)
    {
        return;
    }

    // Run once after deactivation to clear prior transforms and clips.
    m_frozenColumnsActive = leadingFrozenWidth > 0.0;

    double horizontalOffset = 0.0;
    if (auto bodyScroller = m_bodyScroller.get())
    {
        horizontalOffset = bodyScroller.HorizontalOffset();
    }

    // Header and row cells share pinning so they stay aligned.
    if (auto headerHost = m_headerHost.get())
    {
        TableViewCellsPanel::ApplyFrozenColumnLayout(headerHost, horizontalOffset, leadingFrozenWidth);
    }

    // Re-pin realized rows.
    ForEachRealizedRow([&](winrt::TableViewRow const& row)
    {
        winrt::get_self<TableViewRow>(row)->RefreshFrozenColumnLayout(horizontalOffset, leadingFrozenWidth);
    });
}

void TableView::PinFrozenColumnsForRow(const winrt::TableViewRow& row)
{
    if (!row)
    {
        return;
    }

    const double leadingFrozenWidth = ComputeLeadingFrozenWidth();
    if (leadingFrozenWidth <= 0.0)
    {
        return;
    }

    double horizontalOffset = 0.0;
    if (auto bodyScroller = m_bodyScroller.get())
    {
        horizontalOffset = bodyScroller.HorizontalOffset();
    }

    // The row owns its cell panel; ask it to pin its own cells (the same path RefreshFrozenColumns
    // uses) instead of having TableView reach into and lay out the row's panel.
    winrt::get_self<TableViewRow>(row)->RefreshFrozenColumnLayout(horizontalOffset, leadingFrozenWidth);
}

void TableView::DetachAllColumnOwners()
{
    bool purgedSortState = false;
    for (auto const& weakCol : m_trackedColumns)
    {
        if (auto col = weakCol.get())
        {
            winrt::get_self<TableViewColumn>(col)->SetOwningTableViewInternal(nullptr);
            purgedSortState |= PurgeColumnFromSortState(col);
        }
    }
    m_trackedColumns.clear();

    if (purgedSortState)
    {
        QueueClearSortAfterColumnRemoval();
    }
}

void TableView::TrackColumnsFromVector(winrt::IObservableVector<winrt::TableViewColumn> const& columns)
{
    if (!columns)
    {
        return;
    }

    const uint32_t size = columns.Size();
    m_trackedColumns.reserve(size);
    for (uint32_t i = 0; i < size; ++i)
    {
        auto col = columns.GetAt(i);
        const bool ownsColumn = TrySetColumnOwnerForTracking(*this, col);
        m_trackedColumns.push_back(tracker_ref<winrt::TableViewColumn>{ this, ownsColumn ? col : nullptr });
        if (ownsColumn)
        {
            AdoptColumnSortDirection(col);
        }
    }
}

// A column that arrives already showing a direction is lit as if the app had just set it; the
// last one to arrive wins, as with consecutive writes.
void TableView::AdoptColumnSortDirection(winrt::TableViewColumn const& column)
{
    if (auto const direction = column.SortDirection(); direction != winrt::SortDirection::None)
    {
        OnColumnSortDirectionSetByApp(column, direction);
    }
}

void TableView::OnColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    // Avoid rehooking the same vector.
    if (args.OldValue() == args.NewValue())
    {
        return;
    }

    // Detach the old vector so replaced Columns do not leak subscriptions or owners.
    if (m_columnsVectorChangedToken.value)
    {
        if (auto oldColumns = args.OldValue().try_as<winrt::IObservableVector<winrt::TableViewColumn>>())
        {
            oldColumns.VectorChanged(m_columnsVectorChangedToken);
        }
        m_columnsVectorChangedToken = {};
    }

    // Re-hook observable vectors; Columns is ABI-projected as IVector<>.
    if (auto newColumns = args.NewValue().try_as<winrt::IObservableVector<winrt::TableViewColumn>>())
    {
        auto weakThis = get_weak();
        m_columnsVectorChangedToken = newColumns.VectorChanged(
            [weakThis](winrt::IObservableVector<winrt::TableViewColumn> const& sender, winrt::IVectorChangedEventArgs const& args)
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->OnColumnsVectorChanged(sender, args);
                }
            });
    }

    // Treat replacement as a reset so ownership and headers match the new vector.
    if (auto newColumns = args.NewValue().try_as<winrt::IObservableVector<winrt::TableViewColumn>>())
    {
        // Inline Reset semantics because IVectorChangedEventArgs cannot be synthesized here.
        DetachAllColumnOwners();
        TrackColumnsFromVector(newColumns);
        RebuildHeaders();
        // Columns replaced: recompute Auto widths for the new set and re-pin frozen columns.
        ResetColumnDesiredWidths();
        // Realized rows keep old revokers when the owner is unchanged; re-subscribe them.
        ForEachRealizedRow([](winrt::TableViewRow const& row)
        {
            winrt::get_self<TableViewRow>(row)->RefreshColumnsSubscriptionInternal();
        });
    }
    else
    {
        // Non-observable value: drop column tracking.
        DetachAllColumnOwners();
        RebuildHeaders();
        // Flush realized rows' prior revokers; no non-observable vector is attached.
        ForEachRealizedRow([](winrt::TableViewRow const& row)
        {
            winrt::get_self<TableViewRow>(row)->RefreshColumnsSubscriptionInternal();
        });
    }

    // Replacing the whole vector raises no VectorChanged, so announce the shape change here.
    QueueRaiseColumnsStructureChanged();
}

void TableView::OnColumnsVectorChanged(
    const winrt::IObservableVector<winrt::TableViewColumn>& sender,
    const winrt::IVectorChangedEventArgs& args)
{
    // Track columns incrementally so owner back-pointers stay in sync without diffing.
    const auto change = args.CollectionChange();
    const auto index = args.Index();

    switch (change)
    {
    case winrt::CollectionChange::ItemInserted:
    {
        if (index < sender.Size())
        {
            auto col = sender.GetAt(index);
            bool ownsColumn = false;
            if (col)
            {
                auto colImpl = winrt::get_self<TableViewColumn>(col);
                ownsColumn = colImpl->SetOwningTableViewInternal(*this);
                if (ownsColumn)
                {
                    // A re-inserted / recycled column instance may carry a stale grow-only accumulator
                    // from a prior position or table; clear it so it re-grows from its content here.
                    colImpl->ResetDesiredWidthInternal();
                }
                else
                {
                    TVDiag::LogRetailF(L"[TableView] A column already owned by another TableView was ignored.");
                }
            }
            m_trackedColumns.insert(
                m_trackedColumns.begin() + std::min<size_t>(index, m_trackedColumns.size()),
                tracker_ref<winrt::TableViewColumn>{ this, ownsColumn ? col : nullptr });
            if (ownsColumn)
            {
                AdoptColumnSortDirection(col);
            }
        }
        break;
    }
    case winrt::CollectionChange::ItemRemoved:
    {
        if (index < m_trackedColumns.size())
        {
            if (auto col = m_trackedColumns[index].get())
            {
                winrt::get_self<TableViewColumn>(col)->SetOwningTableViewInternal(nullptr);
                // A column that has left Columns must not stay the active sort. Reshaping here
                // would run inside the VectorChanged callback, so the clear is deferred until the
                // collection has settled.
                if (PurgeColumnFromSortState(col))
                {
                    QueueClearSortAfterColumnRemoval();
                }
            }
            m_trackedColumns.erase(m_trackedColumns.begin() + index);
        }
        break;
    }
    case winrt::CollectionChange::ItemChanged:
    {
        if (index < m_trackedColumns.size())
        {
            auto oldCol = m_trackedColumns[index].get();
            if (oldCol)
            {
                winrt::get_self<TableViewColumn>(oldCol)->SetOwningTableViewInternal(nullptr);
                if (PurgeColumnFromSortState(oldCol))
                {
                    QueueClearSortAfterColumnRemoval();
                }
            }
            if (index < sender.Size())
            {
                auto newCol = sender.GetAt(index);
                bool ownsColumn = false;
                if (newCol)
                {
                    auto newColImpl = winrt::get_self<TableViewColumn>(newCol);
                    ownsColumn = newColImpl->SetOwningTableViewInternal(*this);
                    if (ownsColumn)
                    {
                        // Clear any stale grow-only accumulator the incoming instance carried from
                        // elsewhere, but preserve it when the SAME instance is re-notified in place (a
                        // no-op change must not drop the column's accumulated Auto max).
                        if (newCol != oldCol)
                        {
                            newColImpl->ResetDesiredWidthInternal();
                        }
                    }
                    else
                    {
                        TVDiag::LogRetailF(L"[TableView] A column already owned by another TableView was ignored.");
                    }
                }
                m_trackedColumns[index] = tracker_ref<winrt::TableViewColumn>{ this, ownsColumn ? newCol : nullptr };
                if (ownsColumn)
                {
                    AdoptColumnSortDirection(newCol);
                }
            }
        }
        break;
    }
    case winrt::CollectionChange::Reset:
    default:
    {
        // Reset owner back-pointers against the current vector contents.
        DetachAllColumnOwners();
        TrackColumnsFromVector(sender);
        break;
    }
    }

    QueueRebuildHeaders();

    // QueueRebuildHeaders updates visuals only; UIA clients must be told the grid shape changed.
    QueueRaiseColumnsStructureChanged();

    // Realized rows observe Columns directly; no TableView broadcast is needed.
    if (change == winrt::CollectionChange::Reset)
    {
        // The whole vector was replaced: clear every grow-only accumulator and re-resolve.
        ResetColumnDesiredWidths();
    }
    else
    {
        // An incremental add / remove / change does not alter the row data, so pre-existing columns
        // keep their accumulated Auto max (resetting them would shrink unrelated columns to only the
        // currently realized rows). A newly inserted column starts fresh at 0 and grows from content;
        // re-resolve to pick up the new fixed total and star share.
        InvalidateMeasure();
    }

    // A column that left may be added to another TableView before the queued rebuild runs. Last,
    // because releasing runs app code that can change Columns again.
    if (change != winrt::CollectionChange::ItemInserted)
    {
        ReleaseHeaderCells(m_headerHost.get(), HeaderRelease::DetachedColumns);
    }
}

void TableView::OnColumnVisibilityChanged(const winrt::TableViewColumn& column)
{
    if (!column)
    {
        return;
    }

    const auto visibility = column.Visibility();

    if (auto headerHost = m_headerHost.get())
    {
        if (auto headerCell = TableViewCellsPanel::CellForColumn(headerHost, column))
        {
            // Read before collapsing: a collapsed cell drops focus, leaving it nowhere.
            const int32_t focusIndex = visibility == winrt::Visibility::Collapsed
                ? GetCollapsingHeaderFocusIndex(headerHost, headerCell)
                : -1;

            headerCell.Visibility(visibility);

            if (focusIndex >= 0 && FocusVisibleHeaderFrom(focusIndex, +1) < 0)
            {
                FocusVisibleHeaderFrom(focusIndex - 1, -1);
            }
        }
    }

    // The row owns its cells; ask each to apply the column's visibility instead of reaching
    // through the row into its cell panel and mutating each cell here.
    ForEachRealizedRow([&](winrt::TableViewRow const& row)
    {
        winrt::get_self<TableViewRow>(row)->RefreshColumnVisibility(column, visibility);
    });

    // Visibility can move the terminal column. Evaluate after layout so Auto and Star widths,
    // transforms, and the body viewport are final.
    QueueTerminalGridLineRefresh();

    // Keep a synchronous RefreshFrozenColumns here: collapsing a column changes the leading
    // frozen-band width without changing any surviving column's ActualWidth, so ResolveColumnWidths'
    // `changed` gate stays false and would never re-pin. Refresh it directly.
    InvalidateMeasure();
    RefreshFrozenColumns();
    RefreshResizeGripperAvailability();

    // UIA addresses only visible columns (IsVisibleColumn), so a visibility flip shifts column
    // indices just like an add/remove.
    QueueRaiseColumnsStructureChanged();
}

// The column is already Collapsed here, so its own cell is not counted: the result is the index the
// next visible header takes.
int32_t TableView::GetCollapsingHeaderFocusIndex(const winrt::Panel& host, const winrt::FrameworkElement& headerCell) const
{
    auto const root = XamlRoot();
    if (!host || !headerCell || !root)
    {
        return -1;
    }

    auto current = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>();
    while (current && current != headerCell)
    {
        current = winrt::VisualTreeHelper::GetParent(current);
    }
    if (!current)
    {
        return -1;
    }

    int32_t index = 0;
    for (auto const& child : host.Children())
    {
        if (child == headerCell)
        {
            return index;
        }
        if (auto const element = child.try_as<winrt::FrameworkElement>())
        {
            if (auto const childColumn = element.Tag().try_as<winrt::TableViewColumn>();
                childColumn && childColumn.Visibility() == winrt::Visibility::Visible)
            {
                ++index;
            }
        }
    }
    return -1;
}

void TableView::OnColumnWidthChanged(const winrt::TableViewColumn& column)
{
    if (!column)
    {
        return;
    }

    // A Width-mode, MinWidth, or MaxWidth change alters the fixed total and star factors, but none of
    // them is a data-set change, so do NOT reset the grow-only Auto accumulator (that would let an
    // Auto column shrink to only the currently realized rows). Re-resolve on the next measure pass.
    //
    // For a Pixel column, TableViewColumn::OnPropertyChanged already wrote the final ActualWidth (via
    // UpdateActualWidth) BEFORE this runs, so ResolveColumnWidths computes an unchanged width
    // (changed == false) and skips BOTH its cell-panel re-arrange and its frozen re-pin. Compensate
    // directly: re-arrange the header + row cell panels at the new width and re-pin frozen columns.
    // (For Auto/Star the deferred resolve also does both on the real width; these direct calls are
    // idempotent there.)
    InvalidateMeasure();
    InvalidateCellPanels();
    RefreshFrozenColumns();
    QueueTerminalGridLineRefresh();

    // A drag converts columns to Pixel as it goes; re-evaluating then would pull the gripper out
    // from under the pointer. DragCompleted refreshes instead.
    if (!m_activeColumnResizeDrag)
    {
        RefreshResizeGripperAvailability();
    }
}

void TableView::OnColumnCellTemplateChanged(const winrt::TableViewColumn& column, bool resetAutoWidth)
{
    if (!column)
    {
        return;
    }

    // Regenerating cells inside measure or arrange would mutate the tree being laid out. At most one
    // post per column; the reset requests are OR'ed into it.
    if (IsInLayoutCallback())
    {
        auto* const columnImpl = winrt::get_self<TableViewColumn>(column);
        if (columnImpl->QueueCellContentChangeInternal(resetAutoWidth))
        {
            return;
        }
        if (auto const queue = DispatcherQueue())
        {
            if (queue.TryEnqueue([weakColumn = winrt::make_weak(column)]()
                {
                    auto const liveColumn = weakColumn.get();
                    if (!liveColumn)
                    {
                        return;
                    }
                    // The column may have moved to another table meanwhile; its current owner regenerates.
                    auto* const liveColumnImpl = winrt::get_self<TableViewColumn>(liveColumn);
                    const bool reset = liveColumnImpl->TakeQueuedCellContentChangeInternal();
                    if (auto const owner = liveColumnImpl->GetOwningTableView())
                    {
                        winrt::get_self<TableView>(owner)->OnColumnCellTemplateChanged(liveColumn, reset);
                    }
                }))
            {
                return;
            }
        }
        columnImpl->TakeQueuedCellContentChangeInternal();
    }

    // Rebuild realized rows so the new CellTemplate is applied; virtualized rows pick it up on realization.
    ForEachRealizedRow([](winrt::TableViewRow const& row)
    {
        winrt::get_self<TableViewRow>(row)->RefreshCells();
    });

    // The new template can be narrower or wider than the old cells: recompute only THIS column's Auto
    // content width. Reset only this column's grow-only accumulator -- a global reset would shrink
    // unrelated Auto columns to only the currently-realized rows (the same reason OnColumnsVectorChanged
    // does not reset pre-existing columns).
    if (resetAutoWidth)
    {
        winrt::get_self<TableViewColumn>(column)->ResetDesiredWidthInternal();
    }
    InvalidateMeasure();
}

void TableView::OnColumnHeaderChanged(const winrt::TableViewColumn& column)
{
    // Header content changed: re-render headers, then recompute only the CHANGED column's Auto width
    // (a changed header can be wider or narrower than that column's cells). Reset only this column's
    // grow-only accumulator -- a global reset would shrink UNRELATED Auto columns to only the
    // currently-realized rows. Resetting even when the column is currently Pixel/Star preserves the
    // "switched back to Auto" stale-width safeguard for that one column.
    // An old UIElement Header is the app's again now, as with ContentControl.Content, not after the
    // rebuild. Any other content stays until the rebuild, so the header does not go blank meanwhile.
    auto const headerHost = m_headerHost.get();
    if (auto const headerCell = TableViewCellsPanel::CellForColumn(headerHost, column).try_as<winrt::TableViewColumnHeader>())
    {
        if (headerCell.Content().try_as<winrt::UIElement>()
            && !winrt::get_self<TableViewColumnHeader>(headerCell)->OwnsContentInternal())
        {
            ReleaseHeaderCells(headerHost, HeaderRelease::Column, column);
        }
    }
    QueueRebuildHeaders();
    if (column)
    {
        winrt::get_self<TableViewColumn>(column)->ResetDesiredWidthInternal();
    }
    InvalidateMeasure();
}

void TableView::OnColumnHeaderToolTipChanged(const winrt::TableViewColumn& column)
{
    if (!column)
    {
        return;
    }

    // Only the realized header carries a tooltip; RebuildHeaders reads the current value when it runs.
    auto const host = m_headerHost.get();
    if (!host)
    {
        return;
    }

    if (auto const headerCell = TableViewCellsPanel::CellForColumn(host, column))
    {
        TableViewDetails::ApplyHeaderToolTip(headerCell, column.HeaderToolTip());
    }
}

void TableView::OnColumnFrozenEdgeChanged(const winrt::TableViewColumn& column)
{
    if (!column)
    {
        return;
    }

    // Re-render headers to re-tag cells for the new frozen prefix; RebuildHeaders ends with
    // RefreshFrozenColumns, which re-pins the leading-frozen band on the header and realized rows.
    QueueRebuildHeaders();
}
