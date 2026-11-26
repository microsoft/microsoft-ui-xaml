// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TableViewColumn.g.h"
#include "TableViewColumn.properties.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

// Default pixel width and fallback for unresolved Auto values (and Star before the owning
// TableView has a viewport to resolve against); keep in sync with ActualWidth's
// MUX_DEFAULT_VALUE("120.0") in TableView.idl.
static constexpr winrt::GridLength c_widthDefault{ 120.0, winrt::GridUnitType::Pixel };

static inline double NonNegativeFiniteOrZero(double value)
{
    return std::isfinite(value) && value >= 0.0 ? value : 0.0;
}

static inline double NonNegativeFiniteOrDefault(double value)
{
    return std::isfinite(value) && value >= 0.0 ? value : c_widthDefault.Value;
}

static inline double MaxWidthForColumn(winrt::TableViewColumn const& column, double minWidth)
{
    const double maxWidth = column.MaxWidth();
    if (std::isnan(maxWidth) || maxWidth == std::numeric_limits<double>::infinity())
    {
        return std::numeric_limits<double>::infinity();
    }

    // A negative MaxWidth is a malformed finite bound. Treat it as zero so it still clamps to
    // MinWidth; only NaN and positive infinity mean there is no upper bound.
    return std::max(minWidth, std::max(0.0, maxWidth));
}

