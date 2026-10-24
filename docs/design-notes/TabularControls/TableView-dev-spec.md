<!-- The purpose of this document is to describe the design and implementation of a new WinUI control.
     This document contains architectural and implementation details that do not appear in the functional spec. -->

# Description

`TableView` is a virtualized tabular control that ships in `Microsoft.UI.Xaml.Controls.Tabular.dll`. It renders an `ItemsSource` as rows and generates one cell per realized row and column from `Columns`. It is read-only by default with opt-in cell editing.

It provides column width sizing, row virtualization, leading frozen columns, single-row selection, single-column sort, filtering, single-level grouping, hierarchical rows, column resize, cell-level keyboard navigation and a UIA grid/table model. Column reorder is not yet implemented.

# Design considerations

`TableView` is a flat table renderer, not a full data grid. It targets many rows and a small-to-medium column count.

**WPF migration.** The API keeps WPF `DataGrid` concepts where they help:

- `TableViewTextColumn.Binding` is a plain CLR property, like `DataGridBoundColumn.Binding`.
- `TableViewHeadersVisibility` has only `None` and `Column`; row headers are out of scope.
- `TableViewGridLinesVisibility` keeps WPF's names and values, so persisted casts round-trip.
- `Width` is a `GridLength`, so XAML can express `Pixel`, `Auto` and `*`.

**Frozen columns** are a contiguous leading prefix. `TableViewFrozenEdge.Trailing` is reserved.

**Groups are a separate container kind.** A group header is a `TableViewGroupHeader` interleaved into the flat row list, not a nesting level, so `TableViewRow` never needs an "am I a header?" branch. Hierarchical rows also stay a flat list: each row carries its level and expansion state, and its lead cell is indented.

**`TableViewRow` is public and unsealed** although its properties are read-only. It is the realized row container (like `ListViewItem`), and the public row and cell automation peer constructors take it.

**Two built-in column types.** `TableViewTextColumn` is the efficient common case (one `TextBlock` per cell); `TableViewTemplateColumn` hosts any visual through `CellTemplate`. Without a `CellTemplate` it renders an empty cell, not `ToString()`, as in WPF, so type names never leak into cells. Zero-configuration display belongs to a future `AutoGenerateColumns`.

# Functional spec link

- [Functional spec](TableView-functional-spec.md) and [API spec](../../api-specs/TableView/TableView-spec.md)
- Hierarchy: [API spec](Hierarchical-Data-Rows-API-Spec.md), [design](Hierarchical-Data-Rows-Parent-Key-Design.md) and [test plan](Hierarchical-Rows-and-Live-Shaping-Test-Plan.md)

# Architectural overview

`TableView` owns columns, row realization, the header band, scroll sync, frozen columns, sort, selection and edit state, keyboard navigation and the UIA root. Data shaping lives in a separate stack:

```text
TableView                    realization, layout, input, UIA
 └─ TableViewSource          public shaping verbs
     └─ ShapedItemsSource    live projection, row identity, live shaping
         ├─ GroupedSourceAdapter / HierarchicalSourceAdapter   flatten groups or trees into rows
         └─ TabularShaping   ShapingPipeline, RowExpansionModel, ParentKeyIndex, SortDirection
```

Dependencies point down only; the shaping engine never learns what a `TableView` is. An `ItemsSource` that is not a `TableViewSource` is wrapped in one internally, so sorting works over any supported collection.

## Key public components

