// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewCellsPanel.h"
#include "TableViewColumn.h"
#include "TableViewRow.h"
#include "TableView.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace
{
    // A change of more than this (device-independent px) in a cell's own measured width between passes
    // triggers a deferred column-width re-resolve, so an Auto column re-sizes (grows OR shrinks) when a
    // *realized* cell's content changes during interaction (slider/expander/edit). The threshold
    // absorbs sub-pixel / layout-rounding noise so steady state doesn't churn.
    constexpr double c_columnMeasureChangeThreshold = 0.5;
}

winrt::TableViewColumn TableViewCellsPanel::ColumnForCell(winrt::UIElement const& child)
{
    if (auto const fe = child.try_as<winrt::FrameworkElement>())
    {
        return fe.Tag().try_as<winrt::TableViewColumn>();
    }
    return nullptr;
}

winrt::FrameworkElement TableViewCellsPanel::CellForColumn(const winrt::Panel& host, winrt::TableViewColumn const& column)
{
    if (!host || !column)
    {
        return nullptr;
    }

    auto const children = host.Children();
    const uint32_t count = children.Size();
    for (uint32_t i = 0; i < count; ++i)
    {
        if (auto const cell = children.GetAt(i).try_as<winrt::FrameworkElement>())
        {
            if (cell.Tag().try_as<winrt::TableViewColumn>() == column)
            {
                return cell;
            }
        }
    }
    return nullptr;
}

void TableViewCellsPanel::SetOwningRowInternal(winrt::TableViewRow const& row)
{
    m_owningRow = row;
}

double TableViewCellsPanel::MeasuredWidthForColumn(winrt::TableViewColumn const& column) const
{
    if (!column)
    {
        return 0.0;
    }

    // Non-owning identity key (see m_measuredWidthsByColumn note) -- a raw pointer instead of a
    // per-lookup weak_ref resolve on the hot layout path; O(1) hash lookup.
    void* const key = winrt::get_self<TableViewColumn>(column);
    auto const it = m_measuredWidthsByColumn.find(key);
    return it != m_measuredWidthsByColumn.end() ? it->second : 0.0;
}

void TableViewCellsPanel::CacheMeasuredWidthForColumn(winrt::TableViewColumn const& column, double measuredWidth)
{
    if (!column)
    {
        return;
    }

    measuredWidth = std::max(0.0, measuredWidth);
    void* const key = winrt::get_self<TableViewColumn>(column);
    // Grow-only within the pass (the map is cleared each MeasureOverride); operator[] default-inserts
    // 0.0 on first sight of the column this pass, so max() yields the measured width.
    auto& cachedWidth = m_measuredWidthsByColumn[key];
    cachedWidth = std::max(cachedWidth, measuredWidth);
}

bool TableViewCellsPanel::RecordAndDetectMeasuredWidthChange(
    winrt::TableViewColumn const& column, double measuredWidth,
    std::unordered_map<void*, double>& newLastMeasured) const
{
    void* const key = winrt::get_self<TableViewColumn>(column);

    auto const it = m_lastMeasuredWidthsByColumn.find(key);
    const bool hadPrevious = it != m_lastMeasuredWidthsByColumn.end();
    const double previous = hadPrevious ? it->second : 0.0;

    // Record for next pass regardless (the caller swaps newLastMeasured into the member after the loop).
    newLastMeasured[key] = measuredWidth;

    // First time this panel sees the column: no history, so no change signal (a fresh/realized row's
    // initial sizing is already driven by ElementPrepared -> InvalidateMeasure).
    return hadPrevious && std::abs(measuredWidth - previous) > c_columnMeasureChangeThreshold;
}

