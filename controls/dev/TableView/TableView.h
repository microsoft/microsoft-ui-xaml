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
#include "TableViewRowInfo.h"

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
    winrt::GridLength startWidth{};
    bool didWrite{ false };
    bool didDelta{ false };
};

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

namespace ShapingHelpers { class CustomSortRankAdapter; }
class GroupedEntry;

struct TableViewSourceSortBinding
{
    winrt::hstring MemberPath;
    winrt::hstring AxisToken;
    winrt::TableViewKeySelector KeySelector{ nullptr };
    std::shared_ptr<ShapingHelpers::CustomSortRankAdapter> CustomSortState;

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
    // Drops the recycle pools the row-template selector caches. Those pools close a
    // repeater -> template-wrapper -> selector -> template -> pool -> repeater cycle through plain
    // C++ references the reference tracker cannot walk, so nothing here is collected unless the
    // one edge we own is cut. See TableViewRowTemplateSelector::Detach.
    ~TableView();

    void OnApplyTemplate();
    winrt::AutomationPeer OnCreateAutomationPeer();

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

    void RebuildHeaders();
    void OnCanUserResizeColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

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

    void QueueRebuildHeaders();

    void OnColumnVisibilityChanged(const winrt::TableViewColumn& column);

    // Internal — invoked by TableViewColumn when Width / MinWidth / MaxWidth changes so the next
    // measure pass re-resolves column widths (ResolveColumnWidths then re-pins frozen columns and
    // refreshes gridline visuals from the resolved widths).
    void OnColumnWidthChanged(const winrt::TableViewColumn& column);

    void OnColumnCellTemplateChanged(const winrt::TableViewColumn& column);

    void OnColumnHeaderChanged(const winrt::TableViewColumn& column);

    void OnColumnHeaderToolTipChanged(const winrt::TableViewColumn& column);

    void OnColumnFrozenEdgeChanged(const winrt::TableViewColumn& column);

    void PinFrozenColumnsForRow(const winrt::TableViewRow& row);

    void RequestColumnWidthResolve();

    // Automation peer accessors; weak refs may be null before OnApplyTemplate.
    winrt::ItemsRepeater GetRowsRepeaterInternal() const { return m_rowsRepeater.get(); }
    winrt::Panel GetHeaderHostInternal() const { return m_headerHost.get(); }
    int32_t GetRowCountInternal() const { return GetItemsSourceCount(); }
    winrt::ScrollViewer GetBodyScrollerInternal() const { return m_bodyScroller.get(); }

    // Test hook for moving keyboard focus to a row; false for invalid indexes or before rows exist.
    // Focus lands on the row's CURRENT CELL, which is what the control navigates in.
    bool FocusRow(int32_t index);


    // Moves keyboard focus to a cell by row index and VISIBLE column index, realizing and scrolling
    // the row into view first. A negative column means "keep the current one". Group-header rows
    // have no cells and fall back to focusing the header container.
    bool FocusCell(int32_t rowIndex, int32_t visibleColumnIndex);

    // The cell a row should hand focus to when focus is aimed at the row CONTAINER. Used by
    // TableViewRow's GettingFocus redirect: entering the table from outside returns to the cell the
    // user left, while a move inside the table keeps the current column on the requested row.
    // Null when the row has no realized visible cell, in which case the row keeps container focus.
    winrt::UIElement ResolveFocusEntryCell(
        winrt::TableViewRow const& row, winrt::DependencyObject const& oldFocusedElement);

    void OnRowCellFocusChanged(winrt::TableViewRow const& row);

    bool TryGetFocusedCell(int32_t& rowIndex, int32_t& columnIndex, bool requireExactCell) const;

    // Focuses the cell at a visible-column index inside an ALREADY realized container. Group
    // headers share the repeater and have no cells, so they keep taking container focus.
    bool FocusRealizedRowCell(winrt::UIElement const& element, int32_t visibleColumnIndex);

    bool IsWithinThisTableView(winrt::DependencyObject const& element);

    winrt::Size MeasureOverride(winrt::Size const& availableSize);


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

