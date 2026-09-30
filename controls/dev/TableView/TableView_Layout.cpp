// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewColumn.h"
#include "TableViewRow.h"
#include "TableViewGroupHeader.h"

#include <algorithm>
#include <cmath>
#include <vector>

// -----------------------------------------------------------------------------
// Column-width layout engine (all of it lives here).
//
// TableView owns the column-width policy; rows and headers are thin plumbing. Each cell host panel
// measures its Auto-column children unconstrained and caches those measured widths by column;
// Pixel/Star cells are measured once at their resolved width (no unconstrained probe), mirroring
// CGrid. After the template subtree measures, TableView::MeasureOverride pulls those cached values
// from the header host and realized rows once, then resolves Pixel / Auto / Star widths into
// TableViewColumn.ActualWidth.
//
// Only realized rows contribute, so Auto sizes to the widest *realized* cell. Auto is shrink-capable:
// each measure pass re-derives the width from the currently pulled measured max (header + realized
// rows), so a column narrows when its widest content shrinks (CGrid parity) rather than latching a
// grow-only maximum. Because only realized rows contribute under virtualization, the Auto width still
// reflects the widest realized cell and can change as rows scroll into / out of realization.
//
// Resolved widths (Pixel / Auto / Star) are snapped to device pixels when UseLayoutRounding is set,
// using the XamlRoot rasterization scale -- again mirroring CGrid.
// -----------------------------------------------------------------------------

namespace
{
    double MinWidthForStarFactor(winrt::TableViewColumn const& column, double factor)
    {
        if (factor > 0.0)
        {
            return column.MinWidth();
        }

        // WPF gives 0* a zero share. Preserve that for the default MinWidth, but still honor an
        // explicitly-set MinWidth on a 0* column.
        const auto localMinWidth = column.ReadLocalValue(winrt::TableViewColumn::MinWidthProperty());
        return localMinWidth == winrt::DependencyProperty::UnsetValue() ? 0.0 : column.MinWidth();
    }

    // Divides `available` in proportion to each column's Star factor. A column that would clamp at
    // its Min/MaxWidth is fixed there and dropped, then the rest re-divide what is left (the WPF
    // ComputeStarColumnWidths shape).
    template <typename TFactor, typename TRound, typename TResolve>
    void DistributeStarWidths(
        std::vector<winrt::TableViewColumn> pool,
        double available,
        TFactor&& factorOf,
        TRound&& layoutRound,
        TResolve&& resolve)
    {
        auto totalFactor = [&]()
        {
            double total = 0.0;
            for (auto const& c : pool)
            {
                total += factorOf(c);
            }
            return total;
        };

        for (bool adjusted = true; adjusted && !pool.empty(); )
        {
            adjusted = false;

            const double factorSum = totalFactor();
            const double unit = factorSum > 0.0 ? available / factorSum : 0.0;

            for (size_t i = 0; i < pool.size(); ++i)
            {
                auto const& c = pool[i];
                const double factor = factorOf(c);
                const double desired = unit * factor;
                const double lo = MinWidthForStarFactor(c, factor);
                const double hi = std::max(lo, c.MaxWidth());
                const double clamped = std::clamp(desired, lo, hi);
                // std::clamp returns desired exactly when it is already in [lo, hi], so any
                // inequality is a real Min/MaxWidth clamp.
                if (clamped != desired)
                {
                    resolve(c, clamped);
                    available -= clamped;
                    pool.erase(pool.begin() + i);
                    adjusted = true;
                    break;
                }
            }
        }

        const double factorSum = totalFactor();
        const double unit = factorSum > 0.0 ? std::max(0.0, available) / factorSum : 0.0;
        // Rounded on the running total rather than per column, so the widths still sum to
        // `available` once snapped; rounding each independently can overshoot the viewport and
        // leave a permanent one-pixel scrollbar.
        double exactConsumed = 0.0;
        double roundedConsumed = 0.0;
        for (auto const& c : pool)
        {
            const double factor = factorOf(c);
            const double lo = MinWidthForStarFactor(c, factor);
            const double hi = std::max(lo, c.MaxWidth());
            exactConsumed += std::clamp(unit * factor, lo, hi);
            const double edge = layoutRound(exactConsumed);
            resolve(c, edge - roundedConsumed);
            roundedConsumed = edge;
        }
    }
}