class TableViewColumn :
    public ReferenceTracker<TableViewColumn, winrt::implementation::TableViewColumnT, winrt::composable>,
    public TableViewColumnProperties
{
public:
    TableViewColumn();

    // Internal: the display cell, through the overridable GenerateElementCore.
    winrt::FrameworkElement GenerateDisplayElement(const winrt::IInspectable& dataItem);

    // Typed accessor for the owning TableView.
    winrt::TableView GetOwningTableView();

    using TableViewColumnProperties::MinWidth;
    void MinWidth(double value);

    // Internal — stable, non-address token used by column-header automation peers.
    uint32_t AutomationIdentity();

    // Overridable
    virtual winrt::hstring GetSortMemberPathCore();
    virtual winrt::FrameworkElement GenerateElementCore(const winrt::IInspectable& dataItem);

    // Property-changed callback (single dispatch in IDL)
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    // Not a dependency property: the codegen does not project delegate-typed DPs, and a comparer
    // has nothing to bind, animate or style anyway.
    winrt::ITableViewSortComparer CustomSortComparer() const { return m_customSortComparer; }
    void CustomSortComparer(winrt::ITableViewSortComparer const& value);

    // Protected ABI: derived columns call this when one of their own properties invalidates
    // realized cell content. Regenerates realized cells and resets the Auto width.
    void NotifyCellContentChanged();
    void NotifyCellContentChangedInternal(bool resetAutoWidth);

    // Internal: a cell-content change on a live table stays pending until a regeneration takes it.
    // Queue returns true when a post for it is already in flight.
    bool QueueCellContentChangeInternal(bool resetAutoWidth) noexcept
    {
        m_cellContentChangePending = true;
        m_queuedCellContentResetAutoWidth = m_queuedCellContentResetAutoWidth || resetAutoWidth;
        return m_cellContentChangePosted;
    }
    void SetCellContentChangePostedInternal(bool posted) noexcept { m_cellContentChangePosted = posted; }
    bool HasPendingCellContentChangeInternal() const noexcept { return m_cellContentChangePending; }
    // Bumped by every take, so a post that a regeneration has already covered does nothing.
    uint32_t CellContentChangeGenerationInternal() const noexcept { return m_cellContentChangeGeneration; }
    bool TakeQueuedCellContentChangeInternal() noexcept
    {
        m_cellContentChangePending = false;
        InvalidateCellContentChangePostInternal();
        return std::exchange(m_queuedCellContentResetAutoWidth, false);
    }
    // Drops the in-flight post but keeps the change pending, for the next owner to regenerate.
    void InvalidateCellContentChangePostInternal() noexcept
    {
        m_cellContentChangePosted = false;
        ++m_cellContentChangeGeneration;
    }

    // Keep the owner weak to avoid TableView -> Columns -> Column -> TableView cycles.
    bool SetOwningTableViewInternal(winrt::TableView const& owner);

    // Setters let XAML pass the Binding object through without evaluating it.
    winrt::Microsoft::UI::Xaml::Data::Binding DisplayMemberBinding();
    void DisplayMemberBinding(const winrt::Microsoft::UI::Xaml::Data::Binding& value);

    // Internal: the read-only text cell for DisplayMemberBinding, or null when it is not set.
    winrt::FrameworkElement GenerateDisplayMemberElementInternal();
    static bool UsesDisplayMemberText(winrt::TableViewColumn const& column)
    {
        return column && column.DisplayMemberBinding();
    }
    // Restamps the density font size on a generated text cell unless the app has taken it over.
    static void RefreshGeneratedFontSize(winrt::TextBlock const& text, double fontSize);
    static void ClearDisplayMemberProperties();

    winrt::Microsoft::UI::Xaml::Data::Binding CellToolTipBinding();
    void CellToolTipBinding(const winrt::Microsoft::UI::Xaml::Data::Binding& value);

    // Internal — layout: the owning TableView (TableView_Layout.cpp) resolves the final width for
    // every sizing mode (Pixel/Auto/Star) and pushes it here; the caller has already clamped to
    // Min/MaxWidth.
    void SetResolvedActualWidthInternal(double width);

    // Internal - sorting: the owning TableView owns sort policy and pushes the resulting direction
    // into the DP; OnPropertyChanged republishes it to the realized header chevrons. The push is
    // required rather than a binding: SortIndicatorDirection and SortDirection are distinct WinRT
    // enums, so a {Binding} between them silently does nothing.
    void SetSortStateInternal(winrt::SortDirection direction);

    // Internal — Auto size-to-content. The owning TableView pulls measured widths from the header
    // and realized row panels; ResolveColumnWidths keeps the grow-only max of the row widths and of
    // the header width separately, so hiding the headers drops only the header term. Both reset on
    // data-set boundaries (ItemsSource / Columns replaced / CellTemplate / Header changes).
    void SetDesiredWidthInternal(double rowsDesiredWidth, double headerDesiredWidth);
    void ResetDesiredWidthInternal();
    double RowsDesiredWidthInternal() const noexcept { return m_rowsDesiredWidth; }
    double HeaderDesiredWidthInternal() const noexcept { return m_headerDesiredWidth; }
    double DesiredWidthInternal() const noexcept { return std::max(m_rowsDesiredWidth, m_headerDesiredWidth); }

    // Internal - resize: the width the app authored. A user resize rewrites Width as pixels, and
    // the layout needs the original mode to know whether the table is meant to fit its viewport.
    winrt::GridLength AuthoredWidthInternal() const noexcept { return m_authoredWidth; }
    // Width written inside this scope is the control resizing, not the app re-authoring the column.
    [[nodiscard]] auto BeginUserResizeScope()
    {
        m_inUserResize = true;
        return gsl::finally([this]() { m_inUserResize = false; });
    }

private:
    // Write the resolved, clamped width into the read-only ActualWidth DP.
    void UpdateActualWidth();

    // Monotonic maxima of the pulled realized-row cell and header measured widths for an Auto column
    // (0 until first pull; the header term is 0 while headers are hidden).
    double m_rowsDesiredWidth{ 0.0 };
    double m_headerDesiredWidth{ 0.0 };
    winrt::GridLength m_authoredWidth{ c_widthDefault };
    bool m_inUserResize{ false };
    bool m_cellContentChangePosted{ false };
    bool m_cellContentChangePending{ false };
    bool m_hasBeenOwned{ false };
    bool m_isSettingSortStateInternal{ false };
    bool m_queuedCellContentResetAutoWidth{ false };
    uint32_t m_cellContentChangeGeneration{ 0 };

    tracker_ref<winrt::Microsoft::UI::Xaml::Data::Binding> m_displayMemberBinding{ this };
    tracker_ref<winrt::Microsoft::UI::Xaml::Data::Binding> m_cellToolTipBinding{ this };

    weak_ref<winrt::TableView> m_owningTableView{ nullptr };
    int64_t m_cellContextFlyoutToken{ 0 };
    winrt::ITableViewSortComparer m_customSortComparer{ nullptr };
    uint32_t m_automationIdentity{ 0 };
};

namespace TableViewDetails
{
    inline bool IsColumnOwnedBy(winrt::TableView const& owner, winrt::TableViewColumn const& column)
    {
        return column && winrt::get_self<TableViewColumn>(column)->GetOwningTableView() == owner;
    }

    // The leading-frozen prefix: unowned columns are skipped and the first owned non-Leading column ends it.
    template <typename Columns, typename Fn>
    void ForEachLeadingFrozenColumn(winrt::TableView const& owner, Columns const& columns, Fn&& fn)
    {
        for (auto const& column : columns)
        {
            if (!IsColumnOwnedBy(owner, column))
            {
                continue;
            }
            if (column.FrozenEdge() != winrt::TableViewFrozenEdge::Leading)
            {
                break;
            }
            fn(column);
        }
    }
}