    void PostEditNotification(std::function<void()> notify);

    void OnColumnIsReadOnlyChanged(winrt::TableViewColumn const& column);
    bool TryTerminateEditForControlInitiatedReshape();

    void QueueCoalescedEditReshape(std::function<void()> operation);
    void ClearCoalescedEditReshape();
    void DrainCoalescedEditReshape();


    void OnKeyDownForEditing(
        const winrt::IInspectable& sender,
        const winrt::KeyRoutedEventArgs& args);
    void OnLosingFocusForEditing(
        const winrt::IInspectable& sender,
        const winrt::Microsoft::UI::Xaml::Input::LosingFocusEventArgs& args);
    void CompleteFocusLossCommit();


    void OnSelectionModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void Select(int32_t index);
    void Deselect(int32_t index);
    bool IsSelected(int32_t index);
    void DeselectAll();

    bool CanSelectRows();

    // Shared by the pointer gesture, keyboard navigation and the row peer. No-op when off.
    // The one-arg form reads Ctrl live and toggles; automation passes toggle=false, because UIA
    // Select() means "make this the selection", never "clear it".
    void SelectRowIndexFromInteraction(int32_t index);
    void SelectRowIndexFromInteraction(int32_t index, bool toggle);
    void SelectRowIndexFromKeyboardFocus(int32_t index);

    void OnRowPointerSelect(winrt::TableViewRow const& row);

    void RefreshRowSelectionState(winrt::TableViewRow const& row);
    void RefreshRowSelectionState(winrt::TableViewRow const& row, int32_t selectedIndex);

    // For the automation peers, which cannot reach the private members. Both read the model.
    int32_t SelectedIndexInternal() const;
    winrt::IInspectable SelectedItemInternal() const;
    TableViewRowKind GetRowKindForItem(winrt::IInspectable const& item) const;
    winrt::hstring GetGroupHeaderNameCandidate(GroupedEntry const& entry);
    bool TryGetTableViewSourceRowInfo(int32_t rowIndex, TableViewRowInfo& rowInfo) const;
    bool IsTableViewSourceGrouped() const;
    // True when the flat row at `index` is a group header rather than a data row. Group headers
    // share the flat projection (and its index space) with data rows, but are not selectable and
    // must be recognized by keyboard navigation as valid focus anchors.
    bool IsGroupHeaderRow(int32_t index) const;

    // Band gesture (directionless) and the ExpandCollapse pattern (directional). Both resolve the
    // target group's identity immediately and apply the mutation on a later turn.
    void ToggleGroupExpansion(winrt::UIElement const& container);
    void SetGroupExpansion(winrt::UIElement const& container, bool expand);

    void ExpandAllGroups();
    void CollapseAllGroups();

    winrt::ItemsRepeater GetRowsRepeaterForPeer() const { return m_rowsRepeater.get(); }

    bool SortByColumn(const winrt::TableViewColumn& column, winrt::SortDirection direction);
    bool ToggleSortDirection(const winrt::TableViewColumn& column);
    bool ClearSort();

    void RefreshSortIndicators();
    void OnColumnCanSortChanged(const winrt::TableViewColumn& column);
    void OnCanUserSortColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    bool PurgeColumnFromSortState(const winrt::TableViewColumn& removedColumn);

private:
    // Drives the SelectionModel; its SelectionChanged is the single funnel that publishes.
    void ApplySelection(int32_t index);

    void ReleaseHeaderToolTips(const winrt::Panel& host);

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

    winrt::IInspectable SelectedItemForIndex(int32_t index) const;

    winrt::TableViewRow FindRealizedRowForIndex(int32_t index);

    void PushSelectionProperties();
    void RaiseSelectionChanged(winrt::IInspectable const& addedItem);
    void RaiseSelectionAutomationEvents(winrt::TableViewRow const& deselectedRow, winrt::TableViewRow const& selectedRow);
    void RestampAllRealizedRowSelection();
    void RestampAllRealizedRowSelection(int32_t selectedIndex);