winrt::Size TableViewCellsPanel::MeasureOverride(winrt::Size const& availableSize)
{
    const float infinity = std::numeric_limits<float>::infinity();
    float width = 0.0f;
    float height = 0.0f;

    m_measuredWidthsByColumn.clear();

    // Rebuilt below from only the columns measured this pass, then swapped into
    // m_lastMeasuredWidthsByColumn -- so removed/collapsed columns are pruned automatically.
    std::unordered_map<void*, double> newLastMeasured;
    newLastMeasured.reserve(m_lastMeasuredWidthsByColumn.size());

    // If a realized Auto cell's own measured width changed since the last pass (grow or shrink), the
    // column may need to re-resolve. Remember the owning TableView and request a deferred re-resolve
    // after the loop -- the body ScrollViewer otherwise absorbs the cell's measure invalidation, so
    // TableView::MeasureOverride (and ResolveColumnWidths) would never re-run for live content changes.
    winrt::TableView ownerNeedingResolve{ nullptr };

    // The cell currently hosting an editor, if any. An editor is a different control from the display
    // element it replaced - a TextBox carries border, padding and a default MinWidth a TextBlock does
    // not - so letting it feed the Auto-width pass makes the column visibly jump wider the instant the
    // user starts editing, and jump back on commit. The column's width is a property of the DATA, not
    // of which control happens to be showing it, so the editing cell is measured at the width the
    // column already has and contributes nothing to the Auto calculation. WPF's DataGrid behaves the
    // same way: entering edit mode does not resize the column.
    winrt::UIElement editingCell{ nullptr };
    if (auto const row = m_owningRow.get())
    {
        editingCell = winrt::get_self<TableViewRow>(row)->GetEditingCellWrapper();
    }

    for (winrt::UIElement const& child : Children())
    {
        auto const column = ColumnForCell(child);

        // Cells with no owning column (defensive): measure unconstrained and take their natural width.
        if (!column)
        {
            child.Measure({ infinity, availableSize.Height });
            const auto childDesired = child.DesiredSize();
            height = std::max(height, childDesired.Height);
            width += childDesired.Width;
            continue;
        }

        // Collapsed columns occupy no width; still measure once (at zero width) so the child has a
        // valid measure, consistent with being arranged at zero width.
        if (column.Visibility() != winrt::Visibility::Visible)
        {
            child.Measure({ 0.0f, availableSize.Height });
            continue;
        }

        const float columnWidth = static_cast<float>(std::max(0.0, column.ActualWidth()));

        if (column.Width().GridUnitType == winrt::GridUnitType::Auto)
        {
            // The editing cell is pinned to the column's current width and excluded from the Auto
            // calculation entirely - no cached contribution, and no change signal, so it cannot ask
            // the owner to re-resolve either. Normal measurement resumes for this cell as soon as the
            // edit closes and the display element is back.
            if (editingCell && child == editingCell)
            {
                child.Measure({ columnWidth > 0.0f ? columnWidth : infinity, availableSize.Height });
                height = std::max(height, child.DesiredSize().Height);

                // Carry a width forward so the column does not collapse while the edit is open.
                // Contributing nothing would let an Auto column resolve from the header and other
                // rows alone - and with a single realized row that means resolving to near zero,
                // which churns layout and can rebuild the row out from under the live editor.
                // Prefer this cell's previous measured width; fall back to the column's resolved
                // width when the editor opened before this panel ever measured the cell.
                auto const key = winrt::get_self<TableViewColumn>(column);
                auto const previous = m_lastMeasuredWidthsByColumn.find(key);
                const double carried = (previous != m_lastMeasuredWidthsByColumn.end())
                    ? previous->second
                    : static_cast<double>(columnWidth);

                newLastMeasured[key] = carried;
                CacheMeasuredWidthForColumn(column, carried);
                continue;
            }

            // Only Auto columns depend on content width. Measure unconstrained to discover the cell's
            // natural width and cache it; TableView pulls the max across header + realized rows once per
            // pass to resolve the Auto column width.
            child.Measure({ infinity, availableSize.Height });
            const auto childDesired = child.DesiredSize();
            height = std::max(height, childDesired.Height);
            CacheMeasuredWidthForColumn(column, childDesired.Width);

            // Change signal: this cell's own measured width differs from the previous pass (grow OR
            // shrink). Comparing to the cell's own history -- not the column width -- is convergent: a
            // stably-narrower cell reports the same width each pass (no signal), while a cell whose
            // content actually changed reports a delta and asks the owner to re-resolve. Once the
            // re-resolve settles the column and the cell re-measures unchanged, the delta is zero and it
            // stops firing (MaxWidth-clamped columns also converge, since the compare is history-based,
            // not against the clamped column width -- so no ping-pong).
            if (RecordAndDetectMeasuredWidthChange(column, childDesired.Width, newLastMeasured) &&
                !ownerNeedingResolve)
            {
                ownerNeedingResolve = winrt::get_self<TableViewColumn>(column)->GetOwningTableView();
            }

            // Re-measure at the resolved width ONLY when the column is narrower than the content (e.g.
            // clamped by MaxWidth). XAML's arrange would otherwise expand the cell's render size to its
            // desired width and clip it to the column slot -- clipping away the cell's right border (the
            // vertical gridline); the constrained measure keeps ellipsized content and the border within
            // the column, and lets width-sensitive content report the height it wants at the column
            // width. When the column is at least as wide as the content, the unconstrained measure
            // already fits (no clip, no wrap, stable height), so the second measure is skipped -- this
            // mirrors CGrid, which measures unclamped Auto cells only once and does not re-measure them.
            if (columnWidth > 0.0f && columnWidth < childDesired.Width)
            {
                child.Measure({ columnWidth, availableSize.Height });
                height = std::max(height, child.DesiredSize().Height);
            }

            width += columnWidth;
        }
        else
        {
            // Pixel / Star: the width is independent of content, so skip the unconstrained probe and
            // measure once directly at the resolved column width (CGrid measures Star cells once, after
            // star resolution, at the resolved width; Pixel cells once at the fixed width). Before the
            // width is resolved (first pass, ActualWidth == 0), fall back to an unconstrained measure for
            // a provisional height; the panel is re-measured after ResolveColumnWidths sets ActualWidth.
            child.Measure({ columnWidth > 0.0f ? columnWidth : infinity, availableSize.Height });
            height = std::max(height, child.DesiredSize().Height);
            width += columnWidth;
        }
    }

    // Adopt this pass's measured widths as the baseline for the next pass (prunes removed/collapsed
    // columns since only columns measured above were recorded).
    m_lastMeasuredWidthsByColumn = std::move(newLastMeasured);

    // Deferred (not during this measure) so it runs after the current layout pass; TableView debounces
    // so repeated change signals collapse to a single re-resolve.
    if (ownerNeedingResolve)
    {
        winrt::get_self<TableView>(ownerNeedingResolve)->RequestColumnWidthResolve();
    }

    return { width, height };
}

