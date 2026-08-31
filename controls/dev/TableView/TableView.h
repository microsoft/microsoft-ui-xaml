// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include <deque>
#include <functional>
#include <optional>

#include "TableView.g.h"
#include "TableView.properties.h"

// ThemeSettings (Microsoft.UI.System) is used below for UI-thread High Contrast change notifications.
// Included here (not the shared CppWinRTIncludes.h) to keep the rebuild scope local to TableView.
#include <winrt/Microsoft.UI.System.h>

// One in-flight resize drag. The gripper owns the gesture; this is only the host's anchor for it,
// kept reachable so Escape can cancel the drag in flight.
struct ColumnResizeDragState
{
    // Weak: the handlers that own this state are registered on the gripper itself, so a strong
    // reference here is a cycle the XAML reference tracker cannot see.
    winrt::weak_ref<winrt::ResizeGripper> gripper{ nullptr };
    double startValue{ 0.0 };
    // The column's Width as authored: reverting a canceled drag to startValue would rewrite an
    // Auto or Star column as fixed pixels.
    winrt::GridLength startWidth{};
    // Set once a DragDelta has actually written Width, so a canceled press that never moved
    // leaves the column completely untouched.
    bool didWrite{ false };
    // Set once any DragDelta arrived, even one the bounds swallowed. Drives the announcement, so a
    // press that never moved stays silent while a step held at a bound still reports the width.
    bool didDelta{ false };
};

// Per-instance cache of density metrics and the resolved gridline brush. Held as a
// TableView member (not a process-global map keyed by `this`) so instances on
// different UI threads never share or concurrently mutate one container. Grouped into
// small nested structs (DensityInfo/GridLineInfo) so each cached concern reads as one
// cohesive unit; nested-struct naming follows the controls/dev `*Info` convention
// (e.g. LinedFlowLayout::ItemsInfo, WebView2::XamlFocusChangeInfo).
struct TableViewResourceCache
{
    // Density-dependent metrics: resolved from Density-suffixed ThemeResource keys (see
    // DensitySuffix), so they change with the Density property. Cleared with the rest of the
    // cache by InvalidateTableViewResourceCache (density / theme / high-contrast changes).
    struct DensityInfo
    {
        bool hasRowMinHeight{ false };
        double rowMinHeight{ 0.0 };
        bool hasCellPadding{ false };
        winrt::Thickness cellPadding{};
        bool hasHeaderCellPadding{ false };
        winrt::Thickness headerCellPadding{};
    };
    DensityInfo density{};

    // Font sizes resolved from fixed (non-density-suffixed) ThemeResource keys, so they do NOT
    // vary with Density; kept out of DensityInfo to avoid implying otherwise. Still cached and
    // cleared by InvalidateTableViewResourceCache alongside the other resolved resources.
    struct FontInfo
    {
        bool hasCellFontSize{ false };
        double cellFontSize{ 0.0 };
        bool hasHeaderFontSize{ false };
        double headerFontSize{ 0.0 };
    };
    FontInfo font{};

    // Resolved gridline brush; re-resolved when the theme or high-contrast state changes.
    struct GridLineInfo
    {
        bool hasBrush{ false };
        winrt::ElementTheme theme{ winrt::ElementTheme::Default };
        bool highContrast{ false };
        winrt::Brush brush{ nullptr };
    };
    GridLineInfo gridLine{};

    // Cached horizontal scroll offset used to reposition frozen columns (not a theme resource,
    // so it is intentionally left out of the density/gridline groups above).
    bool hasLastFrozenColumnsHorizontalOffset{ false };
    double lastFrozenColumnsHorizontalOffset{ 0.0 };
};

class TableViewCellToolTipRequestedEventArgs;

namespace TabularShapingHelpers { class CustomSortRankAdapter; }

// The control's half of the TableViewSource sort axis. The projection is addressed by an opaque
// axis token, so re-sorting the same column replaces its axis rather than stacking a second one.
struct TableViewSourceSortBinding
{
    winrt::hstring MemberPath;
    winrt::hstring AxisToken;
    winrt::TableViewKeySelector KeySelector{ nullptr };
    // The rank adapter for a CustomSortComparer column. Owned by the control but implemented in the
    // shaping engine: the control feeds it the column's comparer, the engine turns that into the
    // integer sort keys the projection consumes.
    std::shared_ptr<TabularShapingHelpers::CustomSortRankAdapter> CustomSortState;

    // Drops any comparer and its ranks without discarding the selector: the selector closes over
    // the state by shared_ptr, so replacing it would orphan the live closure.
    void ResetCustomSort();
    void Clear();
};

