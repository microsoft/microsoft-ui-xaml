// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include <chrono>
#include <deque>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

#include "TableView.g.h"
#include "TableViewContextMenu.h"
#include "TableView.properties.h"
#include "TableViewRowInfo.h"
#include "TableViewTip.h"
#include "DragEscapeHook.h"
#include "TableViewRowFlattener.h"

// ThemeSettings (Microsoft.UI.System) is used below for UI-thread High Contrast change notifications.
// Included here (not the shared CppWinRTIncludes.h) to keep the rebuild scope local to TableView.
#include <winrt/Microsoft.UI.System.h>

namespace TableViewDetails
{
    // A UIElement header has no text, and a templated header shows its template rather than the
    // object's ToString (CsWinRT makes every managed object IStringable, yielding its type name).
    // A ToString that is just the runtime class name is not text either: the object is typically
    // rendered by an implicit DataType template, so callers fall back to the realized header's name.
    inline std::optional<winrt::hstring> TryGetHeaderContentText(const winrt::IInspectable& header, bool isTemplated)
    {
        if (!header || header.try_as<winrt::UIElement>())
        {
            return std::nullopt;
        }
        if (auto const propValue = header.try_as<winrt::IPropertyValue>();
            propValue && propValue.Type() == winrt::PropertyType::String)
        {
            return propValue.GetString();
        }
        if (!isTemplated)
        {
            if (auto const stringable = header.try_as<winrt::IStringable>())
            {
                auto text = stringable.ToString();
                winrt::hstring className;
                try
                {
                    className = winrt::get_class_name(header);
                }
                catch (...)
                {
                }
                if (text.empty() || text == className)
                {
                    return std::nullopt;
                }
                return text;
            }
        }

        return std::nullopt;
    }

    inline std::optional<winrt::hstring> TryGetColumnHeaderText(const winrt::TableViewColumn& column)
    {
        if (!column)
        {
            return std::nullopt;
        }

        return TryGetHeaderContentText(
            column.Header(), column.HeaderTemplate() || column.HeaderTemplateSelector());
    }

    // Shared by the header cell's automation name and the gripper's OwnerName.
    inline winrt::hstring GetColumnHeaderText(const winrt::TableViewColumn& column)
    {
        return TryGetColumnHeaderText(column).value_or(winrt::hstring{});
    }

    // Whether the app derived the column's class, so the column runs the app's overrides; TableView's own column
    // classes run only TableView's code. The class name is compared, never reported. One that cannot be read is
    // treated as the app's.
    inline bool IsAppDerivedColumn(const winrt::TableViewColumn& column) noexcept
    {
        try
        {
            auto const className = winrt::get_class_name(column);
            std::wstring_view const name{ className };
            return name != winrt::name_of<winrt::TableViewTemplateColumn>() && name != winrt::name_of<winrt::TableViewColumn>();
        }
        catch (...) { return true; }
    }
}

struct ColumnResizeBounds
{
    double Min{ 0.0 };
    double Max{ std::numeric_limits<double>::infinity() };
};

// A Star column before the dragged one, held at the width it already renders. The local Width
// value is kept -- not the effective one -- so a canceled gesture restores a binding rather than
// overwriting it with a local value.
struct ColumnResizeFrozenColumn
{
    winrt::weak_ref<winrt::TableViewColumn> column{ nullptr };
    winrt::IInspectable width{ nullptr };
};

// A locked Star column before a resized one, held at the width it rendered when the resize began.
struct LockedColumnHold
{
    winrt::weak_ref<winrt::TableViewColumn> column{ nullptr };
    double width{ 0.0 };
    // What the Auto columns took when the hold was latched; growth past it can make the hold stale.
    double autoTotal{ 0.0 };
};

// One in-flight resize drag. The gripper owns the gesture; this is only the host's anchor for it,
// kept reachable so Escape can cancel the drag in flight.
struct ColumnResizeDragState
{
    // Weak: the handlers that own this state are registered on the gripper itself, so a strong
    // reference here is a cycle the XAML reference tracker cannot see.
    winrt::weak_ref<winrt::ResizeGripper> gripper{ nullptr };
    double startValue{ 0.0 };
    // The column's Width as a local value: reverting a canceled drag to startValue would rewrite an
    // Auto or Star column as fixed pixels, and restoring the effective value would overwrite a binding.
    winrt::IInspectable startWidth{ nullptr };
    // Captured once per gesture: the drag does not change the other columns' widths.
    ColumnResizeBounds bounds{};
    // Set once a DragDelta has actually written Width, so a canceled press that never moved
    // leaves the column completely untouched.
    bool didWrite{ false };
    // Set once any DragDelta arrived, even one the bounds swallowed, so a step held at a bound
    // still announces its width instead of going silent.
    bool didDelta{ false };
    // Filled on the first write, not at DragStarted, so a press that never moves mutates nothing.
    std::vector<ColumnResizeFrozenColumn> frozen;
    // The table's locked-column holds before the first write, restored if the drag is canceled.
    std::optional<std::pair<std::vector<LockedColumnHold>, size_t>> lockedHoldsBefore;
    // A double-click fit writes a Pixel width even when an Auto or Star column already has its size.
    bool commitPixelWidth{ false };
    uint64_t lastPressTimestamp{ 0 };
    winrt::Point lastPressPosition{};
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
        bool hasCellTextPadding{ false };
        winrt::Thickness cellTextPadding{};
        bool hasHeaderTextPadding{ false };
        winrt::Thickness headerTextPadding{};
        bool hasGroupHeaderTextPadding{ false };
        winrt::Thickness groupHeaderTextPadding{};
        bool hasHeaderCellPadding{ false };
        winrt::Thickness headerCellPadding{};
        bool hasGroupHeaderMinHeight{ false };
        double groupHeaderMinHeight{};
        bool hasGroupHeaderPadding{ false };
        winrt::Thickness groupHeaderPadding{};
    };
    DensityInfo density{};

    // Built-in text defaults to 14px at every density. Density/theme changes clear
    // this cache alongside the other resolved resources.
    struct FontInfo
    {
        bool hasCellFontSize{ false };
        double cellFontSize{ 0.0 };
    };
    FontInfo font{};

    // Cached hierarchy geometry. Runtime resource swaps raise no notification, so they apply only
    // after cache invalidation (density/theme/HC/re-template) and row re-prepare.
    struct HierarchyInfo
    {
        bool hasRowIndentSize{ false };
        double rowIndentSize{ 0.0 };
        bool hasRowExpanderSize{ false };
        double rowExpanderSize{ 0.0 };
        bool hasGroupHeaderContentOffset{ false };
        double groupHeaderContentOffset{ 0.0 };
    };
    HierarchyInfo hierarchy{};

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