winrt::Size TableView::MeasureOverride(winrt::Size const& availableSize)
{
    auto desired = __super::MeasureOverride(availableSize);
    ResolveColumnWidths();
    return desired;
}

// Requested from a cell panel's MeasureOverride when a realized cell's own measured width changed
// (grow or shrink). The request comes from below the body ScrollViewer, which absorbs the child's
// measure invalidation, so TableView::MeasureOverride (and ResolveColumnWidths) would not otherwise
// re-run. Invalidate our measure SYNCHRONOUSLY (not via the DispatcherQueue): InvalidateMeasure only
// marks us dirty, so the layout manager re-measures the TableView within the SAME layout tick -- the
// column resizes in the same frame as the content change (no one-frame lag, so drags stay smooth).
// It converges without a debounce: the delta detector records each cell's new width in the pass that
// detects it, so the follow-up measure sees no delta and stops. Calling InvalidateMeasure during a
// descendant's measure is safe -- it schedules, it does not re-enter layout.
void TableView::RequestColumnWidthResolve()
{
    InvalidateMeasure();
}

// Reset the monotonic Auto desired widths. The next table measure pass remeasures the template
// subtree, pulls header + realized-row measured widths, and resolves once from those caches.
void TableView::ResetColumnDesiredWidths()
{
    if (auto columns = Columns())
    {
        for (auto const& column : columns)
        {
            // Reset EVERY column's grow-only accumulator regardless of its current Width mode. A
            // column that was Auto, temporarily switched to Pixel/Star, then switched back must not
            // retain a stale max from a previous data set. This runs only on real data-set boundaries
            // (ItemsSource / Columns replaced / CellTemplate / Header), so it correctly clears across
            // them while Density / Min / Max changes (which do not call this) preserve the max within a
            // data set.
            if (column && winrt::get_self<TableViewColumn>(column)->GetOwningTableView() == *this)
            {
                winrt::get_self<TableViewColumn>(column)->ResetDesiredWidthInternal();
            }
        }
    }

    InvalidateMeasure();
}