winrt::Size TableViewCellsPanel::ArrangeOverride(winrt::Size const& finalSize)
{
    float x = 0.0f;

    for (winrt::UIElement const& child : Children())
    {
        auto const column = ColumnForCell(child);
        const float w = column
            ? (column.Visibility() == winrt::Visibility::Visible ? static_cast<float>(std::max(0.0, column.ActualWidth())) : 0.0f)
            : child.DesiredSize().Width;

        // Cells are arranged at the resolved column width; content wider than the column clips/ellipsizes.
        child.Arrange({ x, 0.0f, w, finalSize.Height });
        x += w;
    }

    return { x, finalSize.Height };
}

void TableViewCellsPanel::ApplyFrozenColumnLayout(const winrt::Panel& host, double horizontalOffset, double leadingFrozenWidth)
{
    if (!host)
    {
        return;
    }

    if (host.FlowDirection() == winrt::FlowDirection::RightToLeft)
    {
        // Frozen-column pin math is LTR-only; under RTL it would pin/clip the wrong
        // edge. Skip pinning in RTL so scrolling remains plain and uncorrupted.
        auto children = host.Children();
        const uint32_t count = children.Size();
        for (uint32_t i = 0; i < count; ++i)
        {
            if (auto el = children.GetAt(i).try_as<winrt::FrameworkElement>())
            {
                el.Translation({ 0.0f, 0.0f, 0.0f });
                winrt::Canvas::SetZIndex(el, 0);
                el.Clip(nullptr);
            }
        }
        return;
    }

    // Accumulate each cell's panel-space left edge from column ActualWidth.
    double panelX = 0.0;
    // Only the contiguous leading prefix (from the first cell) is pinned. A Leading flag on a
    // non-prefix column is treated as non-frozen so it cannot overlap scrolled cells.
    bool inLeadingPrefix = true;
    auto children = host.Children();
    const uint32_t count = children.Size();
    for (uint32_t i = 0; i < count; ++i)
    {
        auto element = children.GetAt(i).try_as<winrt::FrameworkElement>();
        if (!element)
        {
            continue;
        }

        auto column = element.Tag().try_as<winrt::TableViewColumn>();
        const double cellWidth = column
            ? (column.Visibility() == winrt::Visibility::Visible ? std::max(0.0, column.ActualWidth()) : 0.0)
            : std::max(0.0, element.ActualWidth());
        const bool columnIsLeading = column && column.FrozenEdge() == winrt::TableViewFrozenEdge::Leading;
        const bool isLeadingFrozen = inLeadingPrefix && columnIsLeading;
        if (column && !columnIsLeading)
        {
            // First non-Leading column ends the frozen prefix; later Leading columns are not pinned.
            inLeadingPrefix = false;
        }

        if (isLeadingFrozen)
        {
            // Counter-translate by the scroll offset and paint above scrolled cells.
            // Translation X/Y has no visual effect unless enabled on the element first.
            winrt::ElementCompositionPreview::SetIsTranslationEnabled(element, true);
            element.Translation({ static_cast<float>(horizontalOffset), 0.0f, 0.0f });
            winrt::Canvas::SetZIndex(element, 1);
            element.Clip(nullptr);
        }
        else
        {
            element.Translation({ 0.0f, 0.0f, 0.0f });
            winrt::Canvas::SetZIndex(element, 0);

            // Hide local x below leadingFrozenWidth - panelX + offset.
            const double clipLeft = (leadingFrozenWidth > 0.0)
                ? std::max(0.0, leadingFrozenWidth + horizontalOffset - panelX)
                : 0.0;
            if (clipLeft > 0.0)
            {
                // A zero-width clip hides cells fully covered by the pinned region.
                const double clipWidth = std::max(0.0, cellWidth - clipLeft);
                const double actualHeight = element.ActualHeight();
                if (actualHeight <= 0.0)
                {
                    // Skip clipping until measured instead of using a magic tall sentinel.
                    element.Clip(nullptr);
                    panelX += cellWidth;
                    continue;
                }

                const float clipHeight = static_cast<float>(actualHeight);
                auto geometry = element.Clip().try_as<winrt::RectangleGeometry>();
                if (!geometry)
                {
                    geometry = winrt::RectangleGeometry();
                    element.Clip(geometry);
                }
                geometry.Rect(winrt::Rect{
                    static_cast<float>(clipLeft),
                    0.0f,
                    static_cast<float>(clipWidth),
                    clipHeight });
            }
            else
            {
                element.Clip(nullptr);
            }
        }

        panelX += cellWidth;
    }
}