class GroupedEntry;

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

    // IFrameworkElement overrides
    void OnApplyTemplate();
    winrt::AutomationPeer OnCreateAutomationPeer();

    // Property-changed callbacks (from TableViewProperties)
    void OnItemsSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnItemKeyPathPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnParentKeyPathPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void ApplyHierarchyPaths();
    void ApplyHierarchyPaths(TableViewRowFlattener& flattener);
    void OnColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnHeadersVisibilityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnGridLinesVisibilityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnRowBackgroundPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnAlternatingRowBackgroundPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnEmptyTemplatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnGroupHeaderTemplatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnDensityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    // Density resources fall back to the selected preset; rows and columns call these via get_self.
    double GetDensityRowMinHeight();
    winrt::Thickness GetDensityCellPadding();
    winrt::Thickness GetDensityHeaderCellPadding();
    double GetDensityGroupHeaderMinHeight();
    winrt::Thickness GetDensityGroupHeaderPadding();
    winrt::Style GetGroupHeaderStyle(winrt::TableViewGroupHeader const& header);
    static bool IsDefaultGroupHeaderContentTemplate(winrt::TableViewGroupHeader const& header);
    static bool StyleSetsProperty(const winrt::Style& style, const winrt::DependencyProperty& property);
    double GetCellFontSize();
    winrt::Thickness GetDensityCellTextPadding();
    winrt::Thickness GetDensityHeaderTextPadding();
    winrt::Thickness GetDensityGroupHeaderTextPadding();
    winrt::Thickness GetDisplayCellPadding(double contentVerticalOffset = 0.0);
    void AttachFocusCornerRadius(winrt::FrameworkElement const& target);

    // Per-level indent from the TableViewRowIndentSize resource, cached for rows.
    double GetRowIndentSize();

    // Chevron gutter width from TableViewRowExpanderSize (same key the template binds).
    double GetRowExpanderSize();
    double GetGroupHeaderContentOffset();

    // Resolved grid-line brush (theme/HC-aware, cached); rows call this via get_self, like the
    // density accessors above.
    winrt::Brush GetGridLineBrush();

    static winrt::SortIndicatorDirection ToSortIndicatorDirection(winrt::SortDirection direction);

    void OnColumnHeaderTemplateApplied(winrt::TableViewColumnHeader const& header);
    void OnColumnHeaderStylePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    // Per-instance resource cache (density metrics + gridline brush); accessed by the
    // file-scope resource helpers in TableView.cpp through this owner pointer.
    TableViewResourceCache& GetResourceCacheInternal() { return m_resourceCache; }

    // ActualTheme cannot report HC; cached AccessibilitySettings selects HC grid-line resources.
    bool IsHighContrast();

    // TableViewColumn calls this when header templates change so realized headers refresh.
    void RebuildHeaders();
    // Toggling it adds or removes every gripper, so the header band is rebuilt.
    void OnCanUserResizeColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    // The gripper owns the resize mechanics; this only sizes it and forwards the pointer drag.
    winrt::ResizeGripper CreateResizeGripper(
        const winrt::FrameworkElement& headerCell,
        const winrt::TableViewColumn& column,
        double gripperWidth,
        const winrt::hstring& headerText);

    winrt::ResizeGripper FindResizeGripperInCell(const winrt::FrameworkElement& headerCell) const;

    // Idempotent; safe from any pointer end-event, the gripper's Unloaded, or TableView's own.
    void CancelColumnResizeDrag();

    // UIA Transform support for the column header peer. Driven through the column's gripper so the
    // bounds, Star handling and announcement match a pointer or Alt+Arrow resize exactly.
    bool CanResizeColumnFromAutomation(const winrt::TableViewColumn& column);
    bool TryResizeColumnFromAutomation(const winrt::TableViewColumn& column, double width, bool commitPixelWidth = false);
    bool AutoFitColumn(const winrt::TableViewColumn& column);
    static double DoubleClickSlop(const winrt::UIElement& element);
    static uint64_t DoubleClickIntervalMicroseconds();
    void AnnounceColumnWidth(const winrt::IInspectable& announcer, const winrt::TableViewColumn& column);

    // Coalesces bursts of column-driven header rebuilds (bulk Columns edits, per-column Header /
    // FrozenEdge changes) into a single RebuildHeaders on the next dispatcher tick, so N column
    // mutations cost O(N) header builds instead of O(N^2). Skips entirely before the template is
    // applied (OnApplyTemplate performs the initial build). Falls back to a synchronous rebuild when
    // no dispatcher is available or the enqueue fails.
    void QueueRebuildHeaders();

    // Coalesces a burst of column changes into one StructureChanged on the next tick, so Narrator
    // does not re-read the grid once per mutation.
    void QueueRaiseColumnsStructureChanged();
    void RaiseColumnsStructureChanged();

    // Internal — invoked by TableViewColumn when its Visibility changes so
    // realized header and row cells stay in sync without rebuilding Columns.
    void OnColumnVisibilityChanged(const winrt::TableViewColumn& column);

    // Internal — invoked by TableViewColumn when Width / MinWidth / MaxWidth changes so the next
    // measure pass re-resolves column widths (ResolveColumnWidths then re-pins frozen columns and
    // refreshes gridline visuals from the resolved widths).
    void OnColumnWidthChanged(const winrt::TableViewColumn& column);

    // Internal - ends the locked-column holds; called when an input they were latched against changes.
    void ExpireLockedColumnHolds();

    // TableViewTemplateColumn calls this when CellTemplate changes so realized rows regenerate cells.
    void OnColumnCellTemplateChanged(const winrt::TableViewColumn& column, bool resetAutoWidth = true);
    // Posts (or, outside a live tree, runs) the regeneration.
    void ScheduleColumnCellRegeneration(const winrt::TableViewColumn& column, bool resetAutoWidth, bool postOffTree = false);
    void ScheduleReattachedColumnCellRegeneration(const winrt::TableViewColumn& column);
    void ScheduleReattachedColumnCellRegenerations();
    void RegenerateColumnCells(const winrt::TableViewColumn& column, bool resetAutoWidth);
    void RegenerateColumnCellsFromPost(const winrt::TableViewColumn& column);

    // TableViewColumn calls this when Header / HeaderTemplate / HeaderTemplateSelector changes so
    // headers re-render and Auto columns recompute their content width.
    void OnColumnHeaderChanged(const winrt::TableViewColumn& column);

    // Re-applied in place: a rebuild would drop header focus and re-stamp the sort affordance.
    void OnColumnHeaderToolTipChanged(const winrt::TableViewColumn& column);

    // TableViewColumn calls this when FrozenEdge changes so headers re-render and the leading-frozen
    // band re-pins.
    void OnColumnFrozenEdgeChanged(const winrt::TableViewColumn& column);

    // Pin rebuilt rows immediately when leading-frozen columns are active.
    void PinFrozenColumnsForRow(const winrt::TableViewRow& row);

    // Requested by a cell panel (header/row) during measure when a realized cell's own measured width
    // changed (grow or shrink). Invalidates our measure synchronously so ResolveColumnWidths re-runs in
    // the same layout tick (no deferral -> no one-frame lag). Bridges the body ScrollViewer, which
    // absorbs the cell's own measure invalidation. Converges without a debounce (see the definition).
    void RequestColumnWidthResolve();

    // Automation peer accessors; weak refs may be null before OnApplyTemplate.
    winrt::ItemsRepeater GetRowsRepeaterInternal() const { return m_rowsRepeater.get(); }
    winrt::Panel GetHeaderHostInternal() const { return m_headerHost.get(); }
    bool AreColumnHeadersShownInternal() { return ShouldShowColumnHeaders(); }
    int32_t GetRowCountInternal() const { return GetItemsSourceCount(); }
    int32_t GetDataRowPositionInSetInternal(int32_t rowIndex) const;
    int32_t GetDataRowSizeOfSetInternal() const;
    winrt::ScrollViewer GetBodyScrollerInternal() const { return m_bodyScroller.get(); }
    // Releases the rows repeater's layout anchor, made by GetOrCreateElement(anchor's index), without scrolling.
    void ReleaseRowsRepeaterAnchor(winrt::UIElement const& anchor);

    // Test hook for moving keyboard focus into the body at a row index; false for invalid indexes
    // or before rows exist. Focus lands at whichever LEVEL the cursor is on - the ROW container by
    // default, or that row's current cell once the user has drilled in with Right.
    bool FocusRow(int32_t index);

    void HandleBodyKeyFromRowInternal(const winrt::KeyRoutedEventArgs& args);


    // Moves keyboard focus to a cell by row index and VISIBLE column index, realizing and scrolling
    // the row into view first. A negative column means "keep the current one". Group-header rows
    // have no cells and fall back to focusing the header container. Drills the cursor to CELL level.
    bool FocusCell(int32_t rowIndex, int32_t visibleColumnIndex, std::optional<double> alignRatio = std::nullopt);

    // Moves keyboard focus to the row CONTAINER at a flat row index, realizing and scrolling it
    // into view first, and leaves the cursor at ROW level. This is the body's entry level and what
    // Up/Down move between while the user has not drilled into cells.
    bool FocusRowContainer(int32_t rowIndex, std::optional<double> alignRatio = std::nullopt);

    // No side effects; the caller arms the level it redirects to.
    struct FocusEntryTarget
    {
        winrt::TableViewRow Row{ nullptr };
        // A UIElement, or a Hyperlink in the empty-state content.
        winrt::DependencyObject Element{ nullptr };
        bool CellLevel{ false };
        bool Remembered{ false };
    };

    static bool IsEntryRedirectTargetFocusable(winrt::DependencyObject const& target, winrt::DependencyObject const& owner);
    // allowNoViewport: without a body scroller, every realized row counts as in view (entry only; paging keeps its estimate).
    winrt::UIElement FindViewportEdgeElementInternal(bool last, bool includeGroupHeaders, bool allowNoViewport = false) const;

    // A redirect re-raises GettingFocus with the same CorrelationId; redirect once to avoid a loop.
    bool HasRedirectedFocusEntry(winrt::guid const& correlationId) const noexcept
    {
        return m_hasFocusEntryRedirect && m_focusEntryRedirectCorrelationId == correlationId;
    }
    void NoteFocusEntryRedirect(winrt::guid const& correlationId) noexcept
    {
        m_focusEntryRedirectCorrelationId = correlationId;
        m_hasFocusEntryRedirect = true;
    }

    void ArmCellLevelRowInternal(winrt::TableViewRow const& row);

    winrt::hstring GetContainerIdentityInternal(winrt::UIElement const& container)
    {
        return TryGetContainerIdentity(container);
    }

    winrt::AutomationPeer GetGroupAutomationElementInternal(winrt::hstring const& identity)
    {
        auto const it = m_groupAutomationElements.find(identity);
        return it != m_groupAutomationElements.end() ? it->second : nullptr;
    }
    void SetGroupAutomationElementInternal(winrt::hstring const& identity, winrt::AutomationPeer const& element);

    // Two-level cursor state. False = ROW level, true = CELL level. Owned here rather than on the
    // row because it has to survive row recycling and follow the cursor from row to row.
    void SetCellCursorActiveInternal(bool active);

    void OnRowCellFocusChanged(winrt::TableViewRow const& row, winrt::DependencyObject const& focusedElement);

    bool TryGetFocusedCell(int32_t& rowIndex, int32_t& columnIndex, bool requireExactCell) const;

    // Focuses the cell at a visible-column index inside an ALREADY realized container. Group
    // headers share the repeater and have no cells, so they keep taking container focus.
    bool FocusRealizedRowCell(
        winrt::UIElement const& element,
        int32_t visibleColumnIndex,
        winrt::FocusState focusState = winrt::FocusState::Keyboard);

    // Actionable headers and cells are the only header/body keyboard focus targets.
    bool IsColumnUserSortableInternal(winrt::TableViewColumn const& column);
    bool IsHeaderCellActionableInternal(winrt::FrameworkElement const& header);
    void RefreshCellFocusLevelsInternal();
    void RefreshHeaderTabStopsInternal();
    // The inactive-header IsTabStop stamp; an app's local IsTabStop value or binding is never ours.
    bool HasHeaderTabStopStampInternal(winrt::FrameworkElement const& header);
    void ClearHeaderTabStopStampInternal(winrt::FrameworkElement const& header);

    // True when handled (menu shown, or suppressed: nested or stale target); false lets the
    // request bubble to the native ContextFlyout chain.
    bool ProcessContextMenuRequest(
        TableViewDetails::ContextMenuTarget const& target,
        winrt::ContextRequestedEventArgs const& args);

    bool FocusContextMenuTargetFromPointer(TableViewDetails::ContextMenuTarget const& target);

    // A column a context menu request can target: in this table's Columns, owned by it, Visible.
    bool IsContextMenuColumn(winrt::TableViewColumn const& column) const;

    winrt::Size MeasureOverride(winrt::Size const& availableSize);

    void SetCurrentCell(winrt::IInspectable const& item, winrt::TableViewColumn const& column);

    // Measure, arrange and the repeater's element callbacks. App-visible work is posted from here.
    [[nodiscard]] auto BeginLayoutCallbackScope()
    {
        ++m_layoutCallbackDepth;
        return gsl::finally([this]() { --m_layoutCallbackDepth; });
    }
    bool IsInLayoutCallback() const noexcept { return m_layoutCallbackDepth > 0 || s_elementLayoutDepth > 0; }

    // Measure/arrange of the control's own elements (cells panels, group headers). Per thread, not
    // per table: any of them can run app code that reaches a table.
    static inline thread_local uint32_t s_elementLayoutDepth{ 0 };
    struct ElementLayoutScope
    {
        ElementLayoutScope() noexcept { ++s_elementLayoutDepth; }
        ~ElementLayoutScope() { --s_elementLayoutDepth; }
        ElementLayoutScope(ElementLayoutScope const&) = delete;
        ElementLayoutScope& operator=(ElementLayoutScope const&) = delete;
    };

    // Turns a row's container-level item into the data item. Identity today; the seam a wrapping
    // layer (grouping) changes in one place.
    winrt::IInspectable UnwrapDataItem(winrt::IInspectable const& item) const;

    // Identity comparison that survives boxing and re-projection. Raw IInspectable equality is not
    // reliable for boxed values, so anything comparing data items must use this.
    static bool SameInspectableIdentity(winrt::IInspectable const& lhs, winrt::IInspectable const& rhs);

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
    // Always false while only None and Single exist; Multiple must derive it from SelectionMode.
    bool CanSelectMultipleRows() const noexcept { return false; }

    // Shared by the pointer gesture, keyboard navigation and the row peer. No-op when off.
    // The one-arg form reads Ctrl live and toggles; automation passes toggle=false, because UIA
    // Select() means "make this the selection", never "clear it".
    void SelectRowIndexFromInteraction(int32_t index);
    void SelectRowIndexFromInteraction(int32_t index, bool toggle);
    void SelectRowIndexFromKeyboardFocus(int32_t index);

    void OnRowPointerSelect(winrt::TableViewRow const& row);

    void RefreshRowSelectionState(winrt::TableViewRow const& row);
    void RefreshRowSelectionState(winrt::TableViewRow const& row, int32_t selectedIndex);
    void RefreshRowHierarchyState(winrt::TableViewRow const& row, int32_t index);

    // Restamps realized rows after a reshape; rows that kept their index are not re-prepared.
    void RefreshRealizedRowHierarchyState();

    // Deferred and coalesced: element indices are only trustworthy after the repeater reconciles.
    void QueueRefreshRealizedRowHierarchyState();

    // Rewired whenever the pipeline re-reads the row view.
    void UpdateRowHierarchyResetSubscription();
    void OnRowsSourceResetForHierarchy(
        const winrt::IInspectable& sender,
        const winrt::NotifyCollectionChangedEventArgs& args);

    // For the automation peers, which cannot reach the private members. Both read the model.
    int32_t SelectedIndexInternal() const;
    winrt::IInspectable SelectedItemInternal() const;

    // --- Grouped projections (TableView_Grouping.cpp) ---
    //
    // Item-based because the element factory is only handed the item.
    TableViewRowKind GetRowKindForItem(winrt::IInspectable const& item) const;
    winrt::hstring GetGroupHeaderNameCandidate(GroupedEntry const& entry);
    bool TryGetRowInfo(int32_t rowIndex, TableViewRowInfo& rowInfo) const;
    TableViewRowMetadataProvider RowMetadataInternal() const { return m_tableViewSourceRowMetadata; }
    // True when the rows include group headers.
    bool IsGroupedRowSource() const;
    // True when the flat row at `index` is a group header rather than a data row. Group headers
    // share the flat projection (and its index space) with data rows, but are not selectable and
    // must be recognized by keyboard navigation as valid focus anchors.
    bool IsGroupHeaderRow(int32_t index) const;

    // Band gesture (directionless) and the ExpandCollapse pattern (directional). Both resolve the
    // target group's identity immediately and apply the mutation on a later turn.
    void ToggleGroupExpansion(winrt::UIElement const& container);
    void SetGroupExpansion(winrt::UIElement const& container, bool expand);
    // Expands the row and all descendants (Multiply key). Same deferral as above.
    void ExpandRowSubtree(winrt::UIElement const& container);

    // Tree keyboard navigation. Each moves row focus and returns whether it did.
    bool TryFocusParentRow(winrt::UIElement const& container);

    // Public bulk expansion commands (from TableView IDL), one pair per axis.
    void ExpandAllGroups();
    void CollapseAllGroups();
    void ExpandAllRows();
    void CollapseAllRows();
    void ExpandGroup(winrt::IInspectable const& groupKey);
    void CollapseGroup(winrt::IInspectable const& groupKey);

    // Displayed-row index space, the one Select/SelectedIndex use: group headers take slots.
    int32_t IndexOf(winrt::IInspectable const& item);
    void StartBringItemIntoView(int32_t index, winrt::BringIntoViewOptions const& options);

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
    // Pointer, keyboard and UIA Invoke: the only callers CanUserSort/CanUserSortColumns gate.
    bool ToggleSortDirectionFromUserInput(const winrt::TableViewColumn& column);
    // The app wrote TableViewColumn.SortDirection (not the control). Display only.
    void OnColumnSortDirectionSetByApp(const winrt::TableViewColumn& column, winrt::SortDirection direction);
    // The lit direction is the app's (set through SortDirection, or its own source sort), not one this table applied.
    bool IsSortLitOnlyByAppInternal() const { return IsSortLitOnlyByApp(); }
    // A column's sort key (CustomSortComparer, SortMemberPath, DisplayMemberBinding) changed: re-sorts
    // a column this table sorted on its new key, or clears its sort when it has no key left.
    void OnColumnSortKeyChanged(const winrt::TableViewColumn& column);
    // The chevron and click handler are stamped at header-build time; rebuilds when sortability changed.
    void RefreshHeaderSortability(const winrt::TableViewColumn& column);
    void RaiseColumnSortStateAutomationChanged(const winrt::TableViewColumn& column, winrt::SortDirection oldDirection);

    // Republishes every realized header chevron from its column's SortDirection DP. A push rather
    // than a binding: SortIndicatorDirection and SortDirection are distinct WinRT enums, so a
    // {Binding} between them silently does nothing. Called by TableViewColumn.
    void RefreshSortIndicators();
    // CanUserSort gates whether the chevron and click handler are built, so a runtime flip needs a header rebuild.
    void OnColumnCanUserSortChanged(const winrt::TableViewColumn& column);
    void OnCanUserSortColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    // Drops a column that has left Columns from the active sort state. Returns true when the sort
    // state changed.
    bool PurgeColumnFromSortState(const winrt::TableViewColumn& removedColumn);