    void UpdateSelectionCollectionChangedSubscription();
    void OnSelectionItemsSourceCollectionChanged(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    // Subscribed AHEAD of the SelectionModel so it runs first on a projection Reset (filter/sort/
    // regroup re-materializing the rows in place). SelectionModel clears on a Reset because the
    // indices it holds are gone; this detector captures the still-selected item by object identity
    // BEFORE that clear and arms an identity-based restore, so a selected row that survives the
    // reshape keeps its selection instead of being dropped.
    void UpdateSelectionResetDetectorSubscription();
    void OnSelectionSourceReset(const winrt::IInspectable& sender, const winrt::NotifyCollectionChangedEventArgs& args);

    void ResolveSelectionAfterSourceChange();
    // Unload drains the repeater's source; re-sourcing on load clears the model. Hold the selected
    // item across that round trip so an unload/reload cycle does not drop the selection.
    void StashSelectionForReload();
    bool HasRowsSource() const;

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

    tracker_ref<winrt::IInspectable> m_pendingSelectedItem{ this };

    // The last INTENTIONALLY selected item (user gesture, programmatic set, or a restore), captured
    // by object identity in ApplySelection - the single selection writer. Unlike SelectedItem, it
    // is NOT cleared by a projection Reset that drops the model's indices, so it is the anchor an
    // identity-based restore re-selects against after a filter/sort/regroup reshape. A genuine
    // deselect flows through ApplySelection(-1) and nulls it, so an intentional clear is never
    // "restored".
    tracker_ref<winrt::IInspectable> m_stickySelectedItem{ this };

    // Armed by OnSelectionSourceReset when a Reset drops the selection with a surviving sticky item,
    // consumed by OnSelectionItemsSourceCollectionChanged to run the identity restore once the
    // model has reconciled.
    bool m_resetSelectionRestorePending{ false };

    uint32_t m_selectionVersion{ 0 };

    // Set while a stashed selection is being restored after a reload, so the clear-then-reselect
    // that SelectionModel::Source forces is published once at the end rather than as two events.
    bool m_isRestoringSelection{ false };

    // Set only while keyboard navigation is moving focus and Single selection follows that focus.
    bool m_isKeyboardFocusSelectionChange{ false };

    winrt::ItemsSourceView::CollectionChanged_revoker m_selectionCollectionChangedRevoker{};

    winrt::ItemsSourceView::CollectionChanged_revoker m_selectionResetDetectorRevoker{};

    // The views the two subscriptions above are attached to. Both re-register only when the view
    // actually changes, because SelectionModel::Source cannot be re-assigned for an unchanged view
    // (it clears the selection unconditionally) and so always keeps its original registration.
    // Re-registering these two against the same view would move them behind the model's and invert
    // the detector -> model -> restamp order ResolveSelectionAfterSourceChange documents.
    winrt::ItemsSourceView m_selectionCollectionChangedView{ nullptr };
    winrt::ItemsSourceView m_selectionResetDetectorView{ nullptr };

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

    std::deque<std::function<void()>> m_pendingEditReshapes;

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

    bool CompleteEditEnd(EditingUnit unit, winrt::TableViewEditAction action, bool honorCancel, bool vetoed);
    tracker_ref<winrt::IInspectable> m_currentEditItem{ this };
    tracker_ref<winrt::TableViewColumn> m_currentEditColumn{ this };
    tracker_ref<winrt::TableViewRow> m_currentEditRow{ this };

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

    bool FinishEditTeardown(EditingUnit unit, winrt::TableViewEditAction action, bool honorCancel);
    bool EndCurrentEdit(EditingUnit unit, winrt::TableViewEditAction action, bool honorCancel);

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

    void OnBodyScrollerViewChanged(
        const winrt::IInspectable& sender,
        const winrt::ScrollViewerViewChangedEventArgs& args);

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

    void ForEachRealizedRow(std::function<void(winrt::TableViewRow const&)> const& fn);

    void DetachAllColumnOwners();
    void TrackColumnsFromVector(winrt::IObservableVector<winrt::TableViewColumn> const& columns);
    bool ShouldShowColumnHeaders();
    int32_t GetItemsSourceCount() const;

    void PrepareGroupHeaderElement(winrt::TableViewGroupHeader const& header, int32_t index);
    void ClearGroupHeaderElement(winrt::TableViewGroupHeader const& header);
    void UpdateGroupHeaderWidth(winrt::TableViewGroupHeader const& header);

    // Split responsibilities driven off the ItemsSource DP:
    //   AdoptItemsSource   - source lifetime. Runs only when ItemsSource actually changes: normalize
    //                        a plain collection into a control-owned TableViewSource, detach the
    //                        previous source, adopt the new one, and wire its owner + handlers.
    //   RefreshRowsPipeline- pushes the active source's view into the repeater (identity-guarded),
    //                        re-reads its row-metadata provider, and re-resolves empty-state +
    //                        selection. Runs on every re-entry (template applied, repeater
    //                        reloaded, shaping verb) with no lifetime work.
    void AdoptItemsSource();
    void RefreshRowsPipeline();
    void OnTableViewSourceProjectionChanged();
    // Raised by the bound TableViewSource after a shaping verb rewrote the projection. A
    // programmatic reshape has no input event behind it, so this is the only thing that tells a
    // UIA client its cached rows are stale. reorderOnly separates a pure re-sort (same children,
    // new order) from a membership change.
    void OnTableViewSourceShapingChanged(bool reorderOnly);
    void ReconcileSortStateWithSource();
    void QueueReconcileSortStateWithSource();
    [[nodiscard]] auto BeginControlInitiatedSortScope()
    {
        m_isApplyingControlInitiatedSort = true;
        return gsl::finally([this]() { m_isApplyingControlInitiatedSort = false; });
    }
    void UpdateEmptyState();
    void UpdateEmptyStateCollectionChangedSubscription();
    void OnEmptyStateItemsSourceCollectionChanged(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    winrt::event_token m_columnsVectorChangedToken{};

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

    // Row semantics for the current projection (row kind, identity, group expansion). Null when no
    // TableViewSource is bound, or when the projection is degraded and carries no shaped identity.
    TableViewRowMetadataProvider m_tableViewSourceRowMetadata{};
    // Bumped on every metadata swap so a request captured against the previous provider can tell
    // that the provider which produced its identity is gone. Identities are value-based strings,
    // so without this a queued group toggle could resolve against a same-named group in a
    // brand-new source.
    uint64_t m_rowMetadataGeneration{ 0 };

    void RequestGroupExpansion(winrt::UIElement const& container, std::optional<bool> desired);    void QueueGroupExpansionByIdentity(winrt::hstring const& identity, std::optional<bool> desired);
    void ApplyGroupExpansionByIdentity(winrt::hstring const& identity, std::optional<bool> desired, uint64_t generation);
    void RaiseGroupStructureChanged();
    void SetAllGroupsExpansion(bool expand);

    // Keyboard-driven group toggle loses focus without this: the Enter/Space toggle defers a
    // structural reshape that recycles the focused header container, dropping focus (and its
    // visual) to nothing. Capture the header's identity + FocusState at gesture time, then restore
    // focus to the same group's header once the reshape's relayout has settled. Only keyboard /
    // programmatic focus is restored -- a pointer toggle carries no focus visual.
    void CaptureGroupHeaderFocusForRestore(winrt::UIElement const& container, winrt::hstring const& identity);
    winrt::hstring CaptureFocusedGroupHeaderForRestore();
    void RestoreGroupHeaderFocusIfPending(winrt::hstring const& identity);
    void FocusGroupHeaderByIdentity(winrt::hstring const& identity, winrt::FocusState focusState);
    // Row identity for a realized container. Identity is index-independent once captured.
    winrt::hstring TryGetContainerIdentity(winrt::UIElement const& container);

    winrt::hstring StringifyGroupKey(winrt::IInspectable const& key);
    winrt::DecimalFormatter GetGroupKeyDecimalFormatter();
    winrt::DecimalFormatter m_groupKeyDecimalFormatter{ nullptr };
    int32_t m_groupKeyDefaultFractionDigits{ 0 };

    // Chooses between the row and group-header container templates. Held so ~TableView can drop
    // the recycle pools it caches; see TableViewRowTemplateSelector::Detach.
    tracker_ref<winrt::TableViewRowTemplateSelector> m_rowTemplateSelector{ this };

    bool IsSortRequestStillValid(const winrt::TableViewColumn& column) const;
    bool IsSortClearStillValid() const;
    winrt::TableViewSource ShapingSourceInternal() const;
    void ApplySingleColumnSortState(const winrt::TableViewColumn& column, winrt::SortDirection direction);
    bool SyncTableViewSourceSort(const winrt::TableViewColumn& trigger, winrt::SortDirection direction);
    winrt::TableViewKeySelector GetTableViewSourceSortKeySelector(const winrt::hstring& sortMemberPath);
    bool RaiseSortingAndCheckCanceled(const winrt::TableViewColumn& trigger, winrt::SortDirection direction);
    // Single funnel for "the sort state has been written to the columns": reshapes, restores the
    // selection, raises Sorted, and announces.
    void RecomputeSortDPsAndRaiseInternal(const winrt::TableViewColumn& trigger);

    void ResetSortStateForNewItemsSource();
    int32_t FindEntryIndexForDataItem(const winrt::IInspectable& item) const;
    void AnnounceSortChange(const winrt::hstring& announcement);
    // The active sort column left Columns. Reshaping inside the VectorChanged callback would
    // re-enter the collection that is still mutating, so the reshape runs on the next turn.
    void QueueClearSortAfterColumnRemoval();
    bool m_clearSortAfterColumnRemovalQueued{ false };
    void AppendSortIndicatorVisual(const winrt::Panel& host, const winrt::TableViewColumn& column);
    static winrt::SortIndicator FindSortIndicator(const winrt::Panel& root);
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
    winrt::UIElement::BringIntoViewRequested_revoker m_headerBringIntoViewRevoker{};
    tracker_ref<winrt::ScrollViewer> m_bodyScroller{ this };

    winrt::event_token m_rowElementPreparedToken{};
    winrt::event_token m_rowElementClearingToken{};
    winrt::event_token m_rowElementIndexChangedToken{};
    winrt::event_token m_bodyScrollerViewChangedToken{};
    winrt::FrameworkElement::SizeChanged_revoker m_bodyScrollerSizeChangedRevoker{};
    winrt::event_token m_headerHostLoadedToken{};
    // Set while a drag is in flight, so Escape can reach the gripper that owns it.
    std::shared_ptr<ColumnResizeDragState> m_activeColumnResizeDrag{};
    bool m_isApplyingControlInitiatedSort{ false };
    bool m_sortReconcileQueued{ false };
    winrt::event_token m_rowsRepeaterLoadedToken{};
    winrt::event_token m_pendingFocusLayoutToken{};
    // Deferred restore of keyboard focus to a group header after a toggle reshape recycles it.
    // Separate from m_pendingFocusLayoutToken (row focus) so a row-focus request and a group-focus
    // restore in flight at once cannot clobber each other's one-shot LayoutUpdated token.
    winrt::event_token m_pendingGroupFocusLayoutToken{};
    winrt::hstring m_pendingGroupFocusIdentity{};
    winrt::FocusState m_pendingGroupFocusState{ winrt::FocusState::Unfocused };
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

    winrt::FrameworkElement::Unloaded_revoker m_unloadedRevoker{};
    void OnTableViewUnloaded();
    bool m_rowsSourceDrained{ false };

    double ComputeLeadingFrozenWidth();
    void RefreshFrozenColumns();

    double GetHeaderMeasuredWidthForColumn(const winrt::TableViewColumn& column) const;
    void ResolveColumnWidths();
    void ResetColumnDesiredWidths();
    void InvalidateCellPanels();

    bool m_frozenColumnsActive{ false };

    bool m_rebuildHeadersQueued{ false };

    // Per-instance resource cache; replaces the former process-global map keyed by `this`.
    TableViewResourceCache m_resourceCache{};

    std::vector<tracker_ref<winrt::TableViewColumn>> m_trackedColumns;

    winrt::KeyEventHandler m_keyDownHandler{ nullptr };  // Root KeyDown (handledEventsToo); registration is released with the element, no explicit RemoveHandler needed.
    void OnKeyDownForNavigation(
        const winrt::IInspectable& sender,
        const winrt::KeyRoutedEventArgs& args);

    bool TryHandleHeaderColumnResizeKey(const winrt::KeyRoutedEventArgs& args);
    // Enter / Space on a focused, sortable column header: the keyboard path to sorting.
    bool TryHandleHeaderSortKey(const winrt::KeyRoutedEventArgs& args);
    bool TryHandleHeaderSortKeyUp(const winrt::KeyRoutedEventArgs& args);
    winrt::TableViewColumn ResolveHeaderSortKeyTarget(const winrt::KeyRoutedEventArgs& args);
    winrt::weak_ref<winrt::TableViewColumn> m_headerSortSpaceArmedColumn{ nullptr };
    winrt::UIElement::LostFocus_revoker m_headerSortLostFocusRevoker{};
    winrt::KeyEventHandler m_keyUpHandler{ nullptr };
    void OnKeyUpForHeaderSort(
        const winrt::IInspectable& sender,
        const winrt::KeyRoutedEventArgs& args);
    void OnHeaderBringIntoViewRequested(const winrt::BringIntoViewRequestedEventArgs& args);

    // PreviewKeyDown snapshots the pre-key focus before XAML's built-in navigation can move it.
    winrt::KeyEventHandler m_previewKeyDownHandler{ nullptr };

    winrt::KeyEventHandler m_editingKeyDownHandler{ nullptr };
    winrt::UIElement::LosingFocus_revoker m_editingLosingFocusRevoker{};

    bool m_focusLossCommitQueued{ false };

    int32_t m_navAnchorRow{ -1 };
    // Cell cursor snapshot for keys whose bubbling handler must ignore post-key live focus.
    int32_t m_navAnchorCellRow{ -1 };
    int32_t m_navAnchorCellColumn{ -1 };
    // The cell the keyboard cursor is on, in visible-column coordinates. Up/Down/PageUp/PageDown
    // preserve it, Left/Right move it, and entering the table from outside restores it. Kept as an
    // index rather than an element so it survives row recycling, which destroys cell elements on
    // every scroll.
    int32_t m_currentCellColumn{ 0 };
    int32_t m_currentCellRow{ -1 };
    void OnPreviewKeyDownForNavigation(
        const winrt::IInspectable& sender,
        const winrt::KeyRoutedEventArgs& args);

    bool TryHandleCellNavigationKey(const winrt::KeyRoutedEventArgs& args);

public:
    // The cell-cursor move itself, free of routed-event args so the key handler and the
    // implementation-only test hook on TableViewCell drive exactly the same code. Returns true when
    // the key belongs to cell navigation (including at a boundary, where the cursor does not move).
    //
    // anchorRow / anchorColumn are the cursor position BEFORE the key was delivered. They must be
    // passed by the key path, because built-in directional navigation may already have moved focus.
    // Pass -1, -1 to anchor on live focus instead (no routed event in flight).
    bool TryMoveCellCursorFromAnchor(
        winrt::Windows::System::VirtualKey key, bool isControlDown,
        int32_t anchorRow, int32_t anchorColumn);

    bool TryMoveCellCursor(winrt::Windows::System::VirtualKey key, bool isControlDown);

private:
    static constexpr int32_t c_lastColumnSentinel{ 0x7ffffffe };

    winrt::TableViewRow GetRealizedRowAt(int32_t rowIndex) const;

    int32_t GetFocusedRowIndex() const;
    int32_t GetEstimatedRowsPerPage(); // Non-const — GetDensityRowMinHeight() mutates the resource cache.
};
