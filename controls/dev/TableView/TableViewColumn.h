// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TableViewColumn.g.h"
#include "TableViewColumn.properties.h"

// Default pixel width and fallback for unresolved Auto values (and Star before the owning
// TableView has a viewport to resolve against); keep in sync with ActualWidth's
// MUX_DEFAULT_VALUE("120.0") in TableView.idl.
static constexpr winrt::GridLength c_widthDefault{ 120.0, winrt::GridUnitType::Pixel };

class TableViewColumn :
    public ReferenceTracker<TableViewColumn, winrt::implementation::TableViewColumnT, winrt::composable>,
    public TableViewColumnProperties
{
public:
    TableViewColumn();

    // Public ABI
    winrt::FrameworkElement GenerateElement(const winrt::IInspectable& dataItem);
    winrt::FrameworkElement GenerateEditingElement(const winrt::IInspectable& dataItem);
    winrt::IInspectable PrepareCellForEdit(const winrt::FrameworkElement& editingElement, const winrt::RoutedEventArgs& editingEventArgs);
    bool CommitCellEdit(const winrt::FrameworkElement& editingElement);
    void CancelCellEdit(const winrt::FrameworkElement& editingElement, const winrt::IInspectable& uneditedValue);

    // Typed accessor for the owning TableView.
    winrt::TableView GetOwningTableView();

    // Overridable
    virtual winrt::hstring GetSortMemberPathCore();
    virtual winrt::FrameworkElement GenerateElementCore(const winrt::IInspectable& dataItem);
    virtual winrt::FrameworkElement GenerateEditingElementCore(const winrt::IInspectable& dataItem);
    virtual winrt::IInspectable PrepareCellForEditCore(const winrt::FrameworkElement& editingElement, const winrt::RoutedEventArgs& editingEventArgs);
    virtual bool CommitCellEditCore(const winrt::FrameworkElement& editingElement);
    virtual void CancelCellEditCore(const winrt::FrameworkElement& editingElement, const winrt::IInspectable& uneditedValue);

    // Binding expressions on an editor subtree, for the properties an editor realistically writes.
    static std::vector<winrt::BindingExpression> CollectEditingBindingExpressions(const winrt::FrameworkElement& element);

    // Property-changed callback (single dispatch in IDL)
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    // Not a dependency property: the codegen does not project delegate-typed DPs, and a comparer
    // has nothing to bind, animate or style anyway.
    winrt::ITableViewSortComparer CustomSortComparer() const { return m_customSortComparer; }
    void CustomSortComparer(winrt::ITableViewSortComparer const& value);

    // Derived columns call this when one of their own properties invalidates realized cell content.
    void NotifyCellContentChanged();

    // Keep the owner weak to avoid TableView -> Columns -> Column -> TableView cycles.
    bool SetOwningTableViewInternal(winrt::TableView const& owner);

    // Setter lets XAML pass the Binding object through without evaluating it.
    winrt::Microsoft::UI::Xaml::Data::Binding CellToolTipBinding();
    void CellToolTipBinding(const winrt::Microsoft::UI::Xaml::Data::Binding& value);

    // Internal — layout: the owning TableView (TableView_Layout.cpp) resolves the final width for
    // every sizing mode (Pixel/Auto/Star) and pushes it here; the caller has already clamped to
    // Min/MaxWidth.
    void SetResolvedActualWidthInternal(double width);

    // Internal - sorting: the owning TableView owns sort policy and pushes the resulting direction
    // into this read-only DP, then republishes it to the realized header chevrons. The push is
    // required rather than a binding: SortIndicatorDirection and SortDirection are distinct WinRT
    // enums, so a {Binding} between them silently does nothing.
    void SetSortStateInternal(winrt::SortDirection direction);

    // Internal — Auto size-to-content. The owning TableView pulls measured widths from the header
    // and realized row panels; ResolveColumnWidths derives the current measured max each pass and
    // stores the resulting nonnegative desired width here (shrink-capable — not a grow-only
    // accumulator). Reset on data-set boundaries (ItemsSource / Columns replaced / CellTemplate /
    // Header changes).
    void SetDesiredWidthInternal(double desiredWidth);
    void ResetDesiredWidthInternal();
    double DesiredWidthInternal() const noexcept { return m_desiredWidth; }

private:
    // Write the resolved, clamped width into the read-only ActualWidth DP.
    void UpdateActualWidth();

    // Monotonic max of pulled realized-cell measured widths for an Auto column (0 until first pull).
    double m_desiredWidth{ 0.0 };

    tracker_ref<winrt::Microsoft::UI::Xaml::Data::Binding> m_cellToolTipBinding{ this };

    weak_ref<winrt::TableView> m_owningTableView{ nullptr };
    winrt::ITableViewSortComparer m_customSortComparer{ nullptr };
};