| Type | Kind | Role |
|---|---|---|
| `TableView` | unsealed `Control` | The table. Content property is `Columns`. |
| `TableViewColumn` | unsealed `DependencyObject` | Width, header, sort, freeze, read-only, cell factory (`GenerateElementCore`). |
| `TableViewTextColumn`, `TableViewTemplateColumn` | unsealed columns | Text cells and templated cells. |
| `TableViewRow` | unsealed `Control` | Realized data row. `IsSelected`, `Level`, `IsExpandable`, `IsExpanded` are read-only. |
| `TableViewGroupHeader` | unsealed `ContentControl` | Realized group band. Raises `ToggleRequested`. |
| `TableViewColumnHeader` | sealed `ContentControl` | One per column, created only by `TableView`. |
| `TableViewCellsPanel` | unsealed `Panel` | Hosts header and row cells and keeps columns aligned. |
| `TableViewSource` | sealed | `Filter`, `Sort`, `GroupBy`, `ParentBy`, live shaping. |
| `TableViewGroupInfo` | `INotifyPropertyChanged` | Group-header content. |
| Five automation peers | unsealed | See [Accessibility](#accessibility). |

The full surface, including event args and enums, is in `TableView.idl`, `TableViewSource.idl` and `TabularShaping/SortDirection.idl`, and in the [API spec](../../api-specs/TableView/TableView-spec.md).

## Key internal components

| Component | Role |
|---|---|
| `TableViewCell` | Cell wrapper: a single-child `Grid` (`Border` is sealed) whose `Tag` is the column. The cell's keyboard and UIA focus target. |
| `TableViewRowTemplateSelector` | `[MUX_INTERNAL]`. Picks the row or group-header template, so `ItemsRepeater` keeps one recycle pool per kind. |
| `TableViewRowInfo`, `RowMetadataProvider` | Row kind, level and position metadata, and the row and group identity contract. |
| `SortMemberPathResolver` | One binding-based path evaluator for both sort front-ends, so a path that displays also sorts. |
| `GridCoordinateHelper` | Row/column ↔ index math for keyboard navigation. Not projected. |
| `TableView_ContextMenu.cpp`, `TableViewToolTipHelpers.h` | Context-flyout resolution and cell tooltips. |
| `ResizeGripper` | Framework-internal drag primitive (see [Column resize](#column-resize)). |
| `ShapedItemsSource` | Live projection: rows, group buckets (`ShapedGroup`), identity index (`RowIdentity`), item subscriptions (`LiveShapingTracker`). |
| `GroupedSourceAdapter`, `HierarchicalSourceAdapter` | Materialize groups or a tree into one flat row list. |
| `ShapingPipeline` | The ordered sort, filter and group verbs a caller declared. |
| `RowExpansionModel` | Expand/collapse intent for groups and tree nodes. |
| `ParentKeyIndex` | Validated, immutable tree over the unfiltered source for `ParentBy`. |

## Visual tree

```text
TableView
└─ Border (chrome; CornerRadius = ControlCornerRadius)
   └─ Grid: Row 0 = Auto (header band), Row 1 = * (body band)
      ├─ Border PART_HeaderRow                    header background + 1px bottom rule
      │  └─ ScrollViewer PART_HeaderScroller      no user scrolling; H offset driven from the body
      │     └─ TableViewCellsPanel PART_HeaderHost   one TableViewColumnHeader per column
      └─ ScrollViewer PART_BodyScroller           H + V, auto scrollbars
         └─ Grid PART_BodyContent
            ├─ ItemsRepeater PART_RowsRepeater    vertical StackLayout, VerticalCacheLength 2.0
            └─ ContentControl PART_EmptyStatePresenter   overlay, Collapsed unless empty

TableViewRow
└─ Grid
   ├─ Border PART_RootBorder                      row background
   │  └─ Grid
   │     ├─ ContentPresenter PART_CellForegroundPresenter   foreground for Selected* states
   │     │  └─ Grid
   │     │     ├─ TableViewCellsPanel PART_CellsHost         one TableViewCell per column
   │     │     └─ Border PART_RowExpanderGutter > FontIcon PART_RowExpanderIcon
   │     └─ Rectangle PART_SelectionIndicator     leading-edge pill, Opacity 0 unless Selected*
   └─ Border PART_GridLineBorder                  1px bottom gridline, not hit-testable
```

`RebuildHeaders` and `RebuildCells` create one header or cell per non-null column, in `Columns` order. A cell hosts the element from `TableViewColumn.GenerateElement(dataItem)`. `TableView` sets `PART_RowsRepeater.ItemTemplate` (the selector needs an owner XAML cannot supply) and owns its `ItemsSource`.

# Performance considerations

- **No column virtualization.** Each realized row renders a cell per non-null column, which suits tables of about 5 to 50 columns, not spreadsheet scale (100+).
- **Row virtualization.** Rows in the viewport plus one viewport of cache above and below (`VerticalCacheLength` 2.0) are realized. `StackLayout` handles variable row heights.
- **Grouping** rebuilds the projection on each source change, O(N).
- **Live shaping** coalesces to one reshape per dispatcher turn; each reshape re-reads every item.
- The functional spec targets at least 30 FPS scrolling at about 500 rows.

# Debugging and telemetry considerations

- **Trace keyword:** `KEYWORD_TABLEVIEW` (`0x0000000000020000`) in `Telemetry/MuxcTraceLogging.h`. Under `MUXCONTROLS_TABULAR`, Tabular uses its own provider names (`Microsoft.UI.Xaml.Controls.Tabular`, `.Perf`, `.Debug`).
- **Telemetry:** `TableViewTelemetry.cpp` writes one `TableView_SessionSummary` per loaded session (operation counts, at `Unloaded`) and a local-only `TableView_Error`.
- **RuntimeProfiler:** `ProfId_TableView`, marked in the constructor.
- **Test in production:** `TableViewTip.h`/`TableViewTipTests.h` define a setup test per template application and a test per key operation. `TableView.TestInProduction.props` picks the TiP library or a stub.

## Gesture recognizer handling

- **Begin edit** uses `PointerPressed` with click counting and `handledEventsToo`, not `DoubleTapped`: a row that takes part in selection marks the press handled, which suppresses `Tapped`/`DoubleTapped`.
- **`ResizeGripper`** reads manipulation events, so touch, pen and multi-pointer arbitration come from the gesture recognizer. `ManipulationMode` (one translate axis) is set in the constructor, so a failed style load cannot disable resize. Because the gripper moves with the edge it drags, deltas are measured in `ManipulationContainer` (default: the `XamlRoot` content), not the gripper. Only the mouse is captured, so touch can still pan an ancestor `ScrollViewer`.

# Implementation details

## Column layout and width sizing

Width resolution is a **table-level** decision, made once per layout pass in `TableView::MeasureOverride` (engine in `TableView_Layout.cpp`):

1. **Measure and cache.** Each realized `TableViewCellsPanel` measures its `Auto` cells unconstrained and caches the width per column (`MeasuredWidthForColumn`). If the column is narrower, the cell is re-measured at the column width so ellipsized content and the right gridline stay in the slot. `Pixel` and `*` cells are measured once at the resolved width, collapsed columns at zero, and an open editor at the column width without feeding `Auto`.
2. **Resolve.** `ResolveColumnWidths` takes the largest cached width per column across the header and realized rows and writes `ActualWidth`. `*` columns share the viewport left after fixed columns, with `MinWidth`/`MaxWidth` clamping and re-division (WPF's `ComputeStarColumnWidths` shape).
3. **Share.** If a width changed, `TableView` invalidates the header and row panels directly.
4. **Arrange.** Each panel places cells at `ActualWidth`. Cells have no `Width` binding.

Cells only measure and cache; they never push a width. Every panel reads the same `ActualWidth`, so columns stay aligned without cross-row coordination. The default `Width` is 120 px, `MinWidth` 20 and `MaxWidth` unbounded.

**`Auto` rows are grow-only.** The row term only grows within a data set, so scrolling never narrows a column. The header term is the current measurement and is dropped while headers are hidden, so a shorter header or turning off sorting can narrow the column.

**Data-set boundaries** reset the row term: an `ItemsSource` change or a `Reset` of `Columns` for every column, and a `CellTemplate` or `Header` change for that column. Filtering, an in-place items `Reset`, `Density`, `MinWidth`/`MaxWidth` and adding or removing a column are not boundaries.

**`*` needs a finite width.** The body is measured at infinite width, so the engine reads `ScrollViewer.ViewportWidth` and re-resolves on the body scroller's `SizeChanged`.

## Column resize

Resize is split between a primitive and the table. **The primitive owns** the gesture: hit target, cursor, visual states, `IsDragging`, a 0.5 DIP deadband, rejection of non-finite deltas, re-entrancy guards and cancel. It reports a logical delta (positive grows in reading order; only the horizontal axis mirrors under RTL). **The table owns** every policy: whether a column gets a gripper (`CanUserResizeColumns` and `CanResize`, both default `true`), its width (`TableViewResizeGripperWidth`, 8), the anchor, clamping, writing `Width`, reverting on cancel and the UIA announcement. The gripper owns no width state, because `TableViewColumn` already does.

**Protocol.** `BeginDrag()`, `DragDelta` (`Delta`, `TotalDelta`), then `EndDrag(canceled)`. `DragCompleted.Canceled` marks a torn-down gesture (Escape, lost capture, disabled, unloaded); the table then restores the authored `GridLength`, so a canceled drag never pins an `Auto` or `*` column. The drag methods are public so keyboard resize raises the same events.

**Naming.** `DragOrientation` is the axis the drag is measured along, the opposite of the Windows Community Toolkit's `SizerBase.Orientation`, hence the distinct name. `OwnerName` qualifies the peer's name ("Price, Resize gripper") and is declared rather than read from `Tag`, which a host may already use.

**Keyboard.** The column header is the keyboard target; the gripper is not a tab stop. `Alt+Left`/`Alt+Right` on a header call `TryKeyboardStep`, which owns direction, the RTL mirror, `KeyboardIncrement` (8) and the `Shift` multiplier (4×). `Alt` is the only resize modifier, as in WPF.

**Bounds.** `ResizeBoundsForColumn` applies three rules derived by measuring WPF `DataGrid`:

1. **Donor.** Only a resizable `*` column after the dragged one can give width, because only `*` columns are re-divided.
2. **Direction.** `*` columns before the divider are frozen at their rendered width (`FreezeColumnsBeforeResize`).
3. **Ownership.** A `*` column can only take from another `*` column. A `Pixel` or `Auto` column may still shrink with no donor.

| Condition | Outcome |
|---|---|
| No `*` column | Unbounded; the body scrolls |
| A donor exists | Capped at viewport minus reserved widths |
| No donor, dragged column is `*` | Pinned: no gripper |
| No donor, dragged column is `Pixel`/`Auto` | Shrink-only |

A column with `MinWidth` ≥ `MaxWidth` is also pinned. `IsColumnResizePinned` is shared by the bounds and gripper visibility. It is re-evaluated when `Width`, `MinWidth`, `MaxWidth`, `Visibility` or `CanResize` changes; size changes apply on `DragCompleted`, not mid-drag.

## Row virtualization and recycling

On recycle, `ItemsRepeater` re-points a row's `DataContext`, and cells update **reactively** through inheritance and bindings. Cells are never re-stamped, because changing a live cell during the repeater's measure re-enters layout and fails fast.

- `TableViewTextColumn` binds `TextBlock.Text` to its `Binding`.
- `TableViewTemplateColumn` binds `ContentPresenter.Content` to the cell wrapper's inherited `DataContext`, because a `ContentPresenter` sets its own `DataContext` to its `Content`.
- **Custom columns** must bind to the inherited `DataContext` and not set a local one. `dataItem` is for initial setup only; content that bakes it in (or initializes once, in `Loaded` or with `x:Bind` `OneTime`) shows the previous item after a scroll.

`RebuildCells` runs only for structural changes (columns, templates, density). A recycle refreshes only banding and frozen pinning.

## Scrolling: sticky headers and frozen leading columns

**Sticky headers.** `PART_HeaderScroller` has user scrolling disabled. `TableView` calls `ChangeView` on it from `PART_BodyScroller.ViewChanged`. The sync is one-way and skips differences under 0.5 px.

**Frozen leading columns.** Frozen cells are counter-translated against the horizontal offset through `UIElement.Translation` (enabled with `ElementCompositionPreview.SetIsTranslationEnabled`), and non-frozen cells are clipped out of the pinned band. Only the contiguous prefix from column 0 freezes. Column reorder, when implemented, must not assume this approach.

## Shaping: sort, filter, group and hierarchy

Shaping is declared on the **source**, never on the control. `TableView` renders whatever its `TableViewSource` publishes.

**Single owner.** A `TableViewSource` has one owning `TableView`. A second table bound to it is refused without an error and renders empty, and cannot sort, until the owner releases it by changing `ItemsSource` or unloading. Tables release on `Unloaded` and reclaim on `Loaded`, so page navigation resolves on its own.

**Row identity** is the item's object identity (under `ParentBy`, its node key). Selection and focus re-anchor across a reshape. With no shaping verb, a null item or one object on two rows still displays, but control sorts return `false`; with a verb in force, the same data throws `E_INVALIDARG`.

**One sort axis in force.** The two front-ends reconcile instead of stacking:

- `TableView.SortByColumn`, `ToggleSortDirection` and header clicks declare the control's axis, keyed on column identity so columns sharing a `SortMemberPath` stay separate. The column's read-only `SortDirection` draws the chevron.
- `TableViewSource.Sort(path, …)` lights the column whose sort path matches. `Sort(keySelector, …)` is keyed on the delegate's COM identity and shows no chevron.
- Sorting through the control clears app-declared axes, and a source sort clears the control's axis. Otherwise an older axis could outrank a newer one while the chevron showed the loser.
- With several source axes, the first declared is primary, as in WPF `SortDescriptions`.

A sort declared before `GroupBy` orders the groups; one declared after orders rows within each group. Filtering runs first, and an emptied group disappears.

**Group identity fails fast.** Each group key needs a stable, non-empty string identity. Boxed scalars (`IPropertyValue`) have a built-in one; other keys need `GroupBy(keySelector, groupIdentitySelector)`. A null, empty or throwing identity, or two keys colliding without a selector, throws `E_INVALIDARG` when the projection is built. Row identity degrades instead, deliberately: it comes from app data the app may not control, while a bad group identity comes from a selector the app just wrote, and a silently ungrouped table gives no diagnostic.

**Expansion is intent keyed by identity.** `RowExpansionModel` stores only exceptions to a default, outside the groups, so `ExpandAllGroups`/`CollapseAllGroups` are O(1), new groups take the current default, and a collapse survives a re-sort, filter or regroup. `RetainOnly` prunes identities that no longer exist.

**Toggle.** The whole group band is the toggle target. Re-publishing destroys the realized header, so `TableView` queues the expansion by identity with a generation stamp, and restores focus by identity so a collapse does not drop focus to the top. A header's `Content` is a `TableViewGroupInfo` updated in place.

**Hierarchy.** `ParentBy` builds a `ParentKeyIndex`. Rows start collapsed, sort applies per sibling set, a filter keeps matches plus ancestors, and `GroupBy` buckets the roots. See the [hierarchy design](Hierarchical-Data-Rows-Parent-Key-Design.md).

**Live shaping** (`IsLiveShaping`) reshapes when an item's sort key, group key, filter result or parent changes, and holds reshapes while a cell is edited. Off-thread `PropertyChanged` is marshalled to the UI thread; collection changes must be raised on it.

## Selection

Selection is single, row-scoped, independent of editing, and follows keyboard focus. Group headers share the row index space but are not selectable: `Select` on a header index is rejected and selection-follows-focus skips headers. Under grouping, `SelectedIndex` therefore counts headers and is not an `ItemsSource` index. Future `Multiple`/`Extended` modes will append enum values and a separate `MultiSelectStates` group, so existing templates stay valid.

## Editing

Editing is cell-scoped and starts only from user gestures (double-click, or `F2`). `TableView.IsReadOnly` defaults to `true`. The public surface and the deferred features (`BeginEdit`, `RowEditEnding`, `ITableViewEditableItem`, public editing hooks) are in the API spec's [Cell editing scope](../../api-specs/TableView/TableView-spec.md#cell-editing-scope).

`TableView_Editing.cpp` owns the state machine, reachable entirely through the programmatic API; `TableView_EditingInput.cpp` maps gestures onto it. Gesture routing can change without touching the state machine, which is testable without input. `EditState` is `None → Beginning → {None, Editing}` and `Editing → Ending → {Editing, None}`; `Ending → Editing` is the veto path.

**Gesture layer.** A primary press on a cell makes it the current cell and focuses it, so keyboard, UIA and current cell agree. Selection applies on release over the pressed row. A right press never selects.

**Value transfer.** Editing bindings use `UpdateSourceTrigger=Explicit`; commit calls `UpdateSource()`, validates after the write and undoes a blocked write. A forced close (source change, or read-only) commits a valid value while the item is still in the source and passes the filter, else reports `Cancel` and writes nothing, as WPF does. A rollback inside layout is posted, and skipped if the value changed meanwhile.

**Re-entrancy rules.** Recycling, rebuilds and property callbacks can run inside layout, where moving focus fails fast (`0xc0000420`):

- `TableViewRow::EndCellEdit` restores the display element and moves focus; it runs only on interactive close paths.
- `TableViewRow::AbandonCellEdit` never touches focus. `TableView::TerminateEditWithoutVisualRestore` uses it from `OnRowElementClearing`, `RebuildCells`, `OnIsReadOnlyPropertyChanged` and `OnItemsSourcePropertyChanged`.
- The cell restamp path keeps the edit open, because installing the editor itself triggers a re-measure.
- The focus-loss commit is posted and re-evaluated once focus settles.

## Keyboard navigation

Keyboard handling follows the WAI-ARIA treegrid model. `TableView` handles bubbling `KeyDown`, so hosted controls see keys first. The table has **two tab stops**, the header band and the body, which share one column cursor. Horizontal keys mirror under RTL.

| Where | Key | Behavior |
|---|---|---|
| Header | `Left`/`Right` | Move between visible headers, no wrap. |
| Header | `Down` / `Up` | First row / stay. |
| Header | `Enter`, `Space` | Sort. `Space` fires on key-up, so auto-repeat cannot re-sort. |
| Header | `Alt+Left`/`Alt+Right` | Resize. |
| Row | `Up`/`Down`, `PageUp`/`PageDown` | Move at the same level (row or cell); `Up` from row 0 enters the header. Paging works like `ListView`. |
| Row | `Home`/`End` | First or last row. |
| Row | `Right`/`Left` | Flat row: drill into the first cell / nothing. Tree row: expand, else drill in / collapse, else go to parent. |
| Row | `Multiply` | Expand the subtree. |
| Group header | `Right`/`Left` | Expand / collapse. |
| Cell | `Left`/`Right`, `Home`/`End` | Move within the row, no wrap; `Left` on the first cell returns to the row. |
| Cell | `Ctrl+Home`/`Ctrl+End` | First or last cell of the table. |
| Cell | `Enter` / `Escape` | Into the cell's control / back out. |
| Row, cell | `F2` | Begin edit. |
| Row, cell | `Space` / `Ctrl+Space` | Select / toggle the row. `Ctrl` with a navigation key moves without selecting. |

- **Re-entry.** `Tab`/`Shift+Tab` into the body returns to the item that last had focus, even after a sort or filter, at the same level. If it is not available, entry lands on the first or last enabled row in the viewport, never a cache row.
- **Re-shape while focused.** When a sort or filter reshapes rows while focus is in the body, focus stays at the same row index. Focus elsewhere, including on the header that triggered the sort, stays put.
- **Focus scrolling.** A keyboard-focused row scrolls fully into view. `OnBodyBringIntoViewRequested` drops XAML's 20 DIP vertical padding, so a visible row does not nudge the viewport.
- Gamepad and XY focus navigation are not supported.

## Accessibility

| Peer | Control type | Patterns |
|---|---|---|
| Table | `DataGrid` | `Grid`, `Table`, `ItemContainer`; `Selection` unless `SelectionMode` is `None`; `Scroll` from the body scroller. Bounds exclude cache rows. |
| Row | `DataItem` | `SelectionItem`; `ExpandCollapse` for tree rows; `VirtualizedItem` while virtualized. |
| Cell | `DataItem` ("cell") | `GridItem`, `TableItem`; `Value` only when editable with a text editor; `VirtualizedItem`. |
| Column header | | `Invoke` (sort) when sortable; `Transform` (resize) when resizable, with `CanResize` false while pinned. |
| Group header | `Group` | `ExpandCollapse` always (empty groups report `LeafNode`), `GridItem`. |

The column-header peer implements `ITransformProvider`, though the IDL lists only `IInvokeProvider`.

- **Visible-column model.** UIA counts only visible columns in `ColumnCount`, `GetItem`, header sets and column indices, unlike WPF, so a zero-width column is never an addressable cell.
- **Header context from one source.** A cell's header comes from `GetColumnHeaderItems` or its name, never both, so it is not announced twice. With headers hidden, the name becomes "{header}, {value}". An empty cell is "blank".
- **Header names** come from a string `Header` or the realized content, never the gridline, glyph or gripper.
- **Unknown values.** `PositionInSet`, `SizeOfSet` and `Level` report `0` when unknown, and app-set `AutomationProperties` win. The group-header peer computes no `Level`.
- **Stale items.** A `GridItem` whose row, item or column is gone throws `UIA_E_ELEMENTNOTAVAILABLE` rather than reporting a wrong coordinate.

Strings come from the `Microsoft.UI.Xaml.Controls.Tabular/Resources` PRI subtree; a missing resource falls back safely.

## Styling, theming and template contracts

| v1 knob | Effect |
|---|---|
| `RowBackground`, `AlternatingRowBackground` | Opt-in banding; both null means unbanded. |
| `GridLinesVisibility` | Body gridlines. Header lines are always drawn. |
| `Density` | Row minimum height (`TableViewRowMinHeight` 30/40/48) and padding. |
| `ColumnHeaderStyle`, `TableViewColumn.HeaderStyle` | Header `Style`; the column's wins; no `BasedOn` needed. |
| Theme keys | `TableViewCellFontSize` and `TableViewHeaderFontSize` (both 14), `TableViewHeaderFontWeight` (`SemiBold`), `TabularSurface*` brushes. |

Brushes come from `TabularSurfaces_themeresources.xaml` and re-resolve on theme changes. `TableView.xaml` does not redeclare them, which would shadow their High Contrast values.

| Key | Used for | Fluent | High Contrast |
|---|---|---|---|
| `TabularSurfaceGridLineBrush` | Body gridlines | `DividerStrokeColorDefault` | `SystemColorWindowTextColor` |
| `TabularSurfaceHeaderSeparatorBrush` | Header lines | `DividerStrokeColorDefault` | `SystemColorWindowTextColor` |
| `TabularSurfaceOuterBorderBrush` | Control border | `CardStrokeColorDefault` | `SystemColorWindowTextColor` |

A sortable header's `CommonStates` change only the fill, so an app foreground survives hover; in High Contrast, `HighContrastStates` also swap the foreground. Row `CornerRadius` feeds only the focus visual, keeping the row stack rectangular. Cells and the header `PART_Root` are `Grid`s, which clip when rounded, so `AttachFocusCornerRadius` rounds them only while the keyboard focus rect shows.

**Later:** per-element `RowStyle` (on `TableViewRow`) and `CellStyle` (on the `TableViewCell` wrapper), as in WPF, rather than a monolithic `TableStyle`, so standard `Style` machinery applies without a property explosion.

**Template contracts.** MIDL3 does not surface `[TemplatePart]`. "Resolved" parts are looked up in code. "State" parts are targeted by the default visual states, so a re-template that removes one must remove its setters.

| Template | Parts |
|---|---|
| `TableView` | Resolved: `PART_HeaderHost` (`Panel`, required), `PART_RowsRepeater` (`ItemsRepeater`, required, do not bind `ItemsSource`), `PART_HeaderRow` (collapsed when headers hide), `PART_EmptyStatePresenter` (`ContentControl`, optional). |
| `TableViewRow` | Resolved: `PART_CellsHost` (`Panel`, required), `PART_GridLineBorder` and `PART_RowExpanderGutter` (optional). State: `PART_RootBorder`, `PART_CellForegroundPresenter`, `PART_SelectionIndicator`, `PART_RowExpanderIcon`. |
| `TableViewColumnHeader` | Resolved, optional: `PART_ContentPresenter`, `PART_GridLine`, `PART_SortIndicatorHost`, `PART_ResizeGripperHost`. State: `PART_Root`. |
| `TableViewGroupHeader` | Resolved, optional: `PART_GridLineBorder`. State: `PART_Root`, `PART_ExpanderGutter`, `PART_ExpanderIcon`, `PART_ContentPresenter`. |

The scrollers are found as the nearest `ScrollViewer` ancestors of the header host and repeater, not by name. Row states are `CommonStates` (including four `Selected*` states), `RowExpansionStates`, `RowExpandabilityStates` and `RowHierarchyStates`, `Row`-prefixed so they cannot collide with an app's `Expanded`/`Collapsed`.

## Lifetime, threading and re-entrancy

- Columns and rows hold weak back-pointers to their `TableView`; cross-object events use `auto_revoke`.
- `Unloaded` clears `PART_RowsRepeater.ItemsSource` so queued repeater work cannot run on a detached subtree; `Loaded` restores it and the selection.
- `ThemeSettings` is released on `Unloaded`, or earlier on `AppWindow.Destroying`, because removing its handler after the window is destroyed fails fast. Hosts with no `AppWindow`, or one on another thread, use `Unloaded`.
- `TableView` and `TableViewSource` are UI-thread-affine. Editing re-entrancy rules are in [Editing](#editing).

## Build, namespace and ABI notes

Code comments link here; keep the reasons intact.

- **`SortDirection` namespace.** `TabularShaping/SortDirection.idl` declares it into `Microsoft.UI.Xaml.Controls.Tabular`. It must not go into `Microsoft.UI.Xaml.Data`, which the Windows App SDK also owns: a C# consumer of both winmds binds that namespace to the SDK's copy and fails with `CS0234`.
- **Tabular metadata IDL.** `Microsoft.UI.Xaml.Controls.Tabular.idl` is a renamed copy of MUXC's metadata IDL that produces `XamlControlsTabularXamlMetaDataProvider` and `TabularControlsResources`, avoiding duplicate runtime classes. It does not redeclare `Microsoft.UI.Xaml.CustomAttributes`, which come from MUXC's winmd (see `Tabular.ProjectImports.targets`), and must stay the first `@(Midl)` entry so its `MUX_*` macros reach every later IDL.
- **Private namespace.** `MU_PRIVATE_CONTROLS_NAMESPACE` equals MUXC's value (`Microsoft.UI.Private.Controls`); a Tabular child namespace breaks cppwinrt, which includes a parent that is never generated. It is safe because this build never references MUXC's private winmd. That namespace, not `[MUX_INTERNAL]`, keeps `ResizeGripper` and its peer out of shipped winmds.
- **Frozen overloads.** `[default_overload]`/`[method_name]` on `TableViewSource.GroupBy` (`GroupBy`, `GroupByWithIdentity`) and `Sort` (`SortByPath`, `Sort`) are frozen ABI; without them MIDL emits `GroupBy2`/`Sort2`.
- **`TableViewSource` is sealed** because `TableView` drives the concrete type, so an override would be bypassed. Unsealing later is compatible; sealing later is not.
- **`TableView` is unsealed** for `ReferenceTracker`. `TableViewColumn` has a public constructor so apps can derive columns.
- **`TableViewRowTemplateSelector`** is projected only because `ItemTemplateWrapper` is in another DLL. It is `[MUX_INTERNAL]`, and unsealed with a constructor because MIDL requires that for a member-less class.

# Appendix

## API notes not yet in the API spec

These contracts are documented only here; they belong in the [API spec](../../api-specs/TableView/TableView-spec.md).

- **Sort methods.** `SortByColumn(column, None)` clears, even when `CanSort` is `false`. `ToggleSortDirection` walks the column's `SortCycle`. Both return `false`, changing nothing, when `Sorting` is canceled, an edit will not close, the column cannot sort (`CanSort=False`, or no sort path and no `CustomSortComparer`), rows lack identity, or the source is refused. `CanUserSortColumns=False` gates only header clicks. `TableView.ClearSort()` returns `false` when only an app delegate axis is active; use `TableViewSource.ClearSort()`.
- **Sort events.** `Sorting` is raised first and is cancellable, letting the app own the order and light the chevron with `TableViewSource.Sort(path, …)`. `Sorted` follows. A null `Column` means clear-all. Setting `CanSort=False` on the sorted column clears it with both events.
- **`SortCycle`** is per column. Its first direction is the opening one; the `*None` variants add a step back to source order.
- **`SortMemberPath`.** When empty, `TableViewTextColumn` uses `Binding.Path.Path` if the binding targets the row item (no `Source`, `RelativeSource` or `ElementName`), through the overridable `GetSortMemberPathCore()`. `CustomSortComparer` wins over the path and must be a pure function.
- **`TableViewSource`.** `From` accepts `IVector<Object>`, `IBindableVector`, `IIterable<Object>` or `IBindableIterable`. Null items, predicate or key selector throw `E_INVALIDARG`; `groupIdentitySelector` may be null. Use `ClearFilter()` to remove a filter. `ParentBy` errors are in the [hierarchy API spec](Hierarchical-Data-Rows-API-Spec.md).
- **Context flyouts.** First match wins. Cell: a `ContextFlyout` in the cell content, `CellContextFlyout`, the row's `ContextFlyout`, `TableView.ContextFlyout`. Header: a `ContextFlyout` in the header content, `HeaderContextFlyout`, `TableView.ContextFlyout`. Opening one focuses its target and does not change selection.
- **`TableViewRow.Level`** is 1-based for tree rows and `0` otherwise, matching `AutomationProperties.Level`.

## Known limitations

- **`*` needs a bounded width.** In a width-to-content host, `*` columns keep their provisional width.
- **Fixed columns wider than the viewport** collapse `*` columns to `MinWidth`, as in WPF.
- **Width-dependent height.** An `Auto` cell that fits its column is not re-measured at the final width.
- **Resize differs from WPF** next to `Pixel`/`Auto` neighbors: WPF assigns the neighbor a width, while `TableView` only redistributes `*` space, so those layouts pin rather than overflow.
- **Grouping** is single-level, with no per-group expand or collapse by key.

## Non-goals and deferred work

- **Not yet implemented:** column reorder, trailing frozen columns, multi-select, `RowStyle`/`CellStyle`, programmatic `BeginEdit`, row-scoped editing.
- **Deferred:** `Auto` shrink, multi-column sort, clipboard, incremental loading, editing announcements.
- **Non-goals:** column virtualization, row headers, marquee selection, spreadsheet-style interaction.
