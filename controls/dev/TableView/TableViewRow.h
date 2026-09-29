// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "TableViewRow.g.h"
#include "TableViewRow.properties.h"

class TableViewRow :
    public ReferenceTracker<TableViewRow, winrt::implementation::TableViewRowT>,
    public TableViewRowProperties
{
public:
    TableViewRow();

    // IFrameworkElement overrides
    void OnApplyTemplate();
    winrt::AutomationPeer OnCreateAutomationPeer();

    // Use the Control virtual signatures; event-handler overloads break the ABI shim.
    void OnPointerEntered(winrt::PointerRoutedEventArgs const& args);
    void OnPointerExited(winrt::PointerRoutedEventArgs const& args);
    void OnPointerPressed(winrt::PointerRoutedEventArgs const& args);
    void OnPointerReleased(winrt::PointerRoutedEventArgs const& args);
    void OnPointerCaptureLost(winrt::PointerRoutedEventArgs const& args);
    void OnPointerCanceled(winrt::PointerRoutedEventArgs const& args);

    // Updates the weak owner ref, column subscription, and realized cells.
    void SetOwningTableViewInternal(winrt::TableView const& owner);

    // Rewire realized rows when the owner keeps the same identity but Columns changes.
    void RefreshColumnsSubscriptionInternal();

    // Typed accessor for the owning TableView.
    winrt::TableView GetOwningTableView();

    void RefreshGridLines();
    void RefreshRowBackground();

    // Owner-only writer for the read-only IsSelected DP. Selection is owned by the TableView, so
    // this is the main entry point that both publishes the DP and re-enters the CommonStates VSM
    // with transitions. The only other writer is the recycle path in SetOwningTableViewInternal,
    // which clears it without transitions.
    void SetIsSelectedInternal(bool isSelected);

    // Used by automation peers to enumerate live cells after template application.
    winrt::Panel GetCellsHostPanelInternal() const { return m_cellsHost.get(); }
    winrt::TableViewColumn GetCellOwningColumn(const winrt::UIElement& cellElement) const;

    // ----- Cell-level keyboard focus -----
    //
    // The row is still the element the framework and ItemsRepeater deal in, but the element that
    // actually holds focus is a cell, so that UIA reports a cell and Narrator announces one column
    // rather than the whole row. These are the row's half of that: the control drives navigation.

    // The number of cells in this row whose column is visible. This is the coordinate space the
    // control navigates in, and the same one TableViewCellAutomationPeer::Column reports.
    int32_t GetVisibleCellCountInternal() const;

    winrt::UIElement GetVisibleCellInternal(int32_t visibleColumnIndex) const;

    int32_t GetVisibleCellIndexInternal(const winrt::UIElement& cell) const;

    winrt::UIElement FindOwnCellInternal(const winrt::DependencyObject& element, bool requireExact) const;

    // Moves keyboard focus to the cell at a visible-column index. Falls back to the row when the
    // row has no such cell, so a row with no columns is still reachable.
    bool FocusVisibleCellInternal(int32_t visibleColumnIndex, winrt::FocusState state);

    void RefreshFrozenColumnLayout(double horizontalOffset, double leadingFrozenWidth);
    void RefreshDensity();
    void RefreshCells();

    // Releases control-owned cell tooltips (recycle-out).
    void ReleaseCellToolTips();

    void RefreshColumnVisibility(const winrt::TableViewColumn& column, winrt::Visibility visibility);
    double MeasuredWidthForColumn(const winrt::TableViewColumn& column) const;
    void InvalidateCells();


    bool BeginCellEdit(const winrt::TableViewColumn& column, const winrt::IInspectable& dataItem);

    // Restores the display visual. Safe to call when no cell edit is open.
    void EndCellEdit(winrt::TableViewEditAction action);

    winrt::FrameworkElement GetEditingElement() const { return m_editingElement.get(); }

    winrt::UIElement GetEditingCellWrapper() const { return m_editingCellWrapper.get(); }
    winrt::UIElement GetDisplayElementForAutomation(winrt::UIElement const& cell) const
    {
        return m_editingCellWrapper.get() == cell || m_pendingEditingCell.get() == cell
            ? m_editingDisplayElement.get() : nullptr;
    }

    // Drops the edit bookkeeping and puts the display visual back, WITHOUT moving focus. Used on the
    // recycle / rebuild paths, which run inside the measure pass where changing focus would trip
    // XAML's re-entrancy guard (0xc0000420).
    void AbandonCellEdit();

    void OnPointerPressedForEditing(
        const winrt::IInspectable& sender,
        const winrt::PointerRoutedEventArgs& args);

private:
    void ResetCellAutomationNames();

    // Focus landing on the row container itself is redirected onto a cell: keyboard navigation,
    // the editor teardown and Tab all focus the row, and without this the UIA focused element
    // would be the row - which is the row-level-focus defect this control is fixing.
    void OnRowGettingFocus(
        const winrt::UIElement& sender,
        const winrt::Microsoft::UI::Xaml::Input::GettingFocusEventArgs& args);

    void OnRowGotFocus();

    // Installs a generated display element as a cell's content, wiring the ContentPresenter Content
    // binding a template column needs. GenerateElement alone is not a complete cell.
    void AttachCellContent(const winrt::Grid& cellWrapper, const winrt::FrameworkElement& cellElement);

    // Drops begin-edit gesture state (recycle, owner change).
    void ResetPressState();

    winrt::TableViewColumn ResolvePressedColumn(
        const winrt::IInspectable& originalSource,
        const winrt::Point& hostPoint);

    void OnDataContextChanged(
        const winrt::FrameworkElement& sender,
        const winrt::DataContextChangedEventArgs& args);

    void OnColumnsVectorChanged(
        const winrt::IObservableVector<winrt::TableViewColumn>& sender,
        const winrt::IVectorChangedEventArgs& args);

    // Detach from / attach to the owning TableView's Columns vector. Shared by
    // SetOwningTableViewInternal (owner add/clear) and RefreshColumnsSubscriptionInternal
    // (same owner, Columns replaced). AttachColumnsSubscription is a no-op when owner is null.
    void DetachColumnsSubscription();
    void AttachColumnsSubscription(winrt::TableView const& owner);

    // Routes IsEnabled changes into UpdateVisualState so the row's
    // Disabled VSM activates when consumers toggle row IsEnabled at runtime.
    void OnIsEnabledChanged(
        const winrt::Windows::Foundation::IInspectable& sender,
        const winrt::Microsoft::UI::Xaml::DependencyPropertyChangedEventArgs& args);

    void RebuildCells();
    void ClearOwnedCellToolTips(const winrt::Panel& host);

    // Coalesces a burst of Columns-collection changes into a single cell rebuild on the next
    // dispatcher tick (each realized row observes Columns, so N bulk edits would otherwise cost N
    // full RebuildCells per row). Recycle / owner / DataContext paths still rebuild synchronously.
    void QueueRebuildCells();
    void UpdateVisualState(bool useTransitions);

    tracker_ref<winrt::Panel> m_cellsHost{ this };
    winrt::FrameworkElement::DataContextChanged_revoker m_dataContextChangedRevoker{};
    winrt::Control::IsEnabledChanged_revoker m_isEnabledChangedRevoker{};
    winrt::UIElement::GettingFocus_revoker m_gettingFocusRevoker{};
    winrt::UIElement::GotFocus_revoker m_gotFocusRevoker{};
    weak_ref<winrt::TableView> m_owningTableView{ nullptr };
    winrt::IObservableVector<winrt::TableViewColumn>::VectorChanged_revoker m_columnsVectorChangedRevoker{};
    weak_ref<winrt::IObservableVector<winrt::TableViewColumn>> m_observedColumns{};

    bool m_isPointerOver{ false };
    bool m_isPressed{ false };

    bool m_selectOnPointerRelease{ false };
    uint32_t m_selectPointerId{ 0 };

    // Prevent DataContextChanged re-entry while RebuildCells updates child DCs.
    bool m_isRebuildingCells{ false };

    bool m_rebuildCellsQueued{ false };

    tracker_ref<winrt::TableViewColumn> m_editingColumn{ this };
    tracker_ref<winrt::Grid> m_editingCellWrapper{ this };
    winrt::weak_ref<winrt::UIElement> m_pendingEditingCell{ nullptr };
    tracker_ref<winrt::FrameworkElement> m_editingElement{ this };
    tracker_ref<winrt::UIElement> m_editingDisplayElement{ this };
    winrt::weak_ref<winrt::TableViewCellAutomationPeer> m_editingAutomationPeer{ nullptr };
    tracker_ref<winrt::IInspectable> m_editingAutomationItem{ this };
    winrt::hstring m_editingAutomationValue;
    winrt::hstring m_editingAutomationName;

    // Begin-edit gesture state. Held per row rather than on the control: a double-click that starts
    // on one row and finishes on another is not a double-click, and per-row state makes that fall
    // out for free. Registered handler is kept alive so it can be removed on teardown.
    winrt::PointerEventHandler m_editingPointerPressedHandler{ nullptr };
    uint64_t m_lastPressTimestamp{ 0 };
    winrt::Point m_lastPressPosition{};
    winrt::Microsoft::UI::Input::PointerDeviceType m_lastPressDeviceType{ winrt::Microsoft::UI::Input::PointerDeviceType::Mouse };
    tracker_ref<winrt::TableViewColumn> m_lastPressColumn{ this };
    tracker_ref<winrt::IInspectable> m_lastPressItem{ this };
};