// Resolve every visible column into ActualWidth. Fixed columns (Pixel + measured Auto) are sized
// first; Star columns then split the viewport width left over between them.
void TableView::ResolveColumnWidths()
{
    auto columns = Columns();
    if (!columns)
    {
        return;
    }

    // Snap resolved column widths to device pixels when layout rounding is enabled, mirroring CGrid's
    // use of the rasterization scale. A zero factor leaves values unrounded.
    double layoutRoundFactor = 0.0;
    if (UseLayoutRounding())
    {
        if (auto const xamlRoot = XamlRoot())
        {
            layoutRoundFactor = xamlRoot.RasterizationScale();
        }
    }
    auto layoutRound = [layoutRoundFactor](double value)
    {
        return layoutRoundFactor > 0.0 ? std::round(value * layoutRoundFactor) / layoutRoundFactor : value;
    };

    auto setResolvedActualWidth = [&layoutRound](winrt::TableViewColumn const& target, double resolved)
    {
        resolved = layoutRound(resolved);
        const bool changed = std::abs(resolved - target.ActualWidth()) > 0.0001;
        winrt::get_self<TableViewColumn>(target)->SetResolvedActualWidthInternal(resolved);
        return changed;
    };

    double fixedTotal = 0.0;
    bool changed = false;
    std::vector<winrt::TableViewColumn> starColumns;

    // Collect the currently realized, BOUND rows ONCE (a single repeater walk) and reuse the list for
    // every Auto column below, instead of re-walking the visual tree (GetChild + try_as per row) once
    // per Auto column.
    std::vector<winrt::TableViewRow> realizedRows;
    if (auto repeater = m_rowsRepeater.get())
    {
        ForEachRealizedRow([&realizedRows, &repeater](winrt::TableViewRow const& row)
        {
            // Skip pooled/recycled rows: ItemsRepeater keeps them parented in the pool, but they carry
            // stale measured widths (GetElementIndex < 0). Consuming them would re-pin Auto columns at
            // the previous width after ItemsSource shrinks, making ResetColumnDesiredWidths a no-op
            // (grow-only keeps the max). Only genuinely bound rows contribute to Auto sizing.
            if (repeater.GetElementIndex(row) >= 0)
            {
                realizedRows.push_back(row);
            }
        });
    }

    for (auto const& column : columns)
    {
        // Rejected columns still appear in the app-owned vector; do not resolve widths for another
        // TableView's column.
        if (!column ||
            winrt::get_self<TableViewColumn>(column)->GetOwningTableView() != *this ||
            column.Visibility() != winrt::Visibility::Visible)
        {
            continue;
        }

        const auto width = column.Width();
        const double lo = column.MinWidth();
        const double hi = std::max(lo, column.MaxWidth());

        switch (width.GridUnitType)
        {
        case winrt::GridUnitType::Star:
            // Sized below once the fixed total is known.
            starColumns.push_back(column);
            break;

        case winrt::GridUnitType::Auto:
        {
            // Pull measured widths from the header and currently realized row panels. Preserve v1's
            // grow-only policy by storing max(previous desired, pulled measured) on the column.
            double pulledMeasuredMax = 0.0;
            // Only consume the header host's cache when headers are actually shown: a hidden header is
            // not measured this pass, so its per-pass cache is stale (and, with the raw-identity key,
            // a freed column's address could be reused by a new column and false-match a stale entry).
            // A hidden header must not drive column width regardless.
            if (ShouldShowColumnHeaders())
            {
                pulledMeasuredMax = std::max(pulledMeasuredMax, GetHeaderMeasuredWidthForColumn(column));
            }

            for (auto const& row : realizedRows)
            {
                // Ask the row for its cell's measured width instead of reaching into its panel.
                pulledMeasuredMax = std::max(pulledMeasuredMax, winrt::get_self<TableViewRow>(row)->MeasuredWidthForColumn(column));
            }

            auto columnImpl = winrt::get_self<TableViewColumn>(column);
            // Shrink-capable Auto: size to the CURRENT measured content max rather than a monotonic
            // grow-only max, so the column narrows when its widest content shrinks (CGrid parity).
            const double desired = pulledMeasuredMax;
            columnImpl->SetDesiredWidthInternal(desired);

            const double resolved = layoutRound(std::clamp(desired > 0.0 ? desired : c_widthDefault.Value, lo, hi));
            changed |= setResolvedActualWidth(column, resolved);
            fixedTotal += resolved;
            break;
        }

        case winrt::GridUnitType::Pixel:
        default:
        {
            const double resolved = layoutRound(std::clamp(width.Value, lo, hi));
            changed |= setResolvedActualWidth(column, resolved);
            fixedTotal += resolved;
            break;
        }
        }
    }

    if (starColumns.empty())
    {
        if (changed)
        {
            InvalidateCellPanels();
            RefreshFrozenColumns();
        }
        return;
    }

    // Star needs a finite viewport to divide. The horizontally-scrolling body panel is measured at
    // infinite width, so we pull the ScrollViewer viewport explicitly. Before it is known, leave the
    // Star columns at their provisional width; the body-scroller SizeChanged re-resolves later.
    auto bodyScroller = m_bodyScroller.get();
    const double viewport = bodyScroller ? bodyScroller.ViewportWidth() : 0.0;
    // A non-finite or not-yet-known viewport has no finite space to divide (e.g. the table hosted
    // in a width-to-content parent); leave Star columns at their provisional width rather than
    // arranging an infinite cell. The body scroller's SizeChanged re-resolves once a real width lands.
    if (!(viewport > 0.0) || std::isinf(viewport))
    {
        if (changed)
        {
            InvalidateCellPanels();
            RefreshFrozenColumns();
        }
        return;
    }

    const bool resizeEnabled = CanUserResizeColumns();
    auto authoredOf = [](winrt::TableViewColumn const& c)
    {
        return winrt::get_self<TableViewColumn>(c)->AuthoredWidthInternal();
    };
    // Only while a resize can actually happen; otherwise no column's width is at risk and the
    // ordinary pool already clamps correctly.
    auto isLocked = [resizeEnabled](winrt::TableViewColumn const& c)
    {
        return resizeEnabled && !c.CanResize();
    };

    if (std::any_of(starColumns.begin(), starColumns.end(), isLocked))
    {
        // A locked Star column takes its share of the authored layout rather than of whatever a
        // neighbor's resize left behind, so a drag cannot change its width.
        double authoredFixedTotal = 0.0;
        std::vector<winrt::TableViewColumn> authoredPool;
        for (auto const& c : columns)
        {
            if (!c ||
                winrt::get_self<TableViewColumn>(c)->GetOwningTableView() != *this ||
                c.Visibility() != winrt::Visibility::Visible)
            {
                continue;
            }

            const auto authored = authoredOf(c);
            if (authored.GridUnitType == winrt::GridUnitType::Star)
            {
                authoredPool.push_back(c);
                continue;
            }

            // Clamped and rounded exactly as the resolved pass above does, so the authored basis
            // and the real layout agree on what the fixed columns take.
            const double lo = c.MinWidth();
            const double hi = std::max(lo, c.MaxWidth());
            if (authored.GridUnitType == winrt::GridUnitType::Auto)
            {
                // The content width, not the width a resize gave it: a dragged Auto column must not
                // change what the authored layout leaves for the Star columns.
                const double desired = winrt::get_self<TableViewColumn>(c)->DesiredWidthInternal();
                authoredFixedTotal += layoutRound(std::clamp(desired > 0.0 ? desired : c.ActualWidth(), lo, hi));
            }
            else
            {
                authoredFixedTotal += layoutRound(std::clamp(authored.Value, lo, hi));
            }
        }

        DistributeStarWidths(
            authoredPool,
            std::max(0.0, layoutRound(viewport) - authoredFixedTotal),
            [&authoredOf](winrt::TableViewColumn const& c) { return std::max(0.0, authoredOf(c).Value); },
            layoutRound,
            [&](winrt::TableViewColumn const& c, double width)
            {
                if (isLocked(c))
                {
                    changed |= setResolvedActualWidth(c, width);
                    fixedTotal += c.ActualWidth();
                }
            });

        starColumns.erase(
            std::remove_if(starColumns.begin(), starColumns.end(), isLocked), starColumns.end());
    }

    // The viewport basis is layout-rounded so the divided space is snapped consistently with the
    // fixed columns (CGrid rounds availableSize before distribution); per-column Star widths are
    // then snapped in setResolvedActualWidth.
    DistributeStarWidths(
        starColumns,
        std::max(0.0, layoutRound(viewport) - fixedTotal),
        [](winrt::TableViewColumn const& c) { return std::max(0.0, c.Width().Value); },
        layoutRound,
        [&](winrt::TableViewColumn const& c, double width)
        {
            changed |= setResolvedActualWidth(c, width);
        });

    if (changed)
    {
        InvalidateCellPanels();
        RefreshFrozenColumns();
    }
}