class TableView :
    public ReferenceTracker<TableView, winrt::implementation::TableViewT>,
    public TableViewProperties
{
public:
    TableView();

    // IFrameworkElement overrides
    void OnApplyTemplate();
    winrt::AutomationPeer OnCreateAutomationPeer();

    // Property-changed callbacks (from TableViewProperties)
    void OnItemsSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnIsReadOnlyPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnHeadersVisibilityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnGridLinesVisibilityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnRowBackgroundPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnAlternatingRowBackgroundPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnEmptyTemplatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnDensityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    // Density resources fall back to Standard defaults; rows and columns call these via get_self.
    double GetDensityRowMinHeight();
    winrt::Thickness GetDensityCellPadding();
    winrt::Thickness GetDensityHeaderCellPadding();
    double GetCellFontSize();
    double GetHeaderFontSize();

    // Resolved grid-line brush (theme/HC-aware, cached); rows call this via get_self, like the
    // density/font accessors above.
    winrt::Brush GetGridLineBrush();

    // Per-instance resource cache (density metrics + gridline brush); accessed by the
    // file-scope resource helpers in TableView.cpp through this owner pointer.
    TableViewResourceCache& GetResourceCacheInternal() { return m_resourceCache; }

    // ActualTheme cannot report HC; cached AccessibilitySettings selects HC grid-line resources.
    bool IsHighContrast();

    // TableViewColumn calls this when header templates change so realized headers refresh.
    void RebuildHeaders();

    // Toggling it adds or removes every gripper, so the header band is rebuilt.
    void OnCanUserResizeColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    // The gripper owns the resize mechanics; this only positions it and forwards the pointer drag.
    void AppendResizeGripperVisual(
        const winrt::Grid& headerCell,
        const winrt::TableViewColumn& column,
        double gripperWidth,
        const winrt::hstring& headerText,
        winrt::HorizontalAlignment logicalEndAlignment);

    winrt::ResizeGripper FindResizeGripperInCell(const winrt::FrameworkElement& headerCell) const;

    // Idempotent; safe from any pointer end-event, the gripper's Unloaded, or TableView's own.
    void CancelColumnResizeDrag();
    void AnnounceColumnWidth(const winrt::IInspectable& announcer, const winrt::TableViewColumn& column);

    // Coalesces bursts of column-driven header rebuilds (bulk Columns edits, per-column Header /
    // FrozenEdge changes) into a single RebuildHeaders on the next dispatcher tick, so N column
    // mutations cost O(N) header builds instead of O(N^2). Skips entirely before the template is
    // applied (OnApplyTemplate performs the initial build). Falls back to a synchronous rebuild when
    // no dispatcher is available or the enqueue fails.
    void QueueRebuildHeaders();

    // Internal — invoked by TableViewColumn when its Visibility changes so
    // realized header and row cells stay in sync without rebuilding Columns.
    void OnColumnVisibilityChanged(const winrt::TableViewColumn& column);

    // Internal — invoked by TableViewColumn when Width / MinWidth / MaxWidth changes so the next
    // measure pass re-resolves column widths (ResolveColumnWidths then re-pins frozen columns and
    // refreshes gridline visuals from the resolved widths).
    void OnColumnWidthChanged(const winrt::TableViewColumn& column);

    // TableViewTemplateColumn calls this when CellTemplate changes so realized rows regenerate cells.
    void OnColumnCellTemplateChanged(const winrt::TableViewColumn& column);

    // TableViewColumn calls this when Header / HeaderTemplate / HeaderTemplateSelector changes so
    // headers re-render and Auto columns recompute their content width.
    void OnColumnHeaderChanged(const winrt::TableViewColumn& column);

    // TableViewColumn calls this when FrozenEdge changes so headers re-render and the leading-frozen
    // band re-pins.
    void OnColumnFrozenEdgeChanged(const winrt::TableViewColumn& column);

    // Pin rebuilt rows immediately when leading-frozen columns are active.
    void PinFrozenColumnsForRow(const winrt::TableViewRow& row);

    // Internal — true while at least one CellToolTipRequested handler is attached, so the cell
    // realization path can skip all per-cell tooltip work when nobody is listening.
    bool HasCellToolTipHandler() const { return static_cast<bool>(m_cellToolTipRequestedEventSource); }

    // Internal — raises CellToolTipRequested for one cell and returns the args the handler filled in.
    winrt::com_ptr<TableViewCellToolTipRequestedEventArgs> RaiseCellToolTipRequested(
        const winrt::TableViewColumn& column,
        const winrt::IInspectable& item);

    // Queues a coalesced tooltip re-resolve for every realized cell (public, from the IDL).
    void InvalidateCellToolTips();

    // Requested by a cell panel (header/row) during measure when a realized cell's own measured width
    // changed (grow or shrink). Invalidates our measure synchronously so ResolveColumnWidths re-runs in
    // the same layout tick (no deferral -> no one-frame lag). Bridges the body ScrollViewer, which
    // absorbs the cell's own measure invalidation. Converges without a debounce (see the definition).
    void RequestColumnWidthResolve();

    // Automation peer accessors; weak refs may be null before OnApplyTemplate.
    winrt::ItemsRepeater GetRowsRepeaterInternal() const { return m_rowsRepeater.get(); }
    winrt::Panel GetHeaderHostInternal() const { return m_headerHost.get(); }
    int32_t GetRowCountInternal() const { return GetItemsSourceCount(); }
    winrt::ScrollViewer GetBodyScrollerInternal() const { return m_bodyScroller.get(); }

    // Test hook for moving keyboard focus to a row; false for invalid indexes or before rows exist.
    bool FocusRow(int32_t index);

    // IFrameworkElement override. Must be PUBLIC: C++/WinRT dispatches overrides through a base
    // subobject that can only reach public members; a protected override is silently never called.
    winrt::Size MeasureOverride(winrt::Size const& availableSize);

    // ----- Editing (TableView_Editing.cpp) -----

    // Scope of an edit close. Internal only: the public surface is cell-scoped in this release, but
    // the row scope is real - moving to a different item must end that item's transaction - and the
    // plumbing is kept so row editing can be added without re-threading every signature.
    enum class EditingUnit
    {
        Cell,
        Row,
    };

    winrt::IInspectable CurrentItem();
    winrt::TableViewColumn CurrentColumn();
    void SetCurrentCell(winrt::IInspectable const& item, winrt::TableViewColumn const& column);


    bool IsEditing() const noexcept { return m_editState != EditState::None; }

    // The editor currently in the tree, or null. Internal: the cell automation peer needs it to
    // route IValueProvider.SetValue through the public edit lifecycle.
    winrt::FrameworkElement CurrentEditingElement() const;

    // Turns a row's container-level item into the object an edit writes to. Identity today; the
    // seam a wrapping layer (grouping) changes in one place. Public so TableViewRow's gesture
    // handler resolves the edit target exactly the way the control does - when those disagreed,
    // begin-edit failed on every row.
    winrt::IInspectable UnwrapEditingDataItem(winrt::IInspectable const& item) const;

    // Identity comparison that survives boxing and re-projection. Raw IInspectable equality is not
    // reliable for boxed values, so anything comparing data items must use this.
    static bool SameInspectableIdentity(winrt::IInspectable const& lhs, winrt::IInspectable const& rhs);

    bool BeginEdit();
    bool BeginEdit(winrt::IInspectable const& item, winrt::TableViewColumn const& column);
    bool CommitEdit();
    bool CommitEditInternal(EditingUnit unit);
    bool CancelEdit();
    bool CancelEditInternal(EditingUnit unit);

    // Forced teardown (source-driven reset / ItemsSource replacement / unload). Cannot be vetoed.
    bool TerminateEditForReset(bool force);
    // Same, but leaves the edited row's visuals alone. For callers inside a layout pass, where
    // restoring the display child would mutate the tree during measure.
    //
    // insideLayoutPass additionally defers the app-visible NOTIFICATIONS (CellEditEnding /
    // EditEnded) onto the dispatcher. Raising them synchronously runs app code inside
    // ItemsRepeater's measure, and a handler that touches the tree or invalidates layout
    // re-enters the pass we are standing in and fail-fasts. Pass true only from a genuine layout
    // pass - the DP-change callers are re-entrant for FOCUS reasons, not layout ones.
    bool TerminateEditWithoutVisualRestore(bool insideLayoutPass = false);

    // Runs an app-visible notification now, or on the dispatcher when we are inside a layout pass.
    void PostEditNotification(std::function<void()> notify);

    // Closes an open edit when the column it is on becomes read-only. Called by
    // TableViewColumn::OnPropertyChanged, which cannot reach the edit state itself.
    void OnColumnIsReadOnlyChanged(winrt::TableViewColumn const& column);
    // Control-initiated reshape (sort / group / expand). Stays cancelable.
    bool TryTerminateEditForControlInitiatedReshape();

    // Defer a source-reshaping operation until the open edit closes. Opaque action, so editing
    // needs no compile-time knowledge of sorting, grouping or expansion.
    void QueueCoalescedEditReshape(std::function<void()> operation);
    void ClearCoalescedEditReshape();
    void DrainCoalescedEditReshape();

    // ----- Editing input gestures (TableView_EditingInput.cpp) -----
    // The pointer gesture lives on TableViewRow: the row owns its cells, so it is the level that
    // can resolve which cell a press landed on.

    void OnKeyDownForEditing(
        const winrt::IInspectable& sender,
        const winrt::KeyRoutedEventArgs& args);
    void OnLosingFocusForEditing(
        const winrt::IInspectable& sender,
        const winrt::Microsoft::UI::Xaml::Input::LosingFocusEventArgs& args);
    void CompleteFocusLossCommit();

    // ----- Selection (TableView_Selection.cpp) -----
    // SelectionModel owns the selected index and reconciles it across collection changes; this
    // control keeps the DP projections, the row chrome and the gestures. The DPs are pushed and
    // never read back, because a DP write notifies synchronously.

    void OnSelectionModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void Select(int32_t index);
    void Deselect(int32_t index);
    bool IsSelected(int32_t index);
    void DeselectAll();

    // Centralises the mode gate only. Modifier-aware routing (interacted vs focused, per
    // SelectorBase) is what Multiple/Extended will add on top.
    bool CanSelectRows();

    // Shared by the pointer gesture, keyboard navigation and the row peer. No-op when off.
    // The one-arg form reads Ctrl live and toggles; automation passes toggle=false, because UIA
    // Select() means "make this the selection", never "clear it".
    void SelectRowIndexFromInteraction(int32_t index);
    void SelectRowIndexFromInteraction(int32_t index, bool toggle);

    // The row sees the press first (it owns its cells); selection state lives on the control.
    void OnRowPointerSelect(winrt::TableViewRow const& row);

    // Re-derives IsSelected for a realized or re-indexed row; it never survives recycling.
    void RefreshRowSelectionState(winrt::TableViewRow const& row);
    void RefreshRowSelectionState(winrt::TableViewRow const& row, int32_t selectedIndex);

    // For the automation peers, which cannot reach the private members. Both read the model.
    int32_t SelectedIndexInternal() const;
    winrt::IInspectable SelectedItemInternal() const;
    // The peer resolves the row index of its header through the repeater rather than a tree walk.
    winrt::ItemsRepeater GetRowsRepeaterForPeer() const { return m_rowsRepeater.get(); }

    // --- Sorting (TableView_Sort.cpp) ---
    // Sorting is single-column, and the active state lives on the column: read
    // TableViewColumn.SortDirection, which is the column a Sorted handler is handed. There is
    // deliberately no control-level SortColumn/SortDirection pair mirroring it, matching WPF's
    // DataGrid, which also keeps sort state on DataGridColumn and exposes no control-level
    // equivalent.
    bool SortByColumn(const winrt::TableViewColumn& column, winrt::SortDirection direction);
    bool ToggleSortDirection(const winrt::TableViewColumn& column);
    bool ClearSort();

    // Republishes every realized header chevron from its column's SortDirection DP. A push rather
    // than a binding: SortIndicatorDirection and SortDirection are distinct WinRT enums, so a
    // {Binding} between them silently does nothing. Called by TableViewColumn.
    void RefreshSortIndicators();
    // CanSort gates whether the chevron is built at all, so a runtime flip needs a header rebuild.
    void OnColumnCanSortChanged(const winrt::TableViewColumn& column);
    void OnCanUserSortColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    // Drops a column that has left Columns from the active sort state. Returns true when the sort
    // state changed.
    bool PurgeColumnFromSortState(const winrt::TableViewColumn& removedColumn);

private:
    // Drives the SelectionModel; its SelectionChanged is the single funnel that publishes.
    void ApplySelection(int32_t index);

    // Created lazily so a TableView that never selects pays nothing.
    void EnsureSelectionModel();
    // SelectionModel::Source has no identity short-circuit - re-setting the same source would drop
    // the selection - so this only writes when it actually differs.
    void UpdateSelectionModelSource();
    void OnSelectionModelSelectionChanged(
        const winrt::SelectionModel& sender,
        const winrt::SelectionModelSelectionChangedEventArgs& args);

    // Index of `item` in the ItemsSource, by identity; -1 when absent or the source is not set.
    // Still needed because SelectionModel indexes but does not look items up.
    int32_t IndexOfItem(winrt::IInspectable const& item) const;

    // The selected item for an already-read index, so a caller that has one does not rebuild the
    // model's IndexPath just to resolve it again.
    winrt::IInspectable SelectedItemForIndex(int32_t index) const;

    // The realized container for a data index, or null.
    winrt::TableViewRow FindRealizedRowForIndex(int32_t index);

    void PushSelectionProperties();
    void RaiseSelectionChanged(winrt::IInspectable const& addedItem);
    void RaiseSelectionAutomationEvents(winrt::TableViewRow const& deselectedRow, winrt::TableViewRow const& selectedRow);
    // Re-derives IsSelected on every realized row. Needed after a collection change, where the
    // repeater has already re-indexed its containers.
    void RestampAllRealizedRowSelection();
    void RestampAllRealizedRowSelection(int32_t selectedIndex);

    // Subscribed only to restamp rows: SelectionModel handles the index reconciliation itself, and
    // does not raise when an insert merely shifts the selected index.
    void UpdateSelectionCollectionChangedSubscription();
    void OnSelectionItemsSourceCollectionChanged(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    // Re-points the model at a new ItemsSource and re-selects anything held across a reload.
    void ResolveSelectionAfterSourceChange();
    // Unload drains the repeater's source; re-sourcing on load clears the model. Hold the selected
    // item across that round trip so an unload/reload cycle does not drop the selection.
    void StashSelectionForReload();
    bool HasRowsSource() const;

    // True when selection cannot be resolved yet - selection off, or no source.
    bool ShouldDeferSelectionRequest();
    void ClearPendingSelection();
    bool DrainPendingSelection();

    winrt::SelectionModel m_selectionModel{ nullptr };
    winrt::SelectionModel::SelectionChanged_revoker m_selectionModelChangedRevoker{};

    // Last index published to the rows, so a change knows which container to unstamp and which to
    // raise the UIA "removed from selection" event on.
    int32_t m_lastPublishedIndex{ -1 };

    // Last item REPORTED through SelectionChanged; the delta derives from this.
    tracker_ref<winrt::IInspectable> m_lastRaisedSelectedItem{ this };

    // The selected item, held across an unload/reload round trip so re-sourcing the repeater does
    // not drop it. Nothing else defers.
    tracker_ref<winrt::IInspectable> m_pendingSelectedItem{ this };

    // Bumped on every publish so a re-entrant one can tell that a newer selection overtook it and
    // it must not finish its own (now stale) notification.
    uint32_t m_selectionVersion{ 0 };

    // Set while a stashed selection is being restored after a reload, so the clear-then-reselect
    // that SelectionModel::Source forces is published once at the end rather than as two events.
    bool m_isRestoringSelection{ false };

    winrt::ItemsSourceView::CollectionChanged_revoker m_selectionCollectionChangedRevoker{};

private:
    // Explicit edit lifecycle, replacing four independent booleans whose 16 nominal combinations
    // encoded the real invariants only in the ordering of guards spread across five methods.
    //
    //   None      -> no edit open.
    //   Beginning -> inside BeginningEdit; reentrant edit operations are rejected.
    //   Editing   -> edit open and interactive.
    //   Ending    -> inside CellEditEnding/RowEditEnding, or waiting on a deferral one took.
    //
    // Legal: None->Beginning->{None, Editing}; Editing->Ending->{Editing, None}, where
    // Ending->Editing is the veto/validation-failure path that keeps the edit open.
    enum class EditState
    {
        None,
        Beginning,
        Editing,
        Ending,
    };

    EditState m_editState{ EditState::None };

    // True once this edit has written to the data item. A validation failure keeps the edit open
    // AFTER the write, so a later cancel has to push the pre-edit value back to the source rather
    // than only restoring the editor.
    bool m_editSourceWritten{ false };

    // True while a forced teardown arrived during the Beginning window, where EndCurrentEdit cannot
    // close the edit. BeginEdit re-checks it and unwinds instead of promoting to Editing over a row
    // that has already been recycled onto a different item.
    bool m_abandonPendingBeginEdit{ false };

    // True only while a queued reshape is replaying, so the drain is not re-entered by it.
    bool m_isApplyingCoalescedEditReshape{ false };

    // Bumped when a forced teardown closes an edit awaiting a deferral, so a late completion can
    // recognise itself as stale.
    uint32_t m_editGeneration{ 0 };

    // Pending source-reshaping operations. A deque, not a vector: the drain pops from the front so
    // replay order matches arrival order.
    std::deque<std::function<void()>> m_pendingEditReshapes;

    // Keyboard/focus position. Deliberately NOT redefined while an edit is open, so a value read
    // inside an EditEnding handler stays valid once the edit closes.
    tracker_ref<winrt::IInspectable> m_currentItem{ this };
    tracker_ref<winrt::TableViewColumn> m_currentColumn{ this };

    // True while a teardown must not touch the edited row's visuals (recycle / rebuild paths).
    bool m_suppressEditVisualRestore{ false };
    // True only while tearing down from inside ItemsRepeater's measure/arrange. Distinct from
    // m_suppressEditVisualRestore, which is also set by DP-change callbacks where the hazard is
    // focus re-entrancy rather than layout re-entrancy.
    bool m_insideLayoutPass{ false };

    void SetCurrentItem(winrt::IInspectable const& item);
    void UpdateCurrentColumn(winrt::TableViewColumn const& column);

    // Shared tail of the synchronous and deferred edit-close paths.
    bool CompleteEditEnd(EditingUnit unit, winrt::TableViewEditAction action, bool honorCancel, bool vetoed);
    // The cell being edited. Distinct from m_currentItem/m_currentColumn, which track focus.
    tracker_ref<winrt::IInspectable> m_currentEditItem{ this };
    tracker_ref<winrt::TableViewColumn> m_currentEditColumn{ this };
    tracker_ref<winrt::TableViewRow> m_currentEditRow{ this };

    // Whatever the column's PrepareCellForEdit handed back, returned to it on cancel.
    tracker_ref<winrt::IInspectable> m_editUneditedValue{ this };

    bool RaiseBeginningEdit(winrt::IInspectable const& item, winrt::TableViewColumn const& column);
    // Returns Vetoed or Completed. Synchronous: there is no deferral in this release, so a handler
    // must set Cancel before it returns.
    enum class EditEndingResult { Vetoed, Completed };
    EditEndingResult RaiseEditEnding(
        EditingUnit unit,
        winrt::TableViewEditAction action,
        bool honorCancel);

    bool TryResolveFocusedCell(winrt::IInspectable& item, winrt::TableViewColumn& column);
    bool TryResolveCurrentCell(winrt::IInspectable& item, winrt::TableViewColumn& column);
    bool TryResolveCurrentCellForEdit(winrt::IInspectable& item, winrt::TableViewColumn& column);
    bool TryGetItemAtRowIndex(int32_t rowIndex, winrt::IInspectable& item) const;
    winrt::TableViewRow FindRealizedRowForItem(winrt::IInspectable const& item);
    bool TryBeginEditVisual(winrt::IInspectable const& item, winrt::TableViewColumn const& column);

    void EndEditVisual(winrt::TableViewEditAction action);

    // Applies the outcome of an edit close: writes or reverts, tears down the visual, clears state.
    // Shared by the synchronous and deferred paths so they cannot drift.
    bool FinishEditTeardown(EditingUnit unit, winrt::TableViewEditAction action, bool honorCancel);
    bool EndCurrentEdit(EditingUnit unit, winrt::TableViewEditAction action, bool honorCancel);

    // Scoped to the property the edited column writes; falls back to the object-level check only
    // when the column reports no single editing property path.
    bool HasBlockingValidationErrors(
        winrt::IInspectable const& item,
        winrt::TableViewColumn const& column) const;

private:
    void OnColumnsVectorChanged(
        const winrt::IObservableVector<winrt::TableViewColumn>& sender,
        const winrt::IVectorChangedEventArgs& args);

    void OnRowElementPrepared(
        const winrt::ItemsRepeater& sender,
        const winrt::ItemsRepeaterElementPreparedEventArgs& args);

    void OnRowElementClearing(
        const winrt::ItemsRepeater& sender,
        const winrt::ItemsRepeaterElementClearingEventArgs& args);

    void OnRowElementIndexChanged(
        const winrt::ItemsRepeater& sender,
        const winrt::ItemsRepeaterElementIndexChangedEventArgs& args);

    // Body horizontal scrolling drives the header ScrollViewer so headers track row cells.
    void OnBodyScrollerViewChanged(
        const winrt::IInspectable& sender,
        const winrt::ScrollViewerViewChangedEventArgs& args);

    // Defer ScrollViewer ancestor lookup until Loaded because ScrollViewer template names are shadowed.
    void OnHeaderHostLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnRowsRepeaterLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);

    // ThemeSettings must be created once a XamlRoot/WindowId is available (Loaded); its Changed handler
    // refreshes HC-dependent resources directly on the UI thread (Changed is raised there).
    void OnTableViewLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnThemeSettingsChanged(const winrt::Microsoft::UI::System::ThemeSettings& sender, const winrt::IInspectable& args);

    void UpdateHeaderVisibility();
    void ApplyGridLinesToHeader();
    void RefreshGridLinesOnRealizedRows();
    void RefreshRowBackgroundsOnRealizedRows();

    // Invoke fn for each realized row in PART_RowsRepeater. Centralizes the "enumerate realized
    // rows" walk shared by the density / gridline / frozen-column / column-change update paths
    // (and the future grouping seam), so those callers don't each re-implement the repeater walk.
    void ForEachRealizedRow(std::function<void(winrt::TableViewRow const&)> const& fn);

    // Reset/refill the tracked-column owner back-pointers (shared by the Columns-replaced and
    // vector-Reset paths). Detach clears m_trackedColumns; Track refills it (no-op if null).
    void DetachAllColumnOwners();
    void TrackColumnsFromVector(winrt::IObservableVector<winrt::TableViewColumn> const& columns);
    bool ShouldShowColumnHeaders();
    int32_t GetItemsSourceCount() const;

    // Split responsibilities driven off the ItemsSource DP:
    //   AdoptItemsSource   - source lifetime. Runs only when ItemsSource actually changes: normalize
    //                        a plain collection into a control-owned TableViewSource, detach the
    //                        previous source, adopt the new one, and wire its owner + handlers.
    //   RefreshRowsPipeline- pushes the active source's view into the repeater (identity-guarded)
    //                        and re-resolves empty-state + selection. Runs on every re-entry
    //                        (template applied, repeater reloaded, shaping verb) with no lifetime work.
    void AdoptItemsSource();
    void RefreshRowsPipeline();
    // Raised by the bound TableViewSource after a shaping verb rewrote the projection. A
    // programmatic reshape has no input event behind it, so this is the only thing that tells a
    // UIA client its cached rows are stale. reorderOnly separates a pure re-sort (same children,
    // new order) from a membership change.
    void OnTableViewSourceShapingChanged(bool reorderOnly);
    // EmptyTemplate shows only for null or empty row sources.
    void UpdateEmptyState();
    void UpdateEmptyStateCollectionChangedSubscription();
    void OnEmptyStateItemsSourceCollectionChanged(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    winrt::event_token m_columnsVectorChangedToken{};

    // --- TableViewSource binding ---
    //
    // The projection the rows are driven from. For a bound TableViewSource this IS the source's
    // ItemsSourceView; otherwise it is the raw ItemsSource wrapped. Cached because the shape can
    // be swapped underneath us by a shaping verb.
    winrt::ItemsSourceView m_rowsItemsSourceView{ nullptr };

    // The single active source, whether the app assigned it or the control synthesized it over a
    // plain ItemsSource. Held strongly through a tracker_ref: TableViewSource is a ReferenceTracker,
    // so this edge is visible to the GC's cross-boundary cycle walker (the same reason ItemsRepeater
    // holds its ItemsSourceView by tracker_ref) and double-retention of an app-assigned source
    // alongside the ItemsSource DP cannot leak. Also used to detach the previous source on a swap:
    // binding a different source must clear the old one's owner and handlers, or a source still
    // subscribed to the app's collection keeps driving a control it no longer belongs to. Released
    // when ItemsSource changes to null / a different source; the source keeps only a weak
    // back-pointer to the owner, so this is not a hard cycle. Reassigned exclusively by
    // AdoptItemsSource, so every other path reads it as a stable answer rather than re-deriving it.
    tracker_ref<winrt::TableViewSource> m_activeSource{ this };

    // --- Sorting ---
    //
    // Re-validated after every point where app code could have run (a Sorting handler, or an
    // edit-ending handler): the columns collection may have changed underneath the request that is
    // still in flight.
    bool IsSortRequestStillValid(const winrt::TableViewColumn& column) const;
    bool IsSortClearStillValid() const;
    // The source the rows are projected through, app-assigned or synthesized. Non-null means the
    // control can reshape the rows itself.
    winrt::TableViewSource ShapingSourceInternal() const;
    // Writes the single-column sort state into the columns; does not reshape.
    void ApplySingleColumnSortState(const winrt::TableViewColumn& column, winrt::SortDirection direction);
    // Applies the current sort state to the bound TableViewSource. Returns false when there is no
    // TableViewSource, or the trigger column resolves no sort key.
    bool SyncTableViewSourceSort(const winrt::TableViewColumn& trigger, winrt::SortDirection direction);
    winrt::TableViewKeySelector GetTableViewSourceSortKeySelector(const winrt::hstring& sortMemberPath);
    bool RaiseSortingAndCheckCanceled(const winrt::TableViewColumn& trigger, winrt::SortDirection direction);
    // Single funnel for "the sort state has been written to the columns": reshapes, restores the
    // selection, raises Sorted, and announces.
    void RecomputeSortDPsAndRaiseInternal(const winrt::TableViewColumn& trigger);

    // Silently drops the active sort when the data set is replaced. See the definition for why this
    // is not ClearSort.
    void ResetSortStateForNewItemsSource();
    // Projection index of a data item, or -1. Used to carry the selection across a re-sort.
    int32_t FindEntryIndexForDataItem(const winrt::IInspectable& item) const;
    void AnnounceSortChange(const winrt::hstring& announcement);
    // The active sort column left Columns. Reshaping inside the VectorChanged callback would
    // re-enter the collection that is still mutating, so the reshape runs on the next turn.
    void QueueClearSortAfterColumnRemoval();
    bool m_clearSortAfterColumnRemovalQueued{ false };
    void AppendSortIndicatorVisual(const winrt::Panel& host, const winrt::TableViewColumn& column);
    static winrt::SortIndicatorDirection ToSortIndicatorDirection(winrt::SortDirection direction);

    // v1 is single-column sort, so this holds at most one entry. It stays a vector because the
    // clear/purge walks are written against the collection and multi-column sort is the expected
    // next step. Weak refs: a column removed from Columns must not be kept alive by sort state.
    std::vector<tracker_ref<winrt::TableViewColumn>> m_sortedColumns;
    TableViewSourceSortBinding m_tableViewSourceSort;

    tracker_ref<winrt::ItemsRepeater> m_rowsRepeater{ this };
    tracker_ref<winrt::ContentControl> m_emptyStatePresenter{ this };
    tracker_ref<winrt::FrameworkElement> m_headerRow{ this };
    tracker_ref<winrt::Panel> m_headerHost{ this };
    tracker_ref<winrt::ScrollViewer> m_headerScroller{ this };
    // Keeps the header band locked to the body when focus moves to an off-screen header.
    winrt::UIElement::BringIntoViewRequested_revoker m_headerBringIntoViewRevoker{};
    tracker_ref<winrt::ScrollViewer> m_bodyScroller{ this };

    winrt::event_token m_rowElementPreparedToken{};
    winrt::event_token m_rowElementClearingToken{};
    winrt::event_token m_rowElementIndexChangedToken{};
    winrt::event_token m_bodyScrollerViewChangedToken{};
    // Body-viewport resize invalidates measure so Star columns resolve during the table layout pass.
    winrt::FrameworkElement::SizeChanged_revoker m_bodyScrollerSizeChangedRevoker{};
    winrt::event_token m_headerHostLoadedToken{};
    // Set while a drag is in flight, so Escape can reach the gripper that owns it.
    std::shared_ptr<ColumnResizeDragState> m_activeColumnResizeDrag{};
    winrt::event_token m_rowsRepeaterLoadedToken{};
    winrt::event_token m_pendingFocusLayoutToken{};
    winrt::ItemsSourceView::CollectionChanged_revoker m_emptyStateCollectionChangedRevoker{};
    // ActualThemeChanged refreshes imperatively-resolved brushes that ItemsRepeater rows do not re-pump.
    winrt::event_token m_actualThemeChangedToken{};

    // ThemeSettings (lifted WinUI3) reports the system High Contrast setting and raises Changed on the
    // control's UI thread -- unlike AccessibilitySettings.HighContrastChanged, which could be delivered
    // off-thread. It requires a WindowId, so it is created on Loaded (once a XamlRoot exists), not in
    // the constructor, and torn down on Unloaded.
    winrt::Microsoft::UI::System::ThemeSettings m_themeSettings{ nullptr };
    winrt::Microsoft::UI::System::ThemeSettings::Changed_revoker m_themeSettingsChangedRevoker{}; // Runtime HC toggles must refresh cached HC-dependent brushes.
    // Cached HC state: kept fresh by ThemeSettings.Changed while loaded and read by IsHighContrast().
    // Only touched on the UI thread now, so no atomic is required.
    bool m_isHighContrast{ false };
    winrt::FrameworkElement::Loaded_revoker m_loadedRevoker{};

    // Unloaded drains repeater and body-scroller state before deferred callbacks hit a detached subtree.
    winrt::FrameworkElement::Unloaded_revoker m_unloadedRevoker{};
    void OnTableViewUnloaded();
    bool m_rowsSourceDrained{ false };

    // Leading-frozen columns are offset against horizontal scroll and clipped out of non-frozen cells.
    double ComputeLeadingFrozenWidth();
    void RefreshFrozenColumns();

    // Column-width layout engine internals (TableView_Layout.cpp).
    // GetHeaderMeasuredWidthForColumn encapsulates the header host's concrete panel type so the
    // layout engine pulls the header's measured width through a TableView seam (symmetric with
    // TableViewRow::MeasuredWidthForColumn) instead of casting to TableViewCellsPanel itself.
    double GetHeaderMeasuredWidthForColumn(const winrt::TableViewColumn& column) const;
    // ResolveColumnWidths runs the Pixel/Auto/Star pass (invoked from MeasureOverride once the
    // template subtree has measured), pulling cached measured widths from the header host and realized
    // rows before writing ActualWidth to each column.
    void ResolveColumnWidths();
    // ResetColumnDesiredWidths clears the grow-only Auto desired-width accumulators on data-set
    // boundaries (ItemsSource / Columns replaced / CellTemplate / Header) and invalidates measure so
    // the next table-level pass re-pulls fresh measured widths.
    void ResetColumnDesiredWidths();
    // Re-invalidate the header + realized row cells panels so they re-measure/arrange after a resolve.
    void InvalidateCellPanels();

    // Latches frozen-column state so transforms and clips clear exactly once when disabled.
    bool m_frozenColumnsActive{ false };

    // Set while a coalesced RebuildHeaders is pending on the dispatcher; collapses a burst of column
    // changes into one rebuild. UI-thread only (all column callbacks arrive on the UI thread).
    bool m_rebuildHeadersQueued{ false };
    void RefreshCellToolTipsOnRealizedRows();
    // A coalesced cell-tooltip re-resolve is pending on the dispatcher (InvalidateCellToolTips).
    bool m_cellToolTipRefreshQueued{ false };
    // An invalidate arrived while a pass was queued or running; coalesced into one follow-up.
    bool m_cellToolTipRefreshDirty{ false };
    // Consecutive handler-requested follow-up passes, capped so an unconditionally-invalidating
    // handler is dropped rather than pegging the UI thread.
    int m_cellToolTipRefreshPasses{ 0 };

    // Per-instance resource cache; replaces the former process-global map keyed by `this`.
    TableViewResourceCache m_resourceCache{};

    // Mirrors Columns so removals can clear a column's OwningTableView back-pointer.
    std::vector<tracker_ref<winrt::TableViewColumn>> m_trackedColumns;

    // Bubbling KeyDown lets focused descendants handle input before row navigation.
    winrt::KeyEventHandler m_keyDownHandler{ nullptr };  // Root KeyDown (handledEventsToo); registration is released with the element, no explicit RemoveHandler needed.
    void OnKeyDownForNavigation(
        const winrt::IInspectable& sender,
        const winrt::KeyRoutedEventArgs& args);

    // Left/Right resize for the column whose header has focus; the gripper is a pointer
    // affordance here, not a tab stop.
    bool TryHandleHeaderColumnResizeKey(const winrt::KeyRoutedEventArgs& args);
    // Redirects a header's bring-into-view onto the body scroller, so the header cannot scroll
    // independently of the columns it labels.
    void OnHeaderBringIntoViewRequested(const winrt::BringIntoViewRequestedEventArgs& args);

    // Tunneling PreviewKeyDown captures the focused row BEFORE the framework's built-in focus
    // navigation moves it (and marks the key Handled), so OnKeyDownForNavigation can anchor on the
    // pre-move row and advance exactly one row instead of doubling up with the built-in move.
    winrt::KeyEventHandler m_previewKeyDownHandler{ nullptr };

    // Editing gesture handlers; the registration is released with the element, so no RemoveHandler.
    winrt::KeyEventHandler m_editingKeyDownHandler{ nullptr };
    winrt::UIElement::LosingFocus_revoker m_editingLosingFocusRevoker{};

    // Set while a focus-loss commit check is queued, so a burst of focus changes produces one
    // re-evaluation rather than one commit attempt each.
    bool m_focusLossCommitQueued{ false };

    int32_t m_navAnchorRow{ -1 };
    void OnPreviewKeyDownForNavigation(
        const winrt::IInspectable& sender,
        const winrt::KeyRoutedEventArgs& args);

    // Keyboard navigation helpers.
    int32_t GetFocusedRowIndex() const;
    int32_t GetEstimatedRowsPerPage(); // Non-const — GetDensityRowMinHeight() mutates the resource cache.
};