private:
    class ActionTip;
    // Drives the SelectionModel; its SelectionChanged is the single funnel that publishes.
    void ApplySelection(int32_t index, bool measureTip = false);

    enum class HeaderRelease { All, DetachedColumns, Column };
    void ReleaseHeaderCells(const winrt::Panel& host, HeaderRelease scope = HeaderRelease::All, const winrt::TableViewColumn& column = nullptr);
    void MoveHeaderFocusOffRemovedColumn(const winrt::Panel& host, const winrt::FrameworkElement& removedCell, const winrt::TableViewColumn& removedColumn, winrt::FocusState focusState);
    void RebuildHeadersCore();
    winrt::Style CompatibleHeaderStyle(const winrt::Style& style);
    winrt::TableViewColumn GetFocusedHeaderColumn(const winrt::Panel& host, winrt::FocusState& focusState) const;
    // Refocuses the rebuilt header for column; retries once after layout if it cannot take focus yet.
    void RestoreHeaderFocus(const winrt::TableViewColumn& column, winrt::FocusState focusState);
    bool TryFocusHeaderCell(const winrt::TableViewColumn& column, winrt::FocusState focusState);
    bool CanRestoreHeaderFocus(const winrt::DependencyObject& focusAtSchedule) const;
    void CancelHeaderFocusRestore();
    // Visible-header index that headerCell's successor takes once it collapses, or -1 when focus is
    // not inside headerCell.
    int32_t GetCollapsingHeaderFocusIndex(const winrt::Panel& host, const winrt::FrameworkElement& headerCell) const;

    // Created lazily so a TableView that never selects pays nothing.
    void EnsureSelectionModel();
    // SelectionModel::Source has no identity short-circuit - re-setting the same source would drop
    // the selection - so this only writes when it actually differs.
    void UpdateSelectionModelSource();
    void OnSelectionModelSelectionChanged(
        const winrt::SelectionModel& sender,
        const winrt::SelectionModelSelectionChangedEventArgs& args);
    // Calls into SelectionModel, which raises SelectionChanged back into OnSelectionModelSelectionChanged; an app
    // fault that left that handler gets its tag back here.
    template <typename Call>
    void CallSelectionModel(Call&& call);

    // Index of `item` in the ItemsSource, by identity; -1 when absent or the source is not set.
    // Still needed because SelectionModel indexes but does not look items up.
    int32_t IndexOfItem(winrt::IInspectable const& item) const;
    bool IsDisplayedItemMatch(winrt::IInspectable const& row, winrt::IInspectable const& target, bool targetIsValue) const;

    // The selected item for an already-read index, so a caller that has one does not rebuild the
    // model's IndexPath just to resolve it again.
    winrt::IInspectable SelectedItemForIndex(int32_t index) const;

    // The realized container for a data index, or null.
    winrt::TableViewRow FindRealizedRowForIndex(int32_t index);

    // StartBringItemIntoView: one pending request, retried after layout until the rows can realize it.
    int32_t DisplayedRowCount() const;
    winrt::ItemsSourceView DisplayedRowsView() const;
    void ArmPendingBringIntoView();
    void CancelPendingBringItemIntoView();
    void RevokePendingBringIntoViewLayout();
    bool TryBringPendingItemIntoView();
    void PostPendingBringIntoViewAttempt();
    void RunPendingBringIntoViewAttempt();
    bool IsBringIntoViewSatisfied(winrt::FrameworkElement const& element, winrt::BringIntoViewOptions const& options) const;
    static constexpr int32_t s_bringIntoViewPasses{ 3 };
    // Layout passes to wait for an issued scroll to apply before re-issuing.
    static constexpr int32_t s_bringIntoViewWaits{ 10 };
    bool m_bringIntoViewIssued{ false };
    bool m_bringIntoViewAttemptInProgress{ false };
    int32_t m_bringIntoViewPassesLeft{ 0 };
    int32_t m_bringIntoViewWaitsLeft{ 0 };
    double m_bringIntoViewIssueOffset{ 0.0 };
    int32_t m_pendingBringIntoViewIndex{ -1 };
    winrt::BringIntoViewOptions m_pendingBringIntoViewOptions{ nullptr };
    winrt::event_token m_pendingBringIntoViewLayoutToken{};

    void PushSelectionProperties();
    void RaiseSelectionChanged(winrt::IInspectable const& addedItem);
    void RaiseSelectionAutomationEvents(
        winrt::TableViewRow const& deselectedRow,
        winrt::IInspectable const& deselectedItem,
        winrt::TableViewRow const& selectedRow,
        winrt::IInspectable const& selectedItem);
    // Re-derives IsSelected on every realized row. Needed after a collection change, where the
    // repeater has already re-indexed its containers.
    void RestampAllRealizedRowSelection();
    void RestampAllRealizedRowSelection(int32_t selectedIndex);

    // Subscribed only to restamp rows: SelectionModel handles the index reconciliation itself, and
    // does not raise when an insert merely shifts the selected index.
    void UpdateSelectionCollectionChangedSubscription();
    void OnSelectionItemsSourceCollectionChanged(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    // Subscribed AHEAD of the SelectionModel so it runs first on a projection Reset (filter/sort/
    // regroup re-materializing the rows in place). SelectionModel clears on a Reset because the
    // indices it holds are gone; this detector captures the still-selected item by object identity
    // BEFORE that clear and arms an identity-based restore, so a selected row that survives the
    // reshape keeps its selection instead of being dropped.
    void UpdateSelectionResetDetectorSubscription();
    void OnSelectionSourceReset(const winrt::IInspectable& sender, const winrt::NotifyCollectionChangedEventArgs& args);

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
    // The app fault that left OnSelectionModelSelectionChanged during the CallSelectionModel in progress (depth).
    TableViewTip::AppFaultHop m_selectionModelAppFault;
    uint32_t m_selectionModelCallDepth{};

    // Last index published to the rows, so a change knows which container to unstamp and which to
    // raise the UIA "removed from selection" event on.
    int32_t m_lastPublishedIndex{ -1 };

    // Last item REPORTED through SelectionChanged; the delta derives from this.
    tracker_ref<winrt::IInspectable> m_lastRaisedSelectedItem{ this };

    // The selected item, held across an unload/reload round trip so re-sourcing the repeater does
    // not drop it. Nothing else defers.
    tracker_ref<winrt::IInspectable> m_pendingSelectedItem{ this };

    // The last INTENTIONALLY selected item (user gesture, programmatic set, or a restore), captured
    // by object identity in ApplySelection - the single selection writer. Unlike SelectedItem, it
    // is NOT cleared by a projection Reset that drops the model's indices, so it is the anchor an
    // identity-based restore re-selects against after a filter/sort/regroup reshape. A genuine
    // deselect flows through ApplySelection(-1) and nulls it, so an intentional clear is never
    // "restored".
    tracker_ref<winrt::IInspectable> m_stickySelectedItem{ this };

    // Tree sources only: node key of m_stickySelectedItem, so a re-created row keeps selection
    // across the Reset. Used only for Reset restore, never across a source swap.
    winrt::hstring m_stickySelectedIdentity;
    winrt::hstring m_pendingSelectedIdentity;
    winrt::hstring HierarchyIdentityForIndex(int32_t index) const;

    // Armed by OnSelectionSourceReset when a Reset drops the selection with a surviving sticky item,
    // consumed by OnSelectionItemsSourceCollectionChanged to run the identity restore once the
    // model has reconciled.
    bool m_resetSelectionRestorePending{ false };

    // Armed by OnSelectionSourceReset when the selected row leaves a view but its item stays in the
    // source (a live sort, filter or group move: ItemRemoved, then ItemInserted elsewhere). The row is
    // re-selected when it comes back, or the clear is published at the end of the turn.
    bool m_moveSelectionRestorePending{ false };
    bool IsItemInViewSource(winrt::IInspectable const& item) const;
    void FinishMoveSelectionRestore();

    // Bumped on every publish so a re-entrant one can tell that a newer selection overtook it and
    // it must not finish its own (now stale) notification.
    uint32_t m_selectionVersion{ 0 };

    // Set while a stashed selection is being restored after a reload, so the clear-then-reselect
    // that SelectionModel::Source forces is published once at the end rather than as two events.
    bool m_isRestoringSelection{ false };

    winrt::ItemsSourceView::CollectionChanged_revoker m_selectionCollectionChangedRevoker{};

    // Fires ahead of the SelectionModel on a projection Reset; see OnSelectionSourceReset.
    winrt::ItemsSourceView::CollectionChanged_revoker m_selectionResetDetectorRevoker{};

    // The views the two subscriptions above are attached to. Both re-register only when the view
    // actually changes, because SelectionModel::Source cannot be re-assigned for an unchanged view
    // (it clears the selection unconditionally) and so always keeps its original registration.
    // Re-registering these two against the same view would move them behind the model's and invert
    // the detector -> model -> restamp order ResolveSelectionAfterSourceChange documents.
    winrt::ItemsSourceView m_selectionCollectionChangedView{ nullptr };
    winrt::ItemsSourceView m_selectionResetDetectorView{ nullptr };

private:
    // Keyboard/focus position.
    tracker_ref<winrt::IInspectable> m_currentItem{ this };
    tracker_ref<winrt::TableViewColumn> m_currentColumn{ this };

    uint32_t m_layoutCallbackDepth{ 0 };

    void SetCurrentItem(winrt::IInspectable const& item);
    void UpdateCurrentColumn(winrt::TableViewColumn const& column);

    bool TryGetItemAtRowIndex(int32_t rowIndex, winrt::IInspectable& item) const;
    winrt::TableViewRow FindRealizedRowForItem(winrt::IInspectable const& item);

private:
    // A synchronous action owns its own operation state (and TiP test, when the operation has one) and shares the
    // control's session counters and error mask. withTest false tracks it for TableView_Error without a TiP test.
    class ActionTip
    {
    public:
        ActionTip(TableView& owner, TableViewTip::Operation operation, TableViewTip::Stage stage, bool admitted = true,
            bool withTest = true) noexcept;
        ~ActionTip();
        ActionTip(ActionTip const&) = delete;
        ActionTip& operator=(ActionTip const&) = delete;
        void Complete() noexcept { m_completed = true; }
        void Ignore(TableViewTip::IgnoreReason reason) noexcept { m_completed = false; m_ignoreReason = reason; }
        void Fail(std::optional<HRESULT> error) noexcept;
        // In a catch, before rethrowing: the exception leaving the action, so the app is blamed only when app code
        // threw it.
        void Escaping(TableViewTip::Fault fault) noexcept { m_escaping = fault; }
        // App code stopped the operation: its write did not land.
        void ApplicationFault(TableViewTip::AppFaultKind kind, std::optional<HRESULT> error = {}) noexcept;
        // App code failed (threw, or its write did not land) and TableView carried on: reported, and the
        // outcome stays TableView's.
        void RecoveredAppFault(TableViewTip::AppFaultKind kind, std::optional<HRESULT> error = {}) noexcept;
        uint64_t ControlGenerationAtAdmission() const noexcept { return m_controlGenerationAtAdmission; }

    private:
        void ReportAppFault(TableViewTip::AppFaultKind kind, std::optional<HRESULT> error, bool recoverable) noexcept;

        TableView& m_owner;
        ActionTip* m_previousAction{};
        std::optional<TableViewTip::OperationState> m_state; // only while tracked
        TableViewTip::Stage m_stage;
        uint64_t m_generation{};
        uint64_t m_controlGenerationAtAdmission{};
        uint64_t m_lifetimeGeneration{};
        int m_exceptions{};
        bool m_completed{};
        bool m_failed{};
        TableViewTip::IgnoreReason m_ignoreReason{ TableViewTip::IgnoreReason::Stale };
        std::optional<HRESULT> m_failError;
        std::optional<TableViewTip::Fault> m_escaping;
        TableViewTip::AppFaultKind m_appFaultKind{};
        std::optional<HRESULT> m_appFaultError;
        TableViewTip::AppFaultKind m_recoveredKind{};
        std::optional<HRESULT> m_recoveredError;
    };

    // Whether a synchronous action is tracked: only while a loaded session is active and the rows are not drained.
    bool TracksTipActions() const noexcept { return m_telemetrySession.active && !m_rowsSourceDrained; }
    uint64_t BeginTipOperation(TableViewTip::Operation operation, bool admitted = true) noexcept;
    void EndTipOperation() noexcept;
    void SupersedeTipOperation() noexcept;
    // App code failed (threw, or its write did not land) and TableView carried on without rethrowing.
    void NoteRecoveredAppFault(TableViewTip::Operation operation, TableViewTip::Stage stage,
        TableViewTip::AppFaultKind kind, std::optional<HRESULT> error) noexcept;
    ActionTip* CalloutAction() const noexcept;
    // An app fault ignores the operation; any other failure fails it.
    void FailTipOperation(uint64_t generation, TableViewTip::Stage stage, std::optional<TableViewTip::Fault> fault) noexcept;
    void CompleteTipOperation(uint64_t generation) noexcept;
    void IgnoreTip(TableViewTip::IgnoreReason reason) noexcept;
    TableViewTip::MetricsInputs TipMetricsInputs() noexcept;
    TableViewTip::MetricsInputs TipMetricsInputs(TableViewTip::Operation operation) noexcept;
    // Initialize: starts once the template is applied and a source is attached, and ends at the layout pass of the
    // first realized row or group header, or at once for an empty view.
    void StartTipInitialization() noexcept;
    void CompleteTipInitializationIfEmpty() noexcept;
    // After a source is attached or reclaimed: starts Initialize once the template is applied, and completes it for
    // an empty view. Before Initialize has its verdict, a source swap is part of it rather than a ReplaceSource
    // operation.
    void NoteTipInitializationSource() noexcept;
    // Setup (Stage::Template) or the source attach (Stage::Source) threw: Initialize's failure, or ignored when app
    // code threw, whether or not the test was running yet.
    void FailTipInitialization(TableViewTip::Stage stage, TableViewTip::Fault fault) noexcept;
    void ArmTipInitializationLayout() noexcept;
    void RevokeTipInitializationLayout() noexcept;
    // At unload: closes a waiting Initialize as UnloadedBeforeRender when the table had rows to lay out and a viewport,
    // yet laid none out.
    void IgnoreTipInitializationUnloadedBeforeRender() noexcept;
    // Whether the rows had room: the body scroller's viewport, or the rows repeater's size when no scroller hosts it.
    bool HasRowsViewport();
    // TableView_Instance's two sample points, Loaded and Unloaded (TableView_Telemetry.cpp).
    void SampleInstanceTelemetry() noexcept;

    // Telemetry: the loaded session's TableView_Instance. TiP: the initialization and operation tests.
    TableViewTelemetry::Session m_telemetrySession;
    TableViewTip::State m_tip{ m_telemetrySession };
    ActionTip* m_activeActionTip{};
    uint64_t m_tipLifetimeGeneration{};
    TableViewTip::IgnoreReason m_tipCancellationReason{ TableViewTip::IgnoreReason::Cancelled };
    uint32_t m_tipMutationDepth{};
    winrt::event_token m_tipInitializationLayoutToken{};
    bool m_templateApplied{};
    // The applied template lacks PART_RowsRepeater, PART_HeaderRow or PART_HeaderHost (TableView_Instance).
    bool m_templateIncomplete{};
    // TableView_Instance: how the current ItemsSource arrived (TableViewTelemetry::SourceKind), 0 with none.
    uint32_t m_telemetrySourceKinds{};

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
    void SyncHeaderScroller();
    void StartHeaderScrollAnimation();
    void UpdateHeaderScrollOffset();
    void StopHeaderScrollAnimation();

    // Defer ScrollViewer ancestor lookup until Loaded because ScrollViewer template names are shadowed.
    void OnHeaderHostLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnRowsRepeaterLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);

    // ThemeSettings must be created once a XamlRoot/WindowId is available (Loaded); its Changed handler
    // refreshes HC-dependent resources directly on the UI thread (Changed is raised there).
    void OnTableViewLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnXamlRootChanged(const winrt::XamlRoot& sender, const winrt::XamlRootChangedEventArgs& args);
    void OnThemeSettingsChanged(const winrt::Microsoft::UI::System::ThemeSettings& sender, const winrt::IInspectable& args);

    void UpdateHeaderVisibility();
    void ApplyGridLinesToHeader();
    void RefreshGridLinesOnRealizedRows();
    void QueueTerminalGridLineRefresh(bool isGeometryRetry = false);
    void RefreshTerminalGridLines();
    std::optional<winrt::Rect> GetInnerBorderBounds();
    std::optional<bool> ShouldSuppressTrailingGridLine();
    std::optional<bool> ShouldSuppressBottomGridLine(
        const winrt::FrameworkElement& element,
        bool hasBottomGridLine);
    void RefreshRowBackgroundsOnRealizedRows();
    void QueueGroupExpansionRowRefresh();
    void RefreshRealizedRowsAfterGroupExpansion();

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

    void PrepareGroupHeaderElement(winrt::TableViewGroupHeader const& header, int32_t index);
    void ClearGroupHeaderElement(winrt::TableViewGroupHeader const& header);
    void UpdateGroupHeaderWidth(winrt::TableViewGroupHeader const& header);

    // Split responsibilities driven off the ItemsSource DP:
    //   AdoptItemsSource   - source lifetime. Runs only when ItemsSource actually changes: route the
    //                        source through a view (an ICollectionView is used as is; any other
    //                        collection is wrapped in an internal CollectionViewSource, as WPF wraps
    //                        every ItemsSource in a view) and build the row projection over it.
    //   RefreshRowsPipeline- pushes the projection's rows into the repeater (identity-guarded),
    //                        re-reads its row-metadata provider, and re-resolves empty-state +
    //                        selection. Runs on every re-entry (template applied, repeater
    //                        reloaded, projection rebuilt) with no lifetime work.
    void AdoptItemsSource();
    void RefreshRowsPipeline();
    // Re-reads the projection's rows and row metadata without touching the repeater.
    void RefreshCachedProjection();
    // Raised by the row flattener after it rebuilt (flat <-> grouped <-> tree, or a tree or group set
    // rebuilt in place), so the cached view / row metadata are re-read.
    void OnFlattenedRowsChanged();
    // After a sort the view applied: restores body focus and tells a UIA client the rows were reordered
    // (a programmatic re-sort has no input event behind it).
    void OnRowsReorderedBySort();
    // The arrow follows the view's primary SortDescription: a sort the app (or another consumer of the
    // view) applies lights the column that sorts the same path.
    void ReconcileSortStateWithSource();
    void QueueReconcileSortStateWithSource();
    // Suppresses ReconcileSortStateWithSource for the duration of a control-initiated verb, whose
    // own source mutations would otherwise read as the app taking over.
    [[nodiscard]] auto BeginControlInitiatedSortScope(uint64_t tipGeneration = 0)
    {
        m_isApplyingControlInitiatedSort = true;
        const auto previousGeneration = std::exchange(m_controlSortTipGeneration,
            tipGeneration ? tipGeneration : (std::numeric_limits<uint64_t>::max)());
        return gsl::finally([this, previousGeneration]()
        {
            m_isApplyingControlInitiatedSort = false;
            m_controlSortTipGeneration = previousGeneration;
        });
    }
    // EmptyTemplate shows only for null or empty row sources.
    void UpdateEmptyState();
    void UpdateItemsSourceCollectionChangedSubscription();
    void OnItemsSourceCollectionChanged(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    winrt::event_token m_columnsVectorChangedToken{};

    // --- Rows ---
    //
    // The rows the repeater is driven from: the view itself for flat rows, else the flattener's grouped
    // or tree rows. Cached because the flattener can switch rows underneath us.
    winrt::ItemsSourceView m_rowsItemsSourceView{ nullptr };

    // The CollectionViewSource this table wraps a plain ItemsSource in; null when the app supplied an
    // ICollectionView. A tracker_ref, so the GC's cycle walker sees this edge into the app's collection.
    tracker_ref<winrt::Microsoft::UI::Xaml::Data::CollectionViewSource> m_internalViewSource{ this };
    // Presents the view's items as rows (flat, grouped or tree). Never sorts, filters or groups.
    // Reassigned only by AdoptItemsSource.
    std::shared_ptr<TableViewRowFlattener> m_rowFlattener{};

    // The view's shaping, when it has any. A header sort writes the column's sort to it (SortDescriptions,
    // or CustomSort for a CustomSortComparer column). Null for a view that cannot be sorted this way (an
    // app's own ICollectionView, an IsSourceGrouped view): a header click then raises Sorting only, and the
    // app sorts, as WPF's DataGrid sorts only a view that CanSort.
    winrt::Microsoft::UI::Xaml::Data::ICollectionViewShaping m_collectionViewShaping{ nullptr };
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::Microsoft::UI::Xaml::Data::SortDescription>::VectorChanged_revoker m_collectionViewSortChangedRevoker{};
    bool m_updatingCollectionViewSort{ false };
    // The view's CustomSort this table installed for a CustomSortComparer column; the arrow stays
    // on that column while the view still sorts with it.
    winrt::Microsoft::UI::Xaml::Data::CollectionViewComparer m_collectionViewCustomSort{ nullptr };
    // The view's current sort is one this table wrote (not the app's): only then does an implicit
    // clear remove it, or a new CustomSortComparer re-sort it.
    bool m_collectionViewSortByTable{ false };
    // Row semantics for the current projection (row kind, identity, group expansion). Null for flat rows,
    // where a row is its item.
    TableViewRowMetadataProvider m_tableViewSourceRowMetadata{};
    // Bumped on every metadata swap so a request captured against the previous provider can tell
    // that the provider which produced its identity is gone. Identities are value-based strings,
    // so without this a queued group toggle could resolve against a same-named group in a
    // brand-new source.
    uint64_t m_rowMetadataGeneration{ 0 };

    // Which axis a bulk command drives; both share the coalescing/focus/restamp machinery.
    enum class BulkExpansionAxis
    {
        Groups,
        Rows,
    };

    // Bumped when a bulk expand/collapse is REQUESTED (not when a deferred one replays), per axis,
    // so a queued per-item request that predates the bulk is dropped instead of re-creating the
    // exception the bulk set erases, while one made after it - or on the other axis - survives.
    uint64_t m_groupBulkExpansionGeneration{ 0 };
    uint64_t m_rowBulkExpansionGeneration{ 0 };
    uint64_t& BulkExpansionGeneration(BulkExpansionAxis axis)
    {
        return axis == BulkExpansionAxis::Rows ? m_rowBulkExpansionGeneration : m_groupBulkExpansionGeneration;
    }

    void RequestGroupExpansion(winrt::UIElement const& container, std::optional<bool> desired, bool subtree = false);
    void QueueGroupExpansionByIdentity(
        winrt::hstring const& identity,
        std::optional<bool> desired,
        BulkExpansionAxis axis,
        bool subtree = false);
    void ApplyGroupExpansionByIdentity(
        winrt::hstring const& identity,
        std::optional<bool> desired,
        uint64_t generation,
        BulkExpansionAxis axis,
        uint64_t bulkGeneration,
        bool subtree = false);
    void RaiseGroupStructureChanged();
    void AnnounceRowExpansionChange(winrt::hstring const& identity);
    std::optional<winrt::ExpandCollapseState> TryGetGroupExpandCollapseState(winrt::hstring const& identity);

    // Group identity -> ExpandCollapseState before a toggle that has not reached its header yet.
    std::unordered_map<winrt::hstring, winrt::ExpandCollapseState> m_pendingGroupExpansionStates;
    // Cleared with m_rowMetadataGeneration: identities are only meaningful within one projection.
    std::unordered_map<winrt::hstring, winrt::AutomationPeer> m_groupAutomationElements;

    void SetBulkExpansion(bool expand, BulkExpansionAxis axis);
    void ApplyBulkExpansion(bool expand, BulkExpansionAxis axis);
    void SetGroupExpansionByKey(winrt::IInspectable const& groupKey, bool expand);
    // Never raised inside measure or arrange: every caller runs on a later turn or is posted out of layout.
    void RaiseGroupExpanding(winrt::IInspectable const& groupKey);
    void RaiseGroupExpandingFor(winrt::hstring const& identity, winrt::IInspectable const& groupKey);
    bool IsRaisingGroupExpanding(winrt::hstring const& identity) const;
    // Identities whose GroupExpanding is being raised; a nested request for one of them is a no-op.
    std::vector<std::pair<winrt::hstring, uint64_t>> m_groupsRaisingExpanding;
    void RaiseGroupCollapsed(winrt::IInspectable const& groupKey);

    void ForEachRealizedGroupHeader(std::function<void(winrt::TableViewGroupHeader const&, int32_t)> const& fn);

    // A reshape can rebind a pooled header without ElementPrepared/ElementIndexChanged, leaving its
    // expansion DPs describing the previous projection.
    void RefreshRealizedGroupHeadersAfterExpansion();

    // Keyboard-driven group toggle loses focus without this: the Enter/Space toggle defers a
    // structural reshape that recycles the focused header container, dropping focus (and its
    // visual) to nothing. Capture the header's identity + FocusState at gesture time, then restore
    // focus to the same group's header after the relayout settles, unless focus left the table.
    // Also covers focused hierarchical rows a reset recycles (keyboard / programmatic focus only).
    void CaptureContainerFocusForRestore(
        winrt::UIElement const& container,
        winrt::hstring const& identity,
        winrt::FocusState gestureState = winrt::FocusState::Unfocused);
    winrt::hstring CaptureFocusedContainerForRestore();
    void RestoreContainerFocusIfPending(winrt::hstring const& identity);
    void ArmContainerFocusRestore(
        winrt::hstring const& identity, winrt::FocusState focusState, bool fromPointerGesture, int32_t attemptsLeft);
    bool FocusContainerByIdentity(
        winrt::hstring const& identity, winrt::FocusState focusState, bool fromPointerGesture, bool finalAttempt);
    static constexpr int32_t s_containerFocusRestoreAttempts{ 4 };
    bool CanRestoreContainerFocus();
    // Row identity for a realized container. Identity is index-independent once captured.
    winrt::hstring TryGetContainerIdentity(winrt::UIElement const& container);

    winrt::hstring StringifyGroupKey(winrt::IInspectable const& key);
    // Cached because resolving the culture formatter is measurably expensive and group-key text is
    // produced during measure, once per realized header.
    winrt::DecimalFormatter GetGroupKeyDecimalFormatter();
    winrt::DecimalFormatter m_groupKeyDecimalFormatter{ nullptr };
    int32_t m_groupKeyDefaultFractionDigits{ 0 };

    // Chooses between the row and group-header container templates. Held so ~TableView can drop
    // the recycle pools it caches; see TableViewRowTemplateSelector::Detach.
    tracker_ref<winrt::TableViewRowTemplateSelector> m_rowTemplateSelector{ this };

    // --- Sorting ---
    //
    // Re-validated after every point where app code could have run (a Sorting handler): the
    // columns collection may have changed underneath the request that is still in flight.
    // requireCanSort=false skips the sort-key check; only for clearing a column's own sort.
    bool IsSortRequestStillValid(const winrt::TableViewColumn& column, bool requireCanSort = true) const;
    bool IsSortClearStillValid() const;
    bool CanSortColumnNow(const winrt::TableViewColumn& column, bool requireCanSort = true) const;
    // Writes the single-column sort state into the columns; does not reshape.
    void ApplySingleColumnSortState(const winrt::TableViewColumn& column, winrt::SortDirection direction);
    // Writes the column's sort to the view's ICollectionViewShaping: its SortDescriptions, or, for a
    // CustomSortComparer column, its CustomSort. Returns false when the view has no shaping, the trigger
    // resolves no sort key, or the column's comparer threw (the view's previous sort is put back).
    // explicitClear: the app or user asked to clear, so a clear also removes the app's sort. Any other
    // clear (a column leaving, CanUserSortColumns turning off) removes only a sort this table wrote.
    bool CanSortThroughCollectionView(const winrt::TableViewColumn& column, winrt::SortDirection direction) const;
    bool SyncCollectionViewSort(const winrt::TableViewColumn& trigger, winrt::SortDirection direction, bool explicitClear);
    // resort: a key reconcile offering the column's current key; re-sorts even in the same direction,
    // and a cancel declines that key.
    bool SortByColumnInternal(const winrt::TableViewColumn& column, winrt::SortDirection direction, bool measureTip,
        bool requireCanSort = true, bool resort = false);
    bool ClearSortInternal(bool measureTip, bool explicitClear = false);
    // The bodies of ToggleSortDirection and ClearSortInternal, run inside one sort transition.
    bool ToggleSortDirectionCore(const winrt::TableViewColumn& column);
    bool ClearSortCore(bool measureTip, bool explicitClear);
    // The lit column's direction was set by the app through SortDirection, not applied by this table.
    bool IsSortLitOnlyByApp() const;
    // offersKey: the request re-sorts the trigger on its current key, so a cancel declines that key.
    bool RaiseSortingAndCheckCanceled(const winrt::TableViewColumn& trigger, winrt::SortDirection direction, bool offersKey = false);

    // Sorting and Sorted handlers may change a column's sort key. A change made inside a transition
    // only marks the table dirty; once the outermost transition ends, a reconcile on the next
    // dispatcher turn compares the sorted column's key with the key the view's sort was written with
    // (m_installedSortKey) and with the key a cancelled Sorting declined. A decline lasts only while
    // the column's key still equals it.
    template <typename F> bool RunSortTransition(F&& transition);
    void ScheduleSortKeyReconcile(bool allowSynchronous);
    void ReconcileSortKeys();
    void ReconcileSortKey(const winrt::TableViewColumn& column);
    void RecordDeclinedSortKey(const winrt::TableViewColumn& column);
    void ForgetDeclinedSortKeys(const winrt::TableViewColumn& column = nullptr);
    // Forgets the column's decline once its key moved off it; true if the key is still declined.
    bool IsSortKeyStillDeclined(const winrt::TableViewColumn& column);
    // The installed key is this column's: the view's sort is one this table wrote for it.
    bool IsInstalledSortKeyFor(const winrt::TableViewColumn& column) const;
    struct DeclinedSortKey
    {
        winrt::weak_ref<winrt::TableViewColumn> Column;
        winrt::hstring MemberPath;
        // Weak: the decline must not keep the app's comparer alive. Dead means expired.
        winrt::weak_ref<winrt::ITableViewSortComparer> Comparer;
        // Only for a comparer that can't be weakly referenced; compared, never called.
        winrt::ITableViewSortComparer StrongComparer{ nullptr };
        bool HasComparer{ false };
        // Any later key change, even one made while the column was out of the table, ends the decline.
        uint32_t Generation{ 0 };
    };
    // The key the view's current sort was written with: a SortDescription path, or the column's
    // comparer (compared by identity), and the column's key generation at that time.
    struct InstalledSortKey
    {
        winrt::weak_ref<winrt::TableViewColumn> Column;
        winrt::hstring MemberPath;
        winrt::ITableViewSortComparer Comparer{ nullptr };
        uint32_t Generation{ 0 };
    };
    std::optional<InstalledSortKey> m_installedSortKey;
    std::vector<DeclinedSortKey> m_declinedSortKeys;
    uint32_t m_sortTransitionDepth{ 0 };
    bool m_sortKeysDirty{ false };
    bool m_sortKeyReconcileQueued{ false };
    bool m_reconcilingSortKeys{ false };
    // Single funnel for "the sort state has been written to the columns": reshapes, restores the
    // selection, raises Sorted, and announces.
    // A non-null trigger is always an explicit request; a null one is explicit only when asked.
    // applyDirection: write this direction to the trigger's arrow once the reshape succeeded.
    bool RecomputeSortDPsAndRaiseInternal(const winrt::TableViewColumn& trigger, bool measureTip = true, bool explicitClear = false,
        std::optional<winrt::SortDirection> applyDirection = std::nullopt);

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
    // Removes `column` from the sort state; queues the clear when it was sorted.
    void PurgeLeavingColumnFromSortState(const winrt::TableViewColumn& column);
    // IObservableVector has no Move, so RemoveAt + InsertAt of the same instance before the queued
    // clear runs is a move: the column gets its sort back and the clear finds nothing to do.
    void RestoreSortOfMovedColumn(const winrt::TableViewColumn& column);
    // Any other change to the sort or to the columns while the column is out makes the restore stale.
    void CancelPendingSortRestore()
    {
        m_sortedColumnPendingRemoval = nullptr;
        m_sortedColumnPendingRemovalDeclined.reset();
        m_sortedColumnPendingRemovalAppLit = false;
    }
    winrt::weak_ref<winrt::TableViewColumn> m_sortedColumnPendingRemoval;
    winrt::SortDirection m_sortedColumnPendingRemovalDirection{ winrt::SortDirection::None };
    std::optional<DeclinedSortKey> m_sortedColumnPendingRemovalDeclined;
    // The leaving column's direction was one the app only displayed.
    bool m_sortedColumnPendingRemovalAppLit{ false };

    // v1 is single-column sort, so this holds at most one entry. It stays a vector because the
    // clear/purge walks are written against the collection and multi-column sort is the expected
    // next step. Weak refs: a column removed from Columns must not be kept alive by sort state.
    std::vector<tracker_ref<winrt::TableViewColumn>> m_sortedColumns;
    // The lit column's direction was written by the app through SortDirection and has not ordered the rows.
    bool m_sortDisplaySetByApp{ false };
    // The control's own SortDirection writes run inside this scope; app writes made meanwhile are
    // applied when the outermost scope ends.
    [[nodiscard]] auto BeginSortStateUpdate()
    {
        ++m_sortStateUpdateDepth;
        return gsl::finally([this]() { EndSortStateUpdate(); });
    }
    void EndSortStateUpdate() noexcept;
    std::vector<winrt::TableViewColumn> SnapshotSortedColumns() const;
    uint32_t m_sortStateUpdateDepth{ 0 };
    // Set by SyncCollectionViewSort when the column's comparer threw while the view sorted.
    std::optional<TableViewTip::ContainedFaults> m_sortAbortedByComparerFault;
    std::vector<winrt::weak_ref<winrt::TableViewColumn>> m_deferredAppSortWrites;
    void AdoptColumnSortDirection(winrt::TableViewColumn const& column);

    tracker_ref<winrt::ItemsRepeater> m_rowsRepeater{ this };
    tracker_ref<winrt::ContentControl> m_emptyStatePresenter{ this };
    tracker_ref<winrt::FrameworkElement> m_headerRow{ this };
    tracker_ref<winrt::Panel> m_headerHost{ this };
    tracker_ref<winrt::ScrollViewer> m_headerScroller{ this };
    winrt::ScrollViewer::ViewChanged_revoker m_headerScrollerViewChangedRevoker{};
    winrt::UIElement::ContextRequested_revoker m_headerContextRequestedRevoker{};
    // Keeps the header band locked to the body when focus moves to an off-screen header.
    winrt::UIElement::BringIntoViewRequested_revoker m_headerBringIntoViewRevoker{};
    winrt::UIElement::BringIntoViewRequested_revoker m_bodyBringIntoViewRevoker{};
    // Targets of the requests ReleaseRowsRepeaterAnchor raised; OnBodyBringIntoViewRequested swallows them.
    std::vector<winrt::weak_ref<winrt::UIElement>> m_anchorReleaseTargets;
    tracker_ref<winrt::ScrollViewer> m_bodyScroller{ this };
    double m_headerRequestedOffset{ std::numeric_limits<double>::quiet_NaN() };
    // The expression doesn't keep the property sets it reads alive.
    winrt::ExpressionAnimation m_headerScrollAnimation{ nullptr };
    winrt::CompositionPropertySet m_headerScrollOffset{ nullptr };
    winrt::CompositionPropertySet m_bodyManipulationProperties{ nullptr };

    winrt::event_token m_rowElementPreparedToken{};
    winrt::event_token m_rowElementClearingToken{};
    winrt::event_token m_rowElementIndexChangedToken{};
    winrt::event_token m_bodyScrollerViewChangedToken{};
    // Body-viewport resize invalidates measure so Star columns resolve during the table layout pass.
    winrt::FrameworkElement::SizeChanged_revoker m_bodyScrollerSizeChangedRevoker{};
    winrt::ScrollViewer::DirectManipulationStarted_revoker m_bodyManipulationStartedRevoker{};
    winrt::ScrollViewer::DirectManipulationCompleted_revoker m_bodyManipulationCompletedRevoker{};
    winrt::event_token m_headerHostLoadedToken{};
    // Set while a drag is in flight, so Escape can reach the gripper that owns it.
    std::shared_ptr<ColumnResizeDragState> m_activeColumnResizeDrag{};
    std::vector<LockedColumnHold> m_lockedColumnHolds;
    size_t m_lockedColumnHoldsSignature{ 0 };
    // True while a control-initiated sort verb is mutating the source. Its own mutations must not
    // be mistaken for the app declaring a sort behind the control's back.
    bool m_isApplyingControlInitiatedSort{ false };
    uint64_t m_controlSortTipGeneration{};
    uint64_t m_controlSortPublishedGeneration{};
    bool m_sortReconcileQueued{ false };
    winrt::event_token m_rowsRepeaterLoadedToken{};
    // Terminal-edge geometry is evaluated once after layout settles; gridline rendering then
    // changes without changing row/cell layout thickness.
    winrt::event_token m_terminalGridLinesLayoutToken{};
    winrt::event_token m_headerFocusRestoreLayoutToken{};
    // Focus left a removed column's header, for a remaining one or parked on the removed cell; the
    // rebuild returns it to the column if the column came back (a move), else moves it to the body.
    winrt::weak_ref<winrt::TableViewColumn> m_headerFocusRemovedColumn{ nullptr };
    winrt::weak_ref<winrt::TableViewColumn> m_headerFocusFallbackColumn{ nullptr };
    winrt::FocusState m_headerFocusRemovedState{ winrt::FocusState::Unfocused };
    winrt::weak_ref<winrt::TableViewRow> m_terminalGridLineRow{};
    winrt::weak_ref<winrt::TableViewGroupHeader> m_terminalGridLineGroupHeader{};
    winrt::FrameworkElement::SizeChanged_revoker m_terminalGridLineRowSizeChangedRevoker{};
    bool m_suppressTrailingGridLine{ false };
    bool m_suppressBottomGridLine{ false };
    bool m_terminalGridLineGeometryRetryAvailable{ true };
    int32_t m_terminalGridLineColumnIndex{ -1 };
    double m_terminalGridLineHorizontalOffset{ std::numeric_limits<double>::quiet_NaN() };
    double m_terminalGridLineVerticalOffset{ std::numeric_limits<double>::quiet_NaN() };
    // Deferred restore of keyboard focus to a group header after a toggle reshape recycles it.
    winrt::event_token m_pendingGroupFocusLayoutToken{};
    winrt::hstring m_pendingGroupFocusIdentity{};
    winrt::FocusState m_pendingGroupFocusState{ winrt::FocusState::Unfocused };
    bool m_pendingGroupFocusFromPointerGesture{ false };
    winrt::event_token m_pendingGroupRowRefreshLayoutToken{};
    // Count changes refresh both empty state and the terminal row separator.
    winrt::ItemsSourceView::CollectionChanged_revoker m_itemsSourceCollectionChangedRevoker{};
    // Only edge that restamps realized rows when hierarchy metadata changes without the row set
    // changing (e.g. a node losing its last child).
    winrt::ItemsSourceView::CollectionChanged_revoker m_rowHierarchyResetRevoker{};
    winrt::ItemsSourceView m_rowHierarchyResetView{ nullptr };
    bool m_rowHierarchyRefreshQueued{ false };

    // --- Re-shape focus restore (dev-spec Keyboard, "Re-shape while focused"); see TableView_Keyboard.cpp ---
    void CaptureBodyFocusForReshape();
    void RestoreBodyFocusAfterReshape();
    bool TryResolveLiveBodyFocusPosition(int32_t& row, int32_t& column, bool& cellLevel) const;
    bool IsElementInBody(const winrt::DependencyObject& element) const;
    bool IsFocusOnNonBodyElement() const;
    void ForgetBodyFocusPosition();
    static winrt::FocusState GetRestoreFocusStateFor(const winrt::IInspectable& focusedElement);
    void TrackBodyFocusPosition();
    void OnTableViewGotFocusForBodyTracking(
        const winrt::IInspectable& sender,
        const winrt::RoutedEventArgs& args);
    void OnTableViewLosingFocusForBodyTracking(
        const winrt::IInspectable& sender,
        const winrt::Microsoft::UI::Xaml::Input::LosingFocusEventArgs& args);
    void SuppressBodyFocusTrackingForThisTurn();
    bool m_lastBodyFocusValid{ false };
    int32_t m_lastBodyFocusRow{ -1 };
    int32_t m_lastBodyFocusColumn{ 0 };
    bool m_lastBodyFocusCellLevel{ false };
    winrt::FocusState m_lastBodyFocusState{ winrt::FocusState::Keyboard };
    tracker_ref<winrt::IInspectable> m_lastBodyFocusItem{ this };
    bool m_suppressBodyFocusTracking{ false };
    winrt::UIElement::GotFocus_revoker m_bodyFocusTrackerGotFocusRevoker{};
    winrt::UIElement::LosingFocus_revoker m_bodyFocusTrackerLosingFocusRevoker{};
    void ArmReshapeFocusAttempt();
    void OnReshapeFocusAttempt();
    void CancelReshapeFocusRestore();
    void CancelReshapeFocusRestoreForUserInput();
    bool TryLandReshapeFocus();
    bool IsBodyFocusAtRow(int32_t rowIndex) const;
    bool IsBodyFocusInContainer(winrt::UIElement const& container) const;
    bool IsInUnrealizedBodyContainer(winrt::DependencyObject const& element) const;
    // Keyboard moves to a row that is not arranged yet finish through the same attempt loop.
    void BeginPendingBodyFocus(
        int32_t row, int32_t column, bool cellLevel, std::optional<double> alignRatio, winrt::IInspectable const& item = nullptr);
    bool IsPendingBodyFocusItemAt(int32_t row) const;
    winrt::IInspectable GetRowsSourceItemAt(int32_t index) const;
    bool ArrangeAndRevalidate(winrt::UIElement const& element, int32_t index, winrt::IInspectable const& expectedItem);
    static bool IsRowElementArranged(winrt::UIElement const& element);
    bool IsRowElementInViewport(winrt::UIElement const& element) const;
    bool BringRowElementIntoView(winrt::UIElement const& element, std::optional<double> alignRatio);
    bool ScrollBodyToRowElement(winrt::UIElement const& element, std::optional<double> alignRatio);
    bool ScrollBodyToIndex(int32_t index, std::optional<double> alignRatio);
    void RequestBodyVerticalOffset(double offset);
    bool IsBodyScrollInFlight();
    std::optional<double> m_requestedBodyVerticalOffset{};
    void RevealFocusedBodyElement(winrt::UIElement const& element);
    bool IsKeyboardBodyFocusPending() const { return m_pendingBodyFocusFromKeyboard && m_pendingReshapeFocusRow >= 0; }
    void QueueReshapeFocusCaptureDisarm();
    void UpdateReshapeFocusDetectorSubscription(const winrt::ItemsSourceView& view);
    void OnRowsSourceResetForFocus(
        const winrt::IInspectable& sender,
        const winrt::NotifyCollectionChangedEventArgs& args);
    // A live move (Remove + Insert) or a Reset the view raised on its own: focus follows the row's item.
    void CaptureBodyFocusForMove(int32_t focusedRow);
    bool RestoreBodyFocusToCapturedItem();
    // The focused body row before a rows source change: the change handler runs before ItemsRepeater's.
    int32_t GetBodyFocusRowBeforeChange() const;
    bool IsBodyRowInViewport(int32_t row) const;
    void ShiftLastBodyFocusRow(const winrt::NotifyCollectionChangedEventArgs& args);
    winrt::DependencyObject GetFocusedBodyElement() const;
    bool IsInRecycledRow(const winrt::DependencyObject& element) const;
    void RecordFocusRescueSource();
    void NoteFocusChangeWhileFollowingItem(const winrt::DependencyObject& oldFocus, const winrt::DependencyObject& newFocus);
    // The element ItemsRepeater moves focus away from when it recycles the focused row; any other focus move
    // while focus follows an item is the app's or the user's, and wins.
    winrt::weak_ref<winrt::DependencyObject> m_focusRescueFrom{ nullptr };
    bool m_reshapeFocusMovedElsewhere{ false };
    bool m_reshapeFocusValid{ false };
    bool m_reshapeFocusDisarmQueued{ false };
    int32_t m_reshapeFocusRow{ -1 };
    int32_t m_reshapeFocusColumn{ 0 };
    bool m_reshapeFocusCellLevel{ false };
    winrt::FocusState m_reshapeFocusState{ winrt::FocusState::Keyboard };
    tracker_ref<winrt::IInspectable> m_reshapeFocusItem{ this };
    bool m_reshapeFocusFollowsItem{ false };
    // The consumed capture, replayed until it sticks.
    int32_t m_pendingReshapeFocusRow{ -1 };
    int32_t m_pendingReshapeFocusColumn{ 0 };
    bool m_pendingReshapeFocusCellLevel{ false };
    winrt::FocusState m_pendingReshapeFocusState{ winrt::FocusState::Keyboard };
    std::optional<double> m_pendingReshapeFocusAlign{};
    // Keyboard pending focus follows its item, so source edits above it cannot retarget the move.
    winrt::IInspectable m_pendingBodyFocusItem{ nullptr };
    // Set by BeginPendingBodyFocus: a keyboard move scrolling to its row, not a re-shape restore.
    bool m_pendingBodyFocusFromKeyboard{ false };
    // Where a pending keyboard move started (the header for Down); focus still there does not cancel it.
    winrt::weak_ref<winrt::DependencyObject> m_pendingBodyFocusOrigin{ nullptr };
    int32_t m_reshapeFocusAttemptsLeft{ 0 };
    // Set for the turn a restore is armed in; that turn's input triggered the re-shape.
    bool m_reshapeFocusArmedThisTurn{ false };
    bool m_reshapeFocusTurnQueued{ false };
    winrt::event_token m_reshapeFocusLayoutToken{};
    // Covers realize -> arrange -> deferred re-shape work, yet bounded so a restore never fights
    // the user for focus.
    static constexpr int32_t s_reshapeFocusRestoreAttempts{ 24 };
    // A far keyboard jump over estimated row heights can take a few scroll-and-realize rounds.
    static constexpr int32_t s_keyboardFocusAttempts{ 48 };
    // Set while an attempt waits for a body scroll to apply; only layout passes re-run it then.
    bool m_reshapeFocusWaitForLayout{ false };
    // Subscribed before the repeater so a Reset is observed while the rows are still realized.
    winrt::ItemsSourceView::CollectionChanged_revoker m_reshapeFocusDetectorRevoker{};
    winrt::ItemsSourceView m_reshapeFocusDetectorView{ nullptr };
    // ActualThemeChanged refreshes imperatively-resolved brushes that ItemsRepeater rows do not re-pump.
    winrt::event_token m_actualThemeChangedToken{};

    // ThemeSettings (lifted WinUI3) reports the system High Contrast setting and raises Changed on the
    // control's UI thread -- unlike AccessibilitySettings.HighContrastChanged, which could be delivered
    // off-thread. It requires a WindowId, so it is created on Loaded (once a XamlRoot exists), not in
    // the constructor, and released on Unloaded, AppWindow.Destroying, or destruction.
    winrt::Microsoft::UI::System::ThemeSettings m_themeSettings{ nullptr };
    winrt::Microsoft::UI::System::ThemeSettings::Changed_revoker m_themeSettingsChangedRevoker{}; // Runtime HC toggles must refresh cached HC-dependent brushes.
    // remove_Changed fail-fasts once the window is gone, hence the Destroying release. A token, not
    // auto_revoke: AppWindow has no weak reference support.
    winrt::Microsoft::UI::Windowing::AppWindow m_appWindow{ nullptr };
    winrt::event_token m_appWindowDestroyingToken{};
    // Cached HC state: kept fresh by ThemeSettings.Changed while loaded and read by IsHighContrast().
    // Only touched on the UI thread now, so no atomic is required.
    bool m_isHighContrast{ false };
    winrt::FrameworkElement::Loaded_revoker m_loadedRevoker{};
    winrt::XamlRoot::Changed_revoker m_xamlRootChangedRevoker{};
    double m_layoutRasterizationScale{ 0.0 };

    winrt::FrameworkElement::Unloaded_revoker m_unloadedRevoker{};
    void OnTableViewUnloaded();
    void ReleaseThemeSettings() noexcept;
    bool m_rowsSourceDrained{ false };

    bool IsContextMenuTargetCurrent(TableViewDetails::ContextMenuTarget const& target) const;
    // IsContextMenuTargetCurrent plus, for body targets, an unchanged source.
    bool IsContextMenuRequestCurrent(
        TableViewDetails::ContextMenuTarget const& target,
        winrt::IInspectable const& sourceSnapshot);
    // Moves focus (and, for a cell, the current cell) to a context menu target. Returns false when
    // app code run by the move made the request stale.
    bool FocusContextMenuTarget(
        TableViewDetails::ContextMenuTarget const& target,
        winrt::IInspectable const& sourceSnapshot);
    void OnHeaderContextRequested(winrt::ContextRequestedEventArgs const& args);
    std::optional<TableViewDetails::ContextMenuTarget> ResolveHeaderContextMenuTarget(
        winrt::ContextRequestedEventArgs const& args);
    TableViewDetails::ContextMenuCandidate ResolveContextFlyout(
        TableViewDetails::ContextMenuTarget const& target,
        bool includeTableFlyout);
    void ShowContextFlyout(
        TableViewDetails::ContextMenuTarget const& target,
        TableViewDetails::ContextMenuCandidate const& candidate,
        winrt::ContextRequestedEventArgs const& args);
    bool m_isProcessingContextMenu{ false };

    double ComputeLeadingFrozenWidth();
    void RefreshFrozenColumns();

    double GetHeaderMeasuredWidthForColumn(const winrt::TableViewColumn& column) const;
    void ResolveColumnWidths();
    // Only Auto columns read realized-row widths, so row prepare/clear re-resolves only when one exists.
    bool HasVisibleAutoColumn();
    void InvalidateMeasureForRowRealizationChange();
    void ResetColumnDesiredWidths();
    // Limits a resize gesture to the width the other columns can absorb. Unbounded when nothing
    // constrains it.
    ColumnResizeBounds ResizeBoundsForColumn(const winrt::TableViewColumn& column);
    bool IsColumnResizePinned(const winrt::TableViewColumn& column);
    // Collapses the gripper of every pinned column; it stays in the tree for keyboard resize lookup.
    void RefreshResizeGripperAvailability();
    void FreezeColumnsBeforeResize(const winrt::TableViewColumn& column, ColumnResizeDragState& state);
    // Hash of the hold inputs a user resize does not change: the viewport width, the Columns identities,
    // and each column's authored Width, MinWidth (and whether it is local), MaxWidth, Visibility and
    // CanUserResize. Auto measurements and the RasterizationScale are not hashed; ResolveColumnWidths
    // checks Auto growth separately.
    size_t LockedColumnHoldsSignature();
    void ClearLockedColumnHolds(bool invalidate);
    winrt::ResizeGripper GetAvailableResizeGripper(const winrt::TableViewColumn& column);
    bool IsColumnResizeAllowed(const winrt::TableViewColumn& column);
    static void RestoreColumnWidth(const winrt::TableViewColumn& column, const winrt::IInspectable& localWidth);
    // The drag's width write, clamped; false when the width is already there.
    static bool ApplyColumnResizeDrag(TableView* owner, const winrt::TableViewColumn& col, ColumnResizeDragState& state, double totalDelta);
    // Re-invalidate the header + realized row cells panels so they re-measure/arrange after a resolve.
    void InvalidateCellPanels();

    bool m_frozenColumnsActive{ false };

    bool m_rebuildHeadersQueued{ false };
    bool m_rebuildingHeaders{ false };
    bool m_inRebuildHeaders{ false };
    bool m_rebuildHeadersRequested{ false };
    // Incompatible header Styles already reported, so each is logged once.
    std::vector<winrt::weak_ref<winrt::Style>> m_loggedIncompatibleHeaderStyles;

    bool m_columnsStructureChangedQueued{ false };

    // Per-instance resource cache; replaces the former process-global map keyed by `this`.
    TableViewResourceCache m_resourceCache{};

    std::vector<tracker_ref<winrt::TableViewColumn>> m_trackedColumns;

    winrt::KeyEventHandler m_keyDownHandler{ nullptr };  // Root KeyDown (handledEventsToo); registration is released with the element, no explicit RemoveHandler needed.
    // Set when a row already ran this key through OnKeyDownForNavigation.
    bool m_navKeyHandledEarly{ false };
    void OnKeyDownForNavigation(
        const winrt::IInspectable& sender,
        const winrt::KeyRoutedEventArgs& args);

    bool TryHandleHeaderColumnResizeKey(const winrt::KeyRoutedEventArgs& args);
    // Header-row arrows and clamping; Tab leaves the table.
    bool TryHandleHeaderNavigationKey(const winrt::KeyRoutedEventArgs& args);
    bool TryHandleHeaderVerticalKey(const winrt::KeyRoutedEventArgs& args);
    // Moves focus to a visible header by visible-column index; step 0 means exact only.
    int32_t FocusVisibleHeaderFrom(int32_t visibleIndex, int32_t step);
    // Visible-column index of the header that currently has focus, or -1 when focus is elsewhere.
    int32_t GetFocusedVisibleHeaderIndex() const;
    // Header entry resolves to the remembered column, or the nearest focusable header around it.
    int32_t ResolveHeaderEntryIndex(const std::vector<winrt::FrameworkElement>& cells);
    // The table is one tab stop: Tab/Shift+Tab entry from outside lands on the header row, else the
    // body; focus aimed at a non-actionable header is redirected or cancelled.
    void OnTableGettingFocus(
        const winrt::IInspectable& sender,
        const winrt::Microsoft::UI::Xaml::Input::GettingFocusEventArgs& args);
    bool TryRedirectTableEntry(const winrt::Microsoft::UI::Xaml::Input::GettingFocusEventArgs& args);
    bool TrySetBodyEntryTarget(const winrt::Microsoft::UI::Xaml::Input::GettingFocusEventArgs& args);
    FocusEntryTarget ResolveBodyEntryTarget(bool last);
    bool FocusBodyEntryTarget(bool select, winrt::FocusState focusState = winrt::FocusState::Keyboard);
    void QueueFocusTableEntry(winrt::DependencyObject const& expected, bool bodyOnly);
    void MoveFocusOffCollapsingHeaders();
    winrt::FocusManager::GettingFocus_revoker m_headerCollapseFocusRevoker{};
    winrt::FrameworkElement FindOwnHeaderCell(winrt::DependencyObject const& element) const;
    int32_t FindActionableHeaderIndex(const std::vector<winrt::FrameworkElement>& cells, int32_t visibleIndex, int32_t step);
    winrt::UIElement::GettingFocus_revoker m_tableGettingFocusRevoker{};
    void OnHeaderHostGotFocus(
        const winrt::IInspectable& sender,
        const winrt::RoutedEventArgs& args);
    winrt::UIElement::GotFocus_revoker m_headerHostGotFocusRevoker{};
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

    // Redirects a header's bring-into-view onto the body scroller, so the header cannot scroll
    // independently of the columns it labels.
    void OnHeaderBringIntoViewRequested(const winrt::BringIntoViewRequestedEventArgs& args);
    void OnBodyBringIntoViewRequested(const winrt::BringIntoViewRequestedEventArgs& args);

    // Pre-key focus snapshots are needed because XAML directional navigation can move focus before
    // bubbling handlers run.
    winrt::KeyEventHandler m_previewKeyDownHandler{ nullptr };

    int32_t m_navAnchorRow{ -1 };
    int32_t m_navAnchorCellRow{ -1 };
    int32_t m_navAnchorCellColumn{ -1 };
    int32_t m_navAnchorHeaderColumn{ -1 };
    // Shared body/header visible-column cursor; stored as an index so it survives row recycling.
    int32_t m_currentCellColumn{ 0 };
    int32_t m_currentCellRow{ -1 };

    // Distinguishes column 0 from "no column chosen yet" so first header entry does not relocate.
    bool m_columnCursorEstablished{ false };
    void SetColumnCursorInternal(int32_t visibleColumnIndex);
    void ResetColumnCursorInternal();

    // Two-level ARIA treegrid body cursor: row level or cell level.
    bool m_cellCursorActive{ false };
    // Up/Down at cell level keep this column across rows where it lands on a neighbor or the row.
    int32_t m_cellIntentColumn{ -1 };
    int32_t m_cellIntentRow{ -1 };
    // Horizontal offset before row-level Right drilled in, so Left can undo it in one step.
    double m_drillInOffset{ -1.0 };
    int32_t m_drillInRow{ -1 };
    // The offset our own reveals left after the drill-in; any other horizontal scroll (a touchpad pan,
    // app code) moves the body off it and expires the restore.
    double m_drillSettledOffset{ -1.0 };
    bool m_drillRevealPending{ false };
    // The popped-out row's own focus bring-into-view arrives later and would undo the restore.
    winrt::weak_ref<winrt::UIElement> m_drillRestoreRow{};
    double m_drillRestoreOffset{ -1.0 };
    // Selection changes counted, so entry can tell a selection made after the remembered row.
    uint32_t m_selectionSequence{ 0 };
    uint32_t m_rememberedSelectionSequence{ 0 };
    winrt::weak_ref<winrt::DependencyObject> m_lastFocusedWithin{ nullptr };
    // True while Enter has moved focus into hosted cell content; Escape restores grid navigation
    // after hosted controls, such as ComboBox, handle their own Escape.
    bool m_cellInteractionActive{ false };
    // Weak: cells are recycled on every scroll.
    winrt::weak_ref<winrt::FrameworkElement> m_cellInteractionCell{ nullptr };
    // The row currently drilled in, so the cursor can pop it back to row level when it moves on.
    // Weak: rows are recycled on every scroll.
    winrt::weak_ref<winrt::TableViewRow> m_cellLevelRow{ nullptr };

    winrt::guid m_focusEntryRedirectCorrelationId{};
    bool m_hasFocusEntryRedirect{ false };

    // ListView paging: the viewport edge row first, then one page past it.
    int32_t GetPageTargetRow(int32_t currentRow, bool pageDown, int32_t rowCount);
    // Focus is exactly on one of this table's rows, group headers or cells, not on hosted content.
    bool IsFocusExactlyOnBodyItem() const;

    // Row-level Left/Right drill between row and cells; group headers use Left/Right for expand/collapse.
    bool TryHandleRowLevelDrillKey(const winrt::KeyRoutedEventArgs& args);
    bool ScrollBodyHorizontallyInternal(bool forward, bool toEdge = false, winrt::UIElement const& keepInView = nullptr);
    bool TryHandleGroupHeaderExpandCollapseKey(const winrt::KeyRoutedEventArgs& args);
    // Enter from a focused cell moves into interactive display content; Escape moves back out.
    bool TryHandleCellInteractionEnterKey(const winrt::KeyRoutedEventArgs& args);
    bool TryHandleCellInteractionEscapeKey(const winrt::KeyRoutedEventArgs& args);
    // Swallows navigation keys the hosted control did not want, so XAML's directional navigation
    // cannot wander out of the cell while interaction mode is active.
    bool TryHandleCellInteractionNavigationKey(const winrt::KeyRoutedEventArgs& args);
    // Focus helpers and pre-key anchors for the row/group-header levels.
    bool FocusRowContainerInternal(
        winrt::UIElement const& element,
        winrt::FocusState focusState = winrt::FocusState::Keyboard);
    bool FocusRowElementInternal(int32_t rowIndex, int32_t targetColumn, bool cellLevel, std::optional<double> alignRatio);
    int32_t GetFocusedRowContainerIndex() const;
    int32_t GetFocusedGroupHeaderIndex() const;
    bool TryMoveFocusToHeaderBandFromBody(int32_t visibleColumnIndex);
    int32_t m_navAnchorRowContainer{ -1 };
    int32_t m_navAnchorGroupHeader{ -1 };

    void OnPreviewKeyDownForNavigation(
        const winrt::IInspectable& sender,
        const winrt::KeyRoutedEventArgs& args);

    bool TryHandleCellNavigationKey(const winrt::KeyRoutedEventArgs& args);

    // Moves the cell cursor from a pre-key anchor; consumes handled cell-navigation keys, including
    // boundaries. Pass -1, -1 when no routed event is in flight.
    bool TryMoveCellCursorFromAnchor(
        winrt::Windows::System::VirtualKey key, bool isControlDown,
        int32_t anchorRow, int32_t anchorColumn);

    static constexpr int32_t c_lastColumnSentinel{ 0x7ffffffe };

    winrt::TableViewRow GetRealizedRowAt(int32_t rowIndex) const;

    int32_t GetFocusedRowIndex() const;
    int32_t GetEstimatedRowsPerPage(); // Non-const — GetDensityRowMinHeight() mutates the resource cache.

    // --- Column reorder (TableView_ColumnReorder.cpp) ---
public:
    void OnCanUserReorderColumnsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    // A neighbouring position is allowed, so the header is actionable.
    bool CanUserMoveColumnInternal(const winrt::TableViewColumn& column);
    void CancelColumnReorderDrag();
    void QueueCancelColumnReorderDrag();
    // For TableViewTestHooks: a press on the column's header and its manipulation, without a pointer.
    bool BeginColumnReorderDragForTesting(const winrt::TableViewColumn& column);
    bool UpdateColumnReorderDragForTesting(double translationX) { return UpdateColumnReorderDrag(translationX); }
    bool CompleteColumnReorderDragForTesting(double translationX);
    bool IsColumnReorderDraggingForTesting() const noexcept { return m_columnReorder && m_columnReorder->dragging; }

private:
    // The visible owned columns other than the moving one; own is its slot among them, [min, max] the allowed slots.
    struct ColumnReorderSlots
    {
        std::vector<winrt::TableViewColumn> others;
        int32_t own{ -1 };
        int32_t min{ 0 };
        int32_t max{ 0 };
    };

    // A header press that becomes a reorder once its manipulation starts. Header-row coordinates.
    struct ColumnReorderDragState
    {
        winrt::weak_ref<winrt::TableViewColumn> column{ nullptr };
        winrt::Pointer pointer{ nullptr };
        double pressX{ 0.0 };
        double pointerX{ 0.0 };
        bool dragging{ false };
        // The header band scrolled; the drag ends without a drop.
        bool stale{ false };
        double headerOffset{ 0.0 };
        // The source header's grab offset, left edge and width.
        double grabOffset{ 0.0 };
        double sourceLeft{ 0.0 };
        double sourceWidth{ 0.0 };
        double bandEdge{ -std::numeric_limits<double>::infinity() };
        // The frozen band's width and the body offset the drag measured.
        double frozenWidth{ 0.0 };
        double bodyOffset{ 0.0 };
        float sourceBase{ 0.0f };
        ColumnReorderSlots slots;
        // Per slots.others: edges, cell, pre-drag Translation.X and running slide.
        std::vector<std::pair<double, double>> edges;
        std::vector<winrt::weak_ref<winrt::FrameworkElement>> cells;
        std::vector<float> bases;
        winrt::weak_ref<winrt::FrameworkElement> source{ nullptr };
        struct Slide { float from{ 0.0f }; float to{ 0.0f }; std::chrono::steady_clock::time_point start{}; };
        std::vector<Slide> slides;
    };

    void HookColumnReorderInput();
    void OnHeaderHostPointerPressedForReorder(const winrt::PointerRoutedEventArgs& args);
    winrt::TableViewColumn HeaderColumnFromSource(const winrt::IInspectable& source, winrt::FrameworkElement& headerCell);
    bool TryHandleHeaderColumnReorderKey(const winrt::KeyRoutedEventArgs& args);
    bool BeginColumnReorderDrag();
    bool UpdateColumnReorderDrag(double translationX, bool preview = true);
    void ApplyColumnReorderPreview(ColumnReorderDragState& state);
    bool ColumnReorderLayoutChanged(double frozenWidth, double bodyOffset);
    void EndColumnReorderDrag(bool commit);
    static int32_t ColumnReorderSlotAt(const ColumnReorderDragState& state);
    bool ConsumeColumnReorderTap();
    ColumnReorderSlots GetColumnReorderSlots(const winrt::TableViewColumn& column, bool fullRange);
    int32_t ColumnReorderTargetIndex(const winrt::TableViewColumn& column, const ColumnReorderSlots& slots, int32_t slot);
    bool IsColumnMoveAllowed(uint32_t from, uint32_t to);
    bool TryReorderColumn(const winrt::TableViewColumn& column, int32_t slot);

    std::optional<ColumnReorderDragState> m_columnReorder;
    DragEscapeHook m_columnReorderEscape;
    winrt::weak_ref<winrt::TableViewColumn> m_movingColumn{ nullptr };
    bool m_suppressHeaderTap{ false };
    bool m_bodyManipulating{ false };
    uint32_t m_columnReorderGeneration{ 0 };
    uint32_t m_rebuildHeadersQueueGeneration{ 0 };
    winrt::weak_ref<winrt::UIElement> m_columnReorderHookedHost{ nullptr };
    std::vector<std::pair<winrt::RoutedEvent, winrt::IInspectable>> m_columnReorderHandlers;
    // The host's own ManipulationMode local value, restored on unhook.
    winrt::IInspectable m_columnReorderSavedManipulationMode{ nullptr };
};