// A resize takes space only from the columns after the dragged one (the WPF DataGrid contract).
// Freezing the earlier Star columns at the width they already render keeps them out of both the
// bound below and the redistribution pass, without changing what the user sees.
void TableView::FreezeColumnsBeforeResize(const winrt::TableViewColumn& column)
{
    auto columns = Columns();
    uint32_t index = 0;
    if (!columns || !columns.IndexOf(column, index))
    {
        return;
    }

    for (uint32_t i = 0; i < index; ++i)
    {
        auto const other = columns.GetAt(i);
        if (!other ||
            winrt::get_self<TableViewColumn>(other)->GetOwningTableView() != *this ||
            other.Visibility() != winrt::Visibility::Visible ||
            other.Width().GridUnitType != winrt::GridUnitType::Star)
        {
            continue;
        }

        auto const columnImpl = winrt::get_self<TableViewColumn>(other);
        auto const resizeScope = columnImpl->BeginUserResizeScope();
        other.Width(winrt::GridLengthHelper::FromPixels(other.ActualWidth()));
    }
}

// How far a drag may take this column. A table whose columns divide the viewport may not grow past
// it, and a column the user may not resize neither gives width away nor takes any.
ColumnResizeBounds TableView::ResizeBoundsForColumn(const winrt::TableViewColumn& column)
{
    ColumnResizeBounds bounds;

    auto columns = Columns();
    auto bodyScroller = m_bodyScroller.get();
    const double viewport = bodyScroller ? bodyScroller.ViewportWidth() : 0.0;
    if (!columns || !(viewport > 0.0) || std::isinf(viewport))
    {
        return bounds;
    }

    auto authoredType = [](winrt::TableViewColumn const& c)
    {
        return winrt::get_self<TableViewColumn>(c)->AuthoredWidthInternal().GridUnitType;
    };

    const bool resizeEnabled = CanUserResizeColumns();
    double reservedForOthers = 0.0;
    bool dividesViewport = authoredType(column) == winrt::GridUnitType::Star;
    bool hasParticipant = false;

    for (auto const& other : columns)
    {
        if (!other ||
            other == column ||
            winrt::get_self<TableViewColumn>(other)->GetOwningTableView() != *this ||
            other.Visibility() != winrt::Visibility::Visible)
        {
            continue;
        }

        dividesViewport |= authoredType(other) == winrt::GridUnitType::Star;

        // Only a column that is Star *now* can yield space: the layout pass re-divides by current
        // Width, so one an earlier drag rewrote as pixels donates nothing.
        const bool participates =
            other.Width().GridUnitType == winrt::GridUnitType::Star && resizeEnabled && other.CanResize();
        hasParticipant |= participates;

        if (!participates)
        {
            reservedForOthers += other.ActualWidth();
        }
        else
        {
            reservedForOthers += MinWidthForStarFactor(other, std::max(0.0, other.Width().Value));
        }
    }

    // Without a Star column anywhere the extent is meant to grow and scroll.
    if (!dividesViewport)
    {
        return bounds;
    }

    // Pinned in both directions: bounding only growth would let the drag hand width to a column
    // that is then not allowed to give it back. A Pixel or Auto column owns its width outright, so
    // it may still shrink -- that only makes the table narrower and needs nothing from a neighbour.
    if (!hasParticipant && column.Width().GridUnitType == winrt::GridUnitType::Star)
    {
        bounds.Min = column.ActualWidth();
        bounds.Max = bounds.Min;
        return bounds;
    }

    bounds.Max = std::max(0.0, viewport - reservedForOthers);
    return bounds;
}

// Re-run the cell panels' measure/arrange so the header band and all rows reflect the newly resolved
// column widths (Auto growth in one row must widen the header and every other row).
void TableView::InvalidateCellPanels()
{
    if (auto headerHost = m_headerHost.get())
    {
        headerHost.InvalidateMeasure();
    }

    if (auto repeater = m_rowsRepeater.get())
    {
        const auto childCount = winrt::VisualTreeHelper::GetChildrenCount(repeater);
        for (int32_t i = 0; i < childCount; ++i)
        {
            if (auto row = winrt::VisualTreeHelper::GetChild(repeater, i).try_as<winrt::TableViewRow>())
            {
                // Ask the row to invalidate its own cell panel instead of reaching into it.
                winrt::get_self<TableViewRow>(row)->InvalidateCells();
            }
            else if (auto header = winrt::VisualTreeHelper::GetChild(repeater, i).try_as<winrt::TableViewGroupHeader>())
            {
                // The group-header band spans the same columns and sizes itself during measure, but
                // it is a direct repeater child rather than a cells panel, so the row walk above
                // never reaches it. Handled in this one walk so the two cannot drift apart.
                UpdateGroupHeaderWidth(header);
            }
        }
    }
}
