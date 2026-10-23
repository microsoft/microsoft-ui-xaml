# Description

`TableView` is the minimal, display-only tabular base control for `Microsoft.UI.Xaml.Controls.Tabular.dll`. It renders an `ItemsSource` as virtualized rows and uses `Columns` to generate one cell per realized row/column intersection.

This is the minimal, display-only base of the control, and it is read-only. It establishes column ownership, header/body layout, row virtualization, cell generation, leading frozen columns, basic row keyboard navigation, automation peers, density/theming hooks, and the lifetime patterns later work builds on. The interactive v1 features land additively on top of it: selection, filtering, single-column sort, single-level grouping and hierarchical rows are delivered; column resize/reorder is still outstanding.

Feature-level requirements and the v1-vs-deferred breakdown are in [`TableView-functional-spec.md`](./TableView-functional-spec.md).

Theme-XBF emission is intentionally suppressed in this PR. The control compiles into `Microsoft.UI.Xaml.Controls.Tabular.dll`, but the binary does not emit its own theme XBF yet.

# Design considerations

`TableView` is a flat table renderer, not a full data-grid. It focuses on predictable display of many rows with a small-to-medium column count.

The API preserves migration-compatible concepts where useful:

- `TableViewTextColumn.Binding` is a plain CLR property, matching WPF `DataGridBoundColumn`.
- `TableViewHeadersVisibility` is a flags enum with `None` and `Column` only; row headers are out of scope, so `Row`/`All` (WPF values 2/3) are intentionally not defined.
- `TableViewGridLinesVisibility` preserves WPF `DataGridGridLinesVisibility` values.

`Width` is typed as `GridLength` so XAML can express `Pixel`/`Auto`/`*` intent. All three resolve in v1: `Pixel` is exact, `Auto` sizes to the widest realized cell (it grows within a data set and never shrinks — true auto-shrink is deferred), and `*` takes a proportional share of the body viewport left after the fixed columns. Resolution is owned by the table (see the *Column width sizing* section below); `MinWidth`/`MaxWidth` clamp every mode.

Frozen columns are limited to a contiguous leading prefix in v1. `TableViewFrozenEdge.Trailing` is reserved.

Single-level grouping is delivered (see *Grouping model* below): group headers are their own container kind interleaved into the flat row projection, not a second nesting level in the layout. Hierarchical rows are delivered through `TableViewSource.ParentBy(keySelector, parentKeySelector)` over a flat source, with no depth limit: each visible row carries its level and expansion state, and the row's lead cell is indented with an expander gutter, so the layout stays a flat virtualized list. See [Hierarchical data rows design](Hierarchical-Data-Rows-Parent-Key-Design.md).

`TableViewRow` is intentionally a public `unsealed` `Control` despite carrying no v1 dependency properties: it is the realized row container (mirroring `ListViewItem`/`DataGridRow`) and is referenced by the public `TableViewRowAutomationPeer` and `TableViewCellAutomationPeer` constructors, so it cannot be internalized without collapsing the public accessibility surface. Row-level features layer onto it: selection and hierarchy already do. Grouping deliberately does **not** — a group header is a `TableViewGroupHeader`, a separate container kind chosen by `TableViewRowTemplateSelector`, so `TableViewRow` never grows an "am I a header?" adaptive branch.


## Column model: `TextColumn` vs `TemplateColumn`

`TableView` ships two built-in column types rather than one, mirroring WPF
(`DataGridTextColumn` / `DataGridBoundColumn` vs `DataGridTemplateColumn`):

- `TableViewTextColumn` is the built-in **property-bound text** path: it generates a bare
  `TextBlock`, applies its CLR `Binding` to `Text`, and carries the default table cell styling
  (font, vertical centering, density-aware padding, character-ellipsis trimming). It is the
  efficient common case (one `TextBlock` per cell, no template inflation) and the natural home
  for the future editing story (a text cell's editing element layers onto it).
- `TableViewTemplateColumn` hosts **arbitrary consumer visuals** via `CellTemplate`, bound
  against the row item through `ContentPresenter.Content = dataItem`.

A `TableViewTemplateColumn` with **no `CellTemplate` renders an empty cell** - it deliberately
does **not** fall back to `dataItem.ToString()`. This matches WPF's `DataGridTemplateColumn` and
avoids leaking type-name strings (e.g. `MyApp.Person`) into cells when a template is forgotten.
Zero-configuration "just show my data" is intentionally **not** solved by `ToString()`; the
principled answer is column auto-generation from the item's public properties
(`AutoGenerateColumns`, a candidate follow-up feature), which produces real bound
`TableViewTextColumn`s rather than stringified objects. Consumers who explicitly want the whole
item's string form can use `TableViewTextColumn Binding="{Binding}"`.

# Layout

The default `TableView` template is a two-band layout: a chrome `Border` (rounded with `ControlCornerRadius` for a card edge) containing a `Grid` with two rows — row 0 `Auto` (header band) and row 1 `*` (body band). The corner rounding is render-only and does not shrink the body viewport, so row virtualization is unaffected.

```text
TableView — Default Template
└─ Border (chrome)
   └─ Grid ─ Row 0 = Auto (header) · Row 1 = * (body)
      ├─ Header band  [Row 0, Auto]
      │  └─ Border PART_HeaderRow                 (header background + 1px bottom gridline)
      │     └─ ScrollViewer PART_HeaderScroller   (H-only; scrollbar hidden; V disabled; synced to body)
      │        └─ TableViewCellsPanel PART_HeaderHost   -> one TableViewColumnHeader per column (L->R)
      └─ Body band  [Row 1, *]
         └─ ScrollViewer PART_BodyScroller         (H + V, auto scrollbars)
            └─ Grid PART_BodyContent
               ├─ ItemsRepeater PART_RowsRepeater  (vertical StackLayout; ItemTemplate = TableViewRow)
               └─ ContentControl PART_EmptyStatePresenter  (overlay; Collapsed unless data empty)

TableViewRow — ItemTemplate   (realized / recycled by PART_RowsRepeater)
└─ Border PART_RootBorder   (MinHeight 40; 1px bottom gridline; CommonStates: Normal/PointerOver/Pressed/Disabled)
   └─ TableViewCellsPanel PART_CellsHost   -> one cell-wrapper Border per column (hosts GenerateElement, L->R)
```

Each `TableViewCellsPanel` measures its cells unconstrained and arranges them at the owning column's `ActualWidth` (no `Width` binding) — see *Column alignment* below.

*Structural view of the visual tree. Leading frozen-column counter-translation/clip and the intentional omission of column virtualization are runtime behaviors, covered in the Frozen leading columns and Row virtualization sections below.*

## Header band

`Border` `PART_HeaderRow` → `ScrollViewer` `PART_HeaderScroller` → `TableViewCellsPanel` `PART_HeaderHost`.

`PART_HeaderRow` provides the header-surface background and a 1px bottom rule. `PART_HeaderScroller` is horizontal-only (`HorizontalScrollMode="Auto"`, horizontal scrollbar hidden, vertical disabled). `PART_HeaderHost` is a `TableViewCellsPanel` (inherently horizontal); `RebuildHeaders` fills it with one `TableViewColumnHeader` per non-null column (each cell mirrors the column's `Visibility`), in `Columns` order, left-to-right. Header cells use the density row minimum height, so the header band matches body rows. Like rows, header cells carry `ControlCornerRadius`, consumed only by the system focus visual, so header keyboard focus is rounded. Header content comes from `TableViewColumn.Header`, displayed via `HeaderTemplateSelector` when set, otherwise `HeaderTemplate`, otherwise the default text presenter. Header lines are always drawn, independently of the body's `GridLinesVisibility`: each header's full-height 1px `PART_GridLine` separator on its trailing edge and the 1px rule under `PART_HeaderRow`, both with `TabularSurfaceHeaderSeparatorBrush`; overriding that resource with `Transparent` hides them. `TableView::ApplyGridLinesToHeader` owns the separators; the template draws the bottom rule (`PART_HeaderRow`'s `BorderThickness="0,0,0,1"`). Like the body's vertical lines, the last visible column's separator is suppressed only where it would double the outer border. A focused header keeps focus when `RebuildHeaders` replaces the band (a header, template or theme change); hiding the focused column moves focus to the next visible header, or the previous one when it was last.

**Pointer feedback.** A sortable header (`CanUserSortColumns` and `TableViewColumn.CanSort`) reacts to the pointer like a row: on hover it paints `TabularSurfaceHeaderBackgroundPointerOverBrush`, while pressed `TabularSurfaceHeaderBackgroundPressedBrush`, and it returns to its transparent fill on exit, release, capture loss or cancel (a capture loss or cancel from the resize gripper's own drag capture is ignored, so a header still under the pointer stays hovered). In High Contrast only, the default header text presenter and sort indicator also take `TabularSurfaceHeaderForegroundPointerOverBrush` / `TabularSurfaceHeaderForegroundPressedBrush` (`SystemColorHighlightTextColor` on a `SystemColorHighlightColor` fill). In Light and Dark the header text keeps its inherited foreground, so an app-set `TableView.Foreground`, `HeaderTemplate` foreground or `SortIndicatorForeground` is not overridden on hover; there those foreground keys equal the default rest foreground (`TabularSurfaceRowForegroundBrush`), and the fills equal the row pointer-over/pressed fills. The fills and High Contrast foregrounds are `{ThemeResource}` setters in the `TableViewColumnHeader` template's `CommonStates` and `HighContrastStates`; a theme or High Contrast change rebuilds the header band, so each header restarts in `Normal`/`HighContrastNormal`. `Disabled` dims the text and sort glyph with `TabularSurfaceRowForegroundDisabledBrush`. A header that cannot sort shows no feedback, the pointer events are never marked handled (click-to-sort and focus are unchanged), and a press on the resize gripper stays with the gripper.

## Body band

`ScrollViewer` `PART_BodyScroller` → `Grid` `PART_BodyContent` → `ItemsRepeater` `PART_RowsRepeater`.

`PART_BodyScroller` enables both horizontal and vertical scrolling with auto scrollbars. `PART_RowsRepeater.Layout` is a vertical `StackLayout` (`HorizontalCacheLength`/`VerticalCacheLength` = `2.0`; `ItemTemplate` = `TableViewRow`). `PART_EmptyStatePresenter` is a `ContentControl` overlaid in the same `PART_BodyContent` cell, `Collapsed` unless the empty state is shown.

## Row

`Border` `PART_RootBorder` → `TableViewCellsPanel` `PART_CellsHost`.

`PART_RootBorder` is the row chrome: it supplies the default 40px minimum row height, a 1px bottom gridline, and the `CommonStates` visual states (`Normal`, `PointerOver`, `Pressed`, `Disabled`) that animate the row background. `PART_CellsHost` is a `TableViewCellsPanel` (inherently horizontal); `RebuildCells` fills it with one cell-wrapper `Border` per non-null column (mirroring the column's `Visibility`), in `Columns` order, left-to-right. Each wrapper hosts the element returned by `TableViewColumn.GenerateElement(dataItem)` when non-null. The row's `CornerRadius` (default `ControlCornerRadius`) is consumed only by the system focus visual (`UseSystemFocusVisuals="True"`, which rounds to the focused element's corners), so row/keyboard focus is rounded; it is intentionally **not** bound to `PART_RootBorder`, keeping the row background and 1px bottom gridline rectangular across the gapless row stack (no corner notches between adjacent rows). Group headers follow the same rule. Cells and header cells are `Grid`s, and a rounded `Grid` clips its children and its hit-test area, so they take the TableView's `CornerRadius` (`ControlCornerRadius` by default, set in XAML) only while the focus rect shows (keyboard focus, or programmatic focus after keyboard input) (`TableView::AttachFocusCornerRadius`). The focus rect is rounded, and the clip stays under it.

## Column alignment

The header band and each realized row host their cells in a `TableViewCellsPanel` — a custom `Panel` whose `MeasureOverride` measures every cell twice (first unconstrained, to learn its measured width for Auto sizing; then a second pass constrained to the column's resolved `ActualWidth`) and whose `ArrangeOverride` places each cell at that `ActualWidth` (`x` = sum of preceding visible columns' widths; a collapsed column arranges to zero width). The second, constrained measure matters: without it, a cell whose content is wider than its column keeps a `DesiredSize` larger than the column, so XAML's arrange would expand the cell's *render* size to that desired width and clip it to the column slot — which clips away the cell's right border (the vertical gridline). Constraining the measure keeps the ellipsized content and the border inside the column so the boundary gridline renders. The header band and every realized row stay aligned with no cross-row coordination because every panel reads the *same* shared `TableViewColumn.ActualWidth`. Cells carry **no** `Width` binding: an explicit `Width` would defeat the panel's unconstrained measured-width measurement, so the panel arranges to the resolved width directly.

**Layout ownership — table-level.** Column-width resolution is a single **table-level** decision, computed once per layout pass. `TableView::MeasureOverride` measures the template subtree — which realizes the viewport rows and runs each `TableViewCellsPanel::MeasureOverride`, caching every cell's measured width per column — then calls `ResolveColumnWidths` exactly once: it pulls the max measured width per column across the header host and the currently realized rows and resolves Pixel/Auto/Star. Cells and rows only *measure and cache*; they never decide or push a width, so a column's state is written once per pass rather than once per realized cell. When a resolved width changes, `TableView` directly invalidates the header + realized row panels (no cross-object event), which re-arrange to the new `ActualWidth`; a stable data set settles in one extra pass. The row's cell host is always the controlled `TableViewCellsPanel`, so a custom row template can restyle cell *content* without breaking column alignment.

## Column width sizing

Column widths are resolved by the table in a single pass owned by `TableView::MeasureOverride`; the engine lives in `TableView_Layout.cpp`. Each layout pass runs a four-step pipeline and writes each column's read-only `ActualWidth`, which each cells panel reads when it arranges:

1. **Measure + cache** — `TableView::MeasureOverride` measures the template subtree; each realized `TableViewCellsPanel` (header + rows) measures its cells unconstrained and caches the measured width per column (`MeasuredWidthForColumn`). Cells do not push to the table.
2. **Resolve (once)** — `ResolveColumnWidths` pulls the max measured width per column across the header host and the realized rows, then turns `Width`/`MinWidth`/`MaxWidth` plus the body viewport into `ActualWidth`: `Pixel` → the given pixels; `Auto` → the pulled max (grow-only, see below); `*` → a proportional share of the viewport left after the fixed columns, with min/max clamping and re-division (the WPF `ComputeStarColumnWidths` shape).
3. **Share** — the resolved `ActualWidth` on each `TableViewColumn` is read by every header/row `TableViewCellsPanel` when it arranges; if a width changed, `TableView` directly invalidates those panels so they re-arrange atomically.
4. **Arrange** — each `TableViewCellsPanel.ArrangeOverride` places its cells left-to-right at those widths (leading frozen cells are then pinned to the horizontal scroll offset).

Only realized rows contribute, so `Auto` sizes to the widest *realized* cell. `Auto` is **grow-only** within a data set: the per-column accumulator only increases and is reset only on data-set boundaries — an `ItemsSource` change, the `Columns` collection being *replaced*, a `CellTemplate` change, or a header change. The row cells keep a grow-only maximum per column, while the header term is its current measurement (headers are not virtualized), and the Auto width is `max(rows, header)` with the header term dropped while `HeadersVisibility` is `None`. Toggling the headers therefore adds or drops only the header term and never resets the rows' max, and a shorter header or turning off sorting (which removes the sort indicator's reserve) narrows the column. An in-place collection `Reset` or clear-and-refill, a `TableViewSource` filter/refilter, and a `Density` change are *not* data-set boundaries; an app that wants columns re-fit re-assigns `ItemsSource` (or sets a `Pixel` `Width`). An *incremental* `Columns` add/remove does **not** reset the pre-existing columns' accumulators (their row data is unchanged, so resetting them would let an unrelated column shrink to only the currently realized rows); the newly inserted column simply starts fresh and grows from content. `Min`/`MaxWidth` changes re-resolve without resetting. `*` needs a finite width to divide: the horizontally-scrolling body panel is measured at infinite width, so the engine pulls `ScrollViewer.ViewportWidth` explicitly and re-resolves on the body scroller's `SizeChanged`. `Auto` auto-shrink and column virtualization are out of scope for v1.

**Known layout limitations (v1).**
- **`*` requires a bounded viewport.** Star columns divide the body `ScrollViewer.ViewportWidth`. In a width-to-content host (the table given an infinite width constraint, e.g. inside an auto-sized `StackPanel`), there is no finite width to divide, so `*` columns stay at their provisional default width. Give the table a bounded width — or use `Pixel`/`Auto` columns — in width-to-content layouts.
- **Fixed columns wider than the viewport.** When the `Pixel` + `Auto` (fixed) columns already exceed the viewport, the space left for `*` is zero, so `*` columns collapse to their `MinWidth` and the body scrolls horizontally. This matches WPF `DataGrid`'s star-vs-fixed behavior.
- **Width-dependent cell height.** Cells are measured unconstrained (infinite width) to learn their measured width, then arranged at the resolved column width. A `CellTemplate` whose *height* depends on its *width* (a wrapping `TextBlock`, an aspect-fit image) is measured for its single-line/unwrapped height and is clipped when arranged narrower. The built-in `TableViewTextColumn` uses single-line `CharacterEllipsis`, so it is unaffected; authors of wrapping `TableViewTemplateColumn` content should set an explicit row height or avoid width-dependent wrapping in v1.

## Column resize (`ResizeGripper`)

Interactive column resize is a **policy split** between a framework-internal primitive and the table. `ResizeGripper` (`Microsoft.UI.Private.Controls`, shipped only in `Microsoft.UI.Xaml.Controls.Tabular.dll`) is a *delta source*: it owns the gesture and reports how far the user has dragged. It owns no value, no bounds and no layout, because `TableViewColumn` already owns `Width`/`MinWidth`/`MaxWidth`/`ActualWidth` — a second copy of that state is what the primitive deliberately avoids.

**The primitive owns** the hit target, its own presentation (visual states plus the `ResizeGripperSeparatorBrush`/`ResizeGripperSeparatorThickness` theme keys), the resize cursor, the input protocol, and the gesture state machine: `IsDragging`, a 0.5 DIP deadband, rejection of non-finite deltas, re-entrancy guards, and cancel semantics.

**The table owns** every policy decision: whether a column gets a gripper at all (`CanUserResizeColumns` + `TableViewColumn.CanResize`), the gripper's placement and width (`TableViewResizeGripperWidth`), the anchor captured at `DragStarted`, clamping the reported delta to `MinWidth`/`MaxWidth`, writing `Width`, reverting on cancel, and the UIA width announcement.

**Protocol.** `BeginDrag()` → zero or more `DragDelta` → `EndDrag(canceled)`. `DragDelta` carries `Delta` (since the last raise) and `TotalDelta` (from the start of the gesture), so a host applies `TotalDelta` against the anchor it captured rather than accumulating. `DragCompleted.Canceled` distinguishes a torn-down gesture (Escape, a canceled contact, lost mouse capture, the gripper disabled, the header rebuilt mid-drag) from a release: on cancel the host restores the **authored `GridLength`**, so a canceled drag cannot silently rewrite an `Auto` or `*` column as fixed pixels. `BeginDrag`/`TryDrag`/`EndDrag` are public so a host can drive the identical gesture from its own input handling — that is how keyboard resize stays indistinguishable from a pointer drag.

**Input.** Pointer input arrives as manipulation events, so touch and pen contacts and multi-pointer arbitration come from the framework's gesture recognizer rather than from each host. The gripper captures the mouse only, on press, so the resize cursor holds while the pointer leaves the gripper mid-drag; touch and pen stay uncaptured so an ancestor `ScrollViewer` can still pan the cross axis. `ManipulationMode` is a single translate axis and is set in the constructor, not the default style: a host whose style failed to load must not silently lose pointer resize. Because the gripper travels with the edge it drags, `ManipulationStarting` reparents the measurement frame — the default container is the element itself, whose motion would feed back into the gesture and stall it.

**Axis and RTL.** `DragOrientation` is the axis the drag is **measured along**, which for a divider is perpendicular to how it looks: `Horizontal` drags left/right. It is named `DragOrientation` rather than `Orientation` precisely because the primitive owns no shape — the host sizes it — so the control's own layout axis is not something it can describe. This is also the opposite of the Windows Community Toolkit's `SizerBase.Orientation`, which describes the bar; the distinct name prevents a silent mix-up. The primitive normalizes to a **logical** delta, so positive always grows in reading order and both input paths agree; only the horizontal axis mirrors under RTL.

**Keyboard.** The header cell, not the gripper, is the keyboard target. Column commands belong there, and one tab stop per column would sit between the user and the data. The table therefore sets `IsTabStop(false)` on the gripper and routes **`Alt+Left`/`Alt+Right`** on a focused header into `TryKeyboardStep(VirtualKey)`. That function owns direction, the RTL mirror, `KeyboardIncrement` and the Shift multiplier (`Alt+Shift+Arrow` is the large step), so the pointer and keyboard paths cannot drift apart. `Alt` is the **only** resize modifier: `Ctrl+Arrow` on a header does nothing, and bare `Left`/`Right` move focus between headers (see *Keyboard* below). The binding matches WPF `DataGrid`.

### Resize policy — what each column configuration does

The reference behavior is **WPF `DataGrid`**, and the rules below are derived from measuring it rather than from its source. `ResizeBoundsForColumn` turns them into a `[Min, Max]` the drag is clamped to.

Three rules decide everything:

1. **Donor.** A column can give width away only if it is currently `*`, is user-resizable (`CanUserResizeColumns` and `TableViewColumn.CanResize`), and sits **after** the dragged column. Only `*` qualifies because only `*` columns are re-divided by `ResolveColumnWidths`; reserving space from a `Pixel` or `Auto` neighbour would promise width that never materializes.
2. **Direction.** A resize takes space only from the columns after the dragged divider. `*` columns before it are frozen at the width they already render (`FreezeColumnsBeforeResize`), which keeps them out of both the bound and the redistribution pass without changing what the user sees.
3. **Ownership.** A `*` column can only change width by taking from another `*` column, so with no donor it cannot move at all. A `Pixel` or `Auto` column owns its width outright, so it may still **shrink** even with no donor — that only makes the table narrower and needs nothing from a neighbour.

This yields three outcomes:

| Condition | Outcome |
|---|---|
| No column authored `*` anywhere | **Unbounded** — the column grows, the extent exceeds the viewport, the body scrolls |
| `*` present, at least one donor after the dragged column | **Capped** at `viewport − Σ reserved` |
| `*` present, no donor, dragged column is `*` | **Pinned** — the divider does not move |
| `*` present, no donor, dragged column is `Pixel`/`Auto` | **Shrink-only** — may narrow to its own `MinWidth`, may not grow |

A pinned column shows no gripper. A column whose `MinWidth` is at least its `MaxWidth` can never change width, so it is pinned too, and a fixed-width `*` column is not a donor. Availability is re-evaluated when `Width`, `MinWidth`, `MaxWidth`, `Visibility` or `CanResize` changes. Size changes (`Width`, `MinWidth`, `MaxWidth`) are not re-evaluated mid-drag — the drag itself converts columns to `Pixel` — and are refreshed once on `DragCompleted`; a `Visibility` change re-evaluates immediately, even during a drag. `IsColumnResizePinned` is the one rule shared by the drag bounds and the gripper's visibility; the collapsed gripper stays in the header cell, so `Alt+Arrow` still reaches it and is clamped the same way.

What each other column reserves: a donor reserves its `MinWidth`; a `Pixel` column its pixel width; an `Auto` column its content width; a locked `*` column its share of the **authored** layout, so a neighbour's drag cannot move it.

Two-column expectations, viewport 478, `MinWidth` 50 (`!` = `CanResize="False"`):

| Config | Outcome | Config | Outcome |
|---|---|---|---|
| `P,P` `P,P!` | unbounded, scrolls | `S,S` | capped |
| `P,A` `P,A!` | unbounded, scrolls | `S,S!` | pinned |
| `A,P` `A,P!` | unbounded, scrolls | `S,P` `S,P!` | pinned |
| `A,A` `A,A!` | unbounded, scrolls | `S,A` `S,A!` | pinned |
| `P,S` `A,S` | capped | `A,S!` | shrink-only |
| `P,S!` | shrink-only | | |

**Known divergences from WPF.** WPF's resize *assigns* the neighbour an explicit width instead of relying on a redistribution pass, so it can shrink a `Pixel` or `Auto` neighbour. Ours cannot, so `S,P`, `S,A` and the growth direction of `P,S!,A` **pin** where WPF resizes. This is deliberate: promising space that a non-`*` neighbour never surrenders is what produced the original overflow bug. Matching WPF here would mean adopting its assign-the-neighbour model, which is a larger change to the drag path. All other configurations agree with WPF on whether the divider moves.

## Row virtualization

Rows are virtualized by `ItemsRepeater` on the vertical axis. With the vertical `StackLayout`, only rows whose realization rect intersects the body viewport — plus two viewports of cache on each side — are materialized as `TableViewRow` instances; off-screen rows return to the recycle pool. `StackLayout` is used instead of a uniform two-axis layout because row heights vary with density and template content.

On recycle, `ItemsRepeater` re-points a pooled row's `DataContext` to the new item and cell **data** updates **reactively** through `DataContext` inheritance + bindings — cells are **not** re-stamped imperatively (mutating a live cell during the repeater's measure pass re-enters framework layout and fails fast). `TableViewTextColumn` binds `TextBlock.Text` to the consumer `Binding` (resolved against the inherited `DataContext`); `TableViewTemplateColumn` binds its `ContentPresenter.Content` to the cell-wrapper `Border`'s inherited `DataContext` — bound to the *wrapper*, not the presenter's own `DataContext`, because `ContentPresenter` pins its `DataContext` to its `Content` (a self-referential binding would freeze after the first item). Consequently `RebuildCells` is needed only for structural changes (columns / template / density); a pure recycle refreshes only index-dependent visuals — alternating-row banding (`RefreshRowBackground`) and frozen-column pinning. Custom columns (overridable `GenerateElementCore`) must bind reactively to the inherited `DataContext` for the same reason. This holds on every recycle to a new item: unless a client has realized a cell automation peer, the row reuses its cells, so a custom column that bakes `dataItem` into its element, or template content initialized once per element (in `Loaded`, or through `x:Bind` with `OneTime`), keeps showing the previous item after a scroll.

Column virtualization is intentionally not implemented: each realized row renders a cell for every non-null column (collapsed columns get a hidden cell), which is the right trade for typical 5–50 column tables. Because every realized row materializes one cell wrapper per non-null column, `TableView` targets typical application tables (~5–50 columns) and is **not** designed for spreadsheet-scale column counts (100+). Column virtualization is independent of the width engine: `Auto`/`*` sizing ships in this PR (see *Column width sizing*) without column virtualization, which stays out of scope — each realized row still renders one cell per non-null column.

## Sticky headers

The C++ control drives `PART_HeaderScroller`'s horizontal offset from `PART_BodyScroller.ViewChanged`, so the header band tracks horizontal body scrolling. The sync is one-way (only the body's `ViewChanged` is handled; the header is never the source) and skips near-equal offsets (<0.5px) to avoid `ViewChanged` ping-pong.

## Frozen leading columns

Leading-frozen header and body cells are counter-translated against the horizontal scroll offset. Only the contiguous leading prefix from column 0 is pinned; a later `Leading` column is treated as non-frozen. This requires `ElementCompositionPreview.SetIsTranslationEnabled` (a `UIElement.Translation` X/Y change is otherwise a no-op). Non-frozen cells are clipped out of the pinned band so horizontally scrolled content does not draw underneath the frozen leading cells. This translation/clip approach is a v1 rendering detail; later interactive features (resize, reorder, selection) must not assume it.

# Implementation Details

The render pipeline is `TableView.ItemsSource` → `PART_RowsRepeater` → `TableViewRow` → per-column cell wrapper → `TableViewColumn.GenerateElement(dataItem)`.

`TableView` owns `PART_RowsRepeater.ItemsSource` (the template does not bind it directly). `Unloaded` clears the repeater source so queued repeater cache-build work cannot run against a detached subtree.

`ThemeSettings` (high-contrast tracking) is released on `Unloaded`, or earlier on the host `AppWindow.Destroying`, because removing its `Changed` handler after the window is destroyed fails fast. Hosts without an `AppWindow`, or whose top-level window belongs to another thread, use the `Unloaded` path.

Cell generation is column-specific:

- `TableViewTextColumn` creates a `TextBlock` and applies `Binding`.
- `TableViewTemplateColumn` instantiates `CellTemplate`; a null template produces an empty cell.
- Custom columns override `GenerateElementCore(Object dataItem)`.

`RowBackground` and `AlternatingRowBackground` provide opt-in row banding; when both are null, rows stay unbanded. `GridLinesVisibility` controls row/cell gridline borders; the column-header separators and bottom rule are always drawn. `Density` selects row minimum height and built-in cell/header padding for `Compact`, `Standard`, or `Comfortable`.

Keyboard handling is cell-aware and follows the WAI-ARIA treegrid model. `TableView` listens to bubbling `KeyDown` so template-column descendants handle input first. The table itself is not a tab stop (`IsTabStop=False`). It has **two tab stops**: the header band, then the body. The two share one column cursor, so Tab and Shift+Tab between them keep the current column.

- **Header band.**
  - The band is entered at the remembered column, or at the first header when no column has been chosen.
  - `Left`/`Right` move between visible headers and do not wrap (mirrored under RTL).
  - `Down` moves to the first row; `Up` stays in the band.
  - `Enter` and `Space` sort a sortable header (`Space` fires on key-up, so auto-repeat cannot re-sort). `Alt+Left`/`Alt+Right` resize it.
  - Every visible header is reachable, including one with `CanSort=False`.
  - `Tab` into a scrolled body from the column header band lands on the first fully visible enabled row or group header in the body viewport (a partially visible one if none is fully visible), or the last one for `Shift+Tab`; for a row at cell level, on the cell in the current column. Landing on a group header from the band, scrolled or not, leaves cell level, but the column cursor is kept. If no enabled element is in the viewport, the framework's target is kept. It never targets a cache row, so the body scrolls no further than the focus bring-into-view padding.
- **Rows.**
  - Unhandled `Up`, `Down`, `PageUp` and `PageDown` keep the starting level: row to row, or cell to cell in the same visible column. `Up` from the first row moves to the header band.
  - `PageDown` pages like `ListView`: it first moves to the last visible row, and from there one viewport (less one row) further, aligning that row to the bottom. `PageUp` mirrors it towards the first visible row and the top. The visible row is the last (or first) fully visible enabled data row or group header, falling back to a partially visible one when none is fully visible; disabled rows are skipped. A group header at that edge is a page target like a row (focused at row level, since headers have no cell level), so a viewport holding a single data row between headers still pages by the viewport rather than one header at a time. While focus is on a row or a cell the table handles both keys itself, so the body scroller does not page on its own; when focus is inside hosted cell content (or an open editor) the keys are left to that content and the body scroller's native paging.
  - A row that keyboard navigation focuses is scrolled fully into view, including a row that is not realized yet, such as the last row on `End`. Focusing a row, group header or cell whose row is already fully in view does not scroll the body: XAML pads a keyboard-focus bring-into-view by 20 DIPs, and the table drops that padding vertically (`TableView::OnBodyBringIntoViewRequested`), so for example the first `PageDown` does not nudge the viewport.
  - From a focused row, `Right` drills into the first cell. `Left` on the first cell returns to the row. `Left` on a row does nothing. On a row, `Left`/`Right` are handled before the body scroller sees them, so they never scroll the body on their own; only a newly focused cell is brought into view.
  - At cell level, `Left`/`Right` move the cell cursor within the focused row without wrapping. `Home`/`End` move to the first/last cell of the current row, and `Ctrl+Home`/`Ctrl+End` move to the first/last cell of the table. All horizontal keys mirror under RTL.
- **Selection.**
  - Selection follows keyboard focus.
  - `Ctrl` plus a navigation key moves focus without selecting.
  - `Space` (without `Alt` or `Ctrl`) selects the focused row from row or cell level.
- **Cell content.** `Enter` on a cell that hosts a focusable control moves focus into that control, and arrow keys then stay inside it. `Escape` returns to the cell. `F2` remains the begin-edit key.
- **Re-entry.** Focus entering the body from outside the table with direction `Next` or `Previous` (`Tab`/`Shift+Tab`, including a programmatic `TryMoveFocus(Next/Previous)`) returns to the same **record** that last had focus, even after a sort or filter has moved it, at the level it was left at: the same visible column at cell level, or the row at row level. This holds whichever row element the framework aimed at, including a control hosted in another row's template cell. The remembered record is resumed if it is realized and its row is enabled; otherwise entry lands on the first (`Tab`) or last (`Shift+Tab`) enabled row inside the body viewport at row level, so the fallback never targets a cache row or a disabled row. The same fallback applies when the remembered target can't take focus; the redirect is skipped only when no viewport row can. Entry aimed at a group header is not redirected. The entry is redirected at most once per focus operation. Pointer, arrow-key, UIA and `Focus()` moves are never redirected.
- **Re-shape while focused.** When a sort or filter re-shapes the rows while focus is **inside** the body, focus stays at the same **position**, the same projected row index, and so lands on whichever row now occupies it. This also applies when the focused record was filtered out. The body position is put back only when focus is in the body when the re-shape runs, or when the re-shape itself drops focus. If focus is anywhere else, focus stays there. This includes a column header (however focus got there, by keyboard, pointer, app code, or a UIA `SetFocus`) whose Enter, Space, click, or UIA Invoke caused the sort, and controls outside the TableView.
- **Gamepad.** Gamepad, D-pad and XY-focus navigation are not supported. Only the keyboard routing above is specified.

The internal `GridCoordinateHelper` performs the row/column ↔ flat-index math, wrap behavior, and overflow guards.

Accessibility exposes a UIA grid/table model:

- `TableViewAutomationPeer`: `IGridProvider`, `ITableProvider`, `ISelectionProvider`, `IItemContainerProvider`. `BoundingRectangle` is clipped to the control's layout box, so realized cache rows outside the body viewport are excluded; the default bounds are used when the clip is unavailable.
- `TableViewRowAutomationPeer`: `DataItem` control type, `ISelectionItemProvider`, composed name and group-relative `PositionInSet`/`SizeOfSet`; no grid/table provider (exposes its cell peers as children)
- `TableViewCellAutomationPeer`: `IGridItemProvider`, `ITableItemProvider`, `IValueProvider`, `Custom` control type with localized `cell`. The name is the cell value; while column headers are shown, the column header is reported only through `GetColumnHeaderItems`, and an empty cell is named with the localized `TableViewCellBlankName` ("blank"). While they are hidden, `GetColumnHeaderItems` (and the table's `GetColumnHeaders`) is empty and the name becomes "{header}, {value}"; an object or templated header contributes its header item's name. A cell whose content is focusable (a `Button`, `ToggleSwitch`, ...) is named after its column header alone in both cases, because the control is a Content-view child that names itself. Toggling the column-header bit of `HeadersVisibility` raises `Name` changed on cell peers that already exist.
- `TableViewColumnHeaderAutomationPeer`: column-header name/bounds, `IInvokeProvider` (sort), `PositionInSet`/`SizeOfSet`
- `TableViewGroupHeaderAutomationPeer`: `IExpandCollapseProvider`, `IGridItemProvider`. It computes **no** `Level`: grouping is single-level, so the band reports `0` ("not specified") unless the app sets `AutomationProperties.Level`.
- A stale `GridItem` (a cell whose row or item is gone, a cell replaced by a same-row rebuild such as a column reorder or `CellTemplate` change, a cell whose column is collapsed, or a recycled group header) throws `UIA_E_ELEMENTNOTAVAILABLE` from `Row` (a cell also from `Column`) instead of reporting `-1` or another cell's coordinate. A cell whose row is between recycle and re-prepare while its item is still in the source reports the item's index.

`ScrollItem` is not implemented explicitly on any of these — `FrameworkElementAutomationPeer` already supplies a `ScrollItemAdapter` for every peer.

Peers that compute `PositionInSet`, `SizeOfSet` or `Level` report `0` — UIA's "not specified" — when the value cannot be resolved, never `-1`, and an app-set `AutomationProperties` value always wins over the computed one.

Tabular's compiled `ResourceAccessor` reads `Microsoft.UI.Xaml.Controls.Tabular/Resources`, matching its PRI subtree. The in-box MUXC build retains `Microsoft.UI.Xaml/Resources`. A missing resource still falls back safely, but a correctly merged Tabular PRI now supplies the localized cell, sort, group and resize strings.

Lifetime rules: columns and rows use weak owner back-pointers (`GetOwningTableView()` resolves a strong owner for synchronous work); runtime classes use `ReferenceTracker` where required; cross-object events use `auto_revoke`; recycled rows reset transient visual state before reuse.

Theme values resolve through `TabularSurfaces` resources and re-resolve across theme (and high-contrast) changes. `TabularSurfaceGridLineBrush` (body grid lines) and `TabularSurfaceHeaderSeparatorBrush` (header separators and bottom rule) use the Fluent `DividerStrokeColorDefault` value and `TabularSurfaceOuterBorderBrush` (the control border) uses `CardStrokeColorDefault`; High Contrast uses `SystemColorWindowTextColor` for both. `TabularSurfaceHeaderSeparatorBrush` is the header line brush: overriding `TabularSurfaceGridLineBrush` does not recolor header lines, so override both to recolor every line. The stock style always provides the key; if it is missing, High Contrast falls back to `SystemColorWindowTextColor` and every other theme draws no grid line.

`TableView.xaml` does not redeclare the `TabularSurface*` brush keys: generic-style fallbacks would shadow the canonical palette and bypass its HighContrast system-color brushes. Consumers merge `TabularControlsResources` as documented by the sample.

Template naming unwraps realized presenters and reads public visible text or peer names, with an eight-level/32-element budget per cell. Explicit names and labels win. Name-based item search rechecks the realized public name instead of treating item text as an alias. Text edits capture only an existing cached cell peer; changed Value/Name notifications are deferred until teardown, guarded against recycling/newer values, and never cause peer creation. Space header activation is disarmed on completed focus loss, header replacement, template replacement and unload; autorepeat is consumed only while the same header remains armed. A column header's text is a string `Header`, or an untemplated non-`UIElement` header's `ToString` unless that is just the object's runtime class name (CsWinRT's default, typical of a header rendered by an implicit `DataType` template); otherwise — a `UIElement` header, one shown through `HeaderTemplate`/`HeaderTemplateSelector`, or a class-name `ToString` — it is named from its realized header content only, never from the grid line, sort indicator or resize gripper. Sort announcements use that same header name.

## Sort ownership and reconciliation

Sort has two front-ends and exactly one axis is ever in force; they reconcile rather than stack.

- `TableView.SortByColumn` (and header click) declares the control's own axis and publishes
  `TableViewColumn.SortDirection`, which is what draws the header chevron.
- `TableViewSource.Sort` declares an axis on the source. The path overload
  (`Sort(sortMemberPath, direction)`) names a property, so the control can match it against a
  column's `SortMemberPath` and light that column's chevron. The delegate overload
  (`Sort(keySelector, direction)`) is opaque — the key may not correspond to a column at all
  (computed key, multi-field, custom comparer) — so no chevron is shown.
- `TableViewColumn.SortDirection` is read-only to apps. An app that cancels `Sorting` to order the
  rows itself can make the chevron follow only by declaring that order with the path overload on
  the bound `TableViewSource`.

**Axis identity on the source.** A path axis is keyed on its path, so re-sorting the same path
replaces it in place. A delegate axis is keyed on the delegate object's COM identity
(`ShapingPipeline::SetSort`): the same delegate instance re-sorts that axis in place and `None`
removes it, while any other instance, even one with identical logic, is a new, least significant
axis, and `None` with it removes nothing. Path and delegate axes never match each other, and the
control's own axis is keyed on column identity, so two columns sharing a `SortMemberPath` stay
separate.

Reconciliation rules:

- Sorting through the control clears any axis the app declared on the source.
- Declaring a sort on the source clears the control's axis. A path-declared axis that matches a
  column lights that column and raises `Sorted` with it; otherwise every `SortDirection` is
  cleared and `Sorted` carries a null column.
- `ClearSort()` clears every axis, including one the app declared.

**Single owner.** A `TableViewSource` has exactly one owning `TableView`. A second `TableView`
bound to a source another table already owns is refused and renders empty until the owner releases
it — by assigning a different `ItemsSource` or being unloaded. A table releases its source on
`Unloaded` and reclaims it on `Loaded` (reconciling its sort state then), so a navigation that loads
the new page before unloading the old one resolves on its own. While refused, a table cannot sort.

**Refused sorts.** `SortByColumn` and `ToggleSortDirection` return `false`, with no state changed,
when the sort cannot be applied: besides a canceled `Sorting` or an edit that will not close, a
column with `CanSort=False` (or with neither a sort member path nor a `CustomSortComparer`) is not
sortable, and a source containing a null row object or the same object on two rows has no row
identity to sort by. `CanUserSortColumns=False` gates only header clicks, not these methods.
Setting `CanSort=False` on the sorted column clears its sort through an internal path that skips
that gate, raising `Sorting`/`Sorted` exactly as `SortByColumn(column, None)` does. A `Sorting`
handler can cancel that clear, and it is skipped while the source is refused; the column then keeps
its sort, and `SortByColumn(column, None)` (allowed on a column still holding a sort, whatever its
`CanSort`) or `ClearSort` removes it. A column lit only by the app's `TableViewSource` sort, with
no sort installed by the table, keeps that state and raises no events, as when
`CanUserSortColumns` turns off.

Without this, the earlier-declared axis would silently outrank the later one while the chevron
advertised the loser. WPF splits the same way: `SortDescriptions` carry a property name, which is
how `DataGrid` matches a column and lights its arrow, while a data-layer sort with no property
name stays headerless.


## Grouping model

Grouping is declared on the **data source**, never on the control: `TableViewSource.GroupBy(...)`
installs a grouping verb, and `TableView` renders whatever shape the source publishes. The control
has no `GroupBy` property and no group-key API of its own, which is what keeps the shaping stack
(`TabularShaping` ← `ShapedItemsSource` ← `TableViewSource` ← `TableView`) one-directional: the
engine never learns what a `TableView` is.

**Projection shape.** A grouped source publishes one flat row vector in which group headers are
interleaved with their member rows (`ShapedItemsSource::ProjectionKind::Grouped`). There is no
second nesting level in the layout and no nested repeater: the header is simply another entry in
the same index space. `TableView::GetRowKindForItem` classifies an entry, and
`TableViewRowTemplateSelector` picks `TableViewGroupHeader` or `TableViewRow` accordingly, so
neither container carries an adaptive "am I the other kind?" branch.

**Group identity is the bucketing key, not the key object.** `BucketizeToGroups` asks for a
stable, non-empty string identity per key. Value-typed keys (`String`, `Int32`, `Int64`, `Guid`,
`Boolean`, enums) resolve through a built-in identity; anything else requires the
`GroupBy(keySelector, groupIdentitySelector)` overload. An identity that is empty, non-string,
throwing, or that collides across two genuinely different keys **throws**
`hresult_invalid_argument` rather than degrading to a flat projection — the same fail-fast contract
row identity uses. A `null` key and an empty-string key are rejected on the same terms
(`ValueKey::TryGetStablePropertyKey` is called with `rejectEmptyString`), so grouping on a
nullable or blank-able property needs an app-supplied fallback label. Degrading silently would
ship apps whose grouping mysteriously does nothing, with no diagnostic. (Two distinct key
instances *may* map to one identity when the app supplied a `groupIdentitySelector`; supplying one
is the opt-in that makes the collapse intentional.)

Only **row** identity degrades instead of throwing, and the asymmetry is deliberate: a row identity
the engine cannot derive is a property of the app's data that the app may not control, so the
`Unshaped` mirror still shows every row — it just cannot be sorted by the control, and
`SortByColumn`/`ToggleSortDirection` return `false` up front; a bad *group* identity
comes from the `GroupBy(...)` selector the app just wrote, and an ungrouped table is an outcome it
would not notice.

**Composition order is meaningful.** A sort declared **before** `GroupBy` orders the groups; a
sort declared **after** `GroupBy` orders rows within each bucket
(`ApplySort(rows, -1, GroupOrder())` for the group axis, then `ApplySort(bucket.Items,
GroupOrder(), -1)` per bucket). A filter runs first in either case, and a group whose last member
is filtered out ceases to exist.

**Expansion is intent keyed by identity, held outside the groups.** `RowExpansionModel` (layer 1)
stores only the *exceptions* to a default, so `ExpandAllGroups`/`CollapseAllGroups` move the
default in O(1), a group that appears later inherits the current default rather than someone
else's stale intent, and a collapse survives a re-sort, a re-filter or a regroup that re-mints
every `ShapedGroup`. Storing `IsExpanded` on the group object instead is what used to lose a
collapse across a re-sort. `RetainOnly` prunes intent for identities that no longer exist so the
store cannot grow without bound across changing data sets.

**Toggle path.** `TableViewGroupHeader` raises `ToggleRequested`; the owning container forwards it
to `TableView::ToggleGroupExpansion`, so the header stays free of `TableView` plumbing and is
testable on its own. The whole band is the toggle target (matching `ListView`/`TreeView`) rather
than a nested button, so there is one interactive element and one automation story. Because
expanding or collapsing re-publishes the projection, the realized header the gesture started on is
gone by the time the work runs: expansion is queued **by identity** with a generation stamp
(`QueueGroupExpansionByIdentity` / `ApplyGroupExpansionByIdentity`), and focus is captured and
restored by identity too (`CaptureContainerFocusForRestore` /
`RestoreContainerFocusIfPending`), so focus does not drop to the top of the
table on every collapse. The restore covers group headers and hierarchical data rows. For a header the restored focus state follows the gesture (`Keyboard` for a key, `Pointer` for a click), and a restore after a pointer click on the header does not scroll it into view. A data row is restored only from keyboard or programmatic focus, or when `Multiply` expands its subtree (a keyboard gesture, whatever state the row's focus had), always with `Keyboard` state, and only when the reset recycled its container. The restore is skipped when focus has meanwhile moved outside the table or the host window is hidden. A pointer click on a band focuses the header on **release** inside the band, before the toggle, and only when the band still shows the group that was pressed; if committing an open edit re-targets the header to another group, the click is consumed without toggling. A press dragged off the band does nothing.

**Header presentation.** `TableViewGroupHeader` is a templated `ContentControl`, not a code-built
visual tree: the chevron gutter (`PART_ExpanderGutter` / `PART_ExpanderIcon`) and the themed band
live in the control-owned `ControlTemplate`, while `TableView.GroupHeaderTemplate` fills only the
content region — so an app template cannot accidentally drop the expander. Its `Content` is a
`TableViewGroupInfo` projection that is updated **in place** and raises `PropertyChanged`, so
recycling a header does not re-evaluate every binding in the template. `KeyText`/`ItemCountText`
are computed lazily through a `DecimalFormatter`, so a template that binds only `Key`/`ItemCount`
pays nothing for them.

Under grouping, a hierarchical root row's lead cell is indented by the group header's expander
gutter plus its spacing (`TableViewGroupExpanderSize` + `TableViewGroupExpanderSpacing`, 32px by
default). The header's own `Padding` is not part of the indent, so the root row's chevron starts
`Padding.Left` (8px by default) left of the group key text rather than under it.

**Interaction with selection.** Group headers share the flat projection's index space with data
rows but are **not** selectable. `Select(index)` on a header index is rejected rather than coerced
(without that, the internal grouped entry would be handed to the app as `SelectedItem`), and
selection-follows-focus leaves the existing selection alone when keyboard navigation lands on a
header, resuming on the next data row. The consequence an app must know: under grouping,
`SelectedIndex` is an index into a row space that *includes* headers, so it is not an index into
`ItemsSource`.

**Accessibility.** `TableViewGroupHeaderAutomationPeer` is separate from
`TableViewRowAutomationPeer` because the header is its own container: there are no `GridItem`
coordinates for a band spanning every column, and `ExpandCollapse` is advertised unconditionally —
a non-expandable group reports `LeafNode` rather than dropping the pattern, so an AT client never
sees the pattern appear and disappear as data changes.

**Known limitations.** A source change under grouping rebuilds the projection rather than splicing
it (the flat incremental fast path is explicitly disabled by `ClearFlatRowIdentityTracking`), so
grouping is O(N) per collection change. There is no per-group programmatic expand/collapse by key,
only the bulk commands plus the header's own gesture and UIA pattern. Grouping is single-level:
`TableViewGroupInfo.Level` exists for the shape but is always `0` in v1.

# Styling model

Styling is layered so the display-only base stays minimal while a richer surface can grow
additively — the realized row/cell/header are already styleable targets (`TableViewRow` is a
public `Control`; cell wrappers are `Border`s; column headers are public, sealed
`TableViewColumnHeader` controls that only the TableView creates), so the additions below
need no new base API or re-plumbing.

**v1 (this base):**
- `RowBackground` / `AlternatingRowBackground` — opt-in row banding (both null = unbanded, WPF
  `DataGrid` parity; `AlternatingRowBackground` overrides odd rows only when set).
- `GridLinesVisibility` — row/cell gridline borders.
- `Density` — row min-height + built-in cell/header padding preset.
- `TabularSurfaces` theme resources — the theme-aware (light/dark/high-contrast) source of the
  default brushes, re-resolved on theme and high-contrast changes.
- Typography theme keys — `TableViewCellFontSize` (14), `TableViewHeaderFontSize` (14) and
  `TableViewHeaderFontWeight` (`SemiBold`). They do not vary with `Density`; an override in the
  TableView's or an ancestor's `Resources`, or in `Application.Resources`, wins. The cell size is
  cached per instance, so a later override applies after a theme, high-contrast or density change.
  The header keys are `{ThemeResource}` setters in the `TableViewColumnHeader` default style, so a
  header style's `FontSize`/`FontWeight` setter wins over them and nothing is written as a local value.
- `ColumnHeaderStyle` / `TableViewColumn.HeaderStyle` — `Style` for the templated
  `TableViewColumnHeader` (`ContentControl`); the column's style wins. Both are layered over the
  default header style, so they need not be `BasedOn` it. Pointer-over and pressed are `CommonStates`
  (`Normal`/`PointerOver`/`Pressed`/`Disabled`) that change only the fill (the `TabularSurfaceHeader*`
  brushes), so a custom `TableView.Foreground` survives hover; under High Contrast a separate
  `HighContrastStates` group (`HighContrastNormal`/`HighContrastPointerOver`/`HighContrastPressed`)
  swaps the text and glyph to the highlight-text brush. `SortStates`
  (`Unsorted`/`SortAscending`/`SortDescending`) only show or hide the sort glyph. `MinHeight` and
  `Padding` follow `Density` unless the applied header style sets them (an implicit header style's
  setters for those two are overridden by the density values).

**Rich styling (later):** delivered as **per-element `Style` dependency properties**, matching WPF
`DataGrid` (`RowStyle` / `CellStyle` / `ColumnHeaderStyle`) — **not** a monolithic `TableStyle`.
`ColumnHeaderStyle` has shipped (above); `RowStyle` and `CellStyle` follow the same shape.
`RowStyle` targets the realized `TableViewRow`; `CellStyle` the per-cell container (the cell-wrapper
`Border`; cell *content* is customized via the column's template / `GenerateElementCore`, matching
WPF where `CellStyle` targets `DataGridCell`, not the content); `ColumnHeaderStyle` the generated
header. The header went first because its pointer, sort and typography visuals were the ones
built in code; the public `TableViewColumnHeader` type and `ColumnHeaderStyle` are preview API
pending API review.

**Why per-element Styles + theme resources, not a monolithic `TableStyle`:** per-element `Style`s
compose with the standard WinUI `Style`/`Setter`/`ControlTemplate` machinery and scale to any
property without a dependency-property explosion as selection/sort/hover/focus visuals arrive;
theme-awareness is owned by the `TabularSurfaces` resource dictionary (override a `TabularSurface*`
key at app/page scope and it re-resolves per theme); and a single swappable "table look" is already
expressible as a `Style` targeting `TableView` plus a `ResourceDictionary` of `TabularSurface`
overrides.

**Precedence:** the v1 convenience brushes (`RowBackground` / `AlternatingRowBackground`) are
retained as WPF-migration shortcuts and **augment** the Style DPs — an explicitly set convenience
brush wins over the corresponding `RowStyle` setter. If they prove redundant once `RowStyle` lands,
they can be obsoleted while the API is still `[MUX_PREVIEW]`.

# Architectural overview

`TableView` owns the column collection, row realization, header realization, scroll synchronization, frozen-column layout, density/theming state, keyboard navigation, and the automation surface. The architecture separates:

- data realization: `ItemsRepeater`
- row presentation: `TableViewRow`
- cell generation: `TableViewColumn`
- header presentation: generated header cells in `PART_HeaderHost`
- coordinate math: `GridCoordinateHelper` (internal)
- accessibility: dedicated automation peers

## Key public components

**`TableView`** — `Control`, `[contentproperty("Columns")]`. DPs: `ItemsSource`, `Columns` (get-only `IVector<TableViewColumn>`), `HeadersVisibility`, `GridLinesVisibility`, `RowBackground`, `AlternatingRowBackground`, `EmptyTemplate`, `Density`, `IsReadOnly` (default `true`).

**`TableViewColumn`** — `DependencyObject`, `[contentproperty("Header")]`. Members: `Header`, `HeaderTemplate`, `HeaderTemplateSelector`, `Width` (`GridLength`), `MinWidth` (`20`), `MaxWidth` (infinity), `ActualWidth` (get-only), `FrozenEdge`, `Visibility`, `IsReadOnly` (default `false`), `GetOwningTableView()`, `GenerateElement(Object)`, overridable `GenerateElementCore(Object)`, `GenerateEditingElement(Object)`, overridable `GenerateEditingElementCore(Object)` and `GetEditingPropertyPathCore()`.

**`TableViewTextColumn`** — `TableViewColumn` that generates text cells. `Binding` is a plain CLR property. Its editing element is a `TextBox` bound with a clone of `Binding` forced to `TwoWay` + `UpdateSourceTrigger=Explicit`. The built-in text column's editor `TextBox` takes `AutomationProperties.Name` from the column header text.

**`TableViewTemplateColumn`** — `TableViewColumn` that generates templated cells. DPs `CellTemplate` and `CellEditingTemplate`.

**`TableViewRow`** — `Control` representing one realized item row. `GetOwningTableView()`.

**`TableViewGroupHeader`** — `ContentControl` representing one realized group-header band under a
grouped source. DPs `IsExpanded`, `IsExpandable`; event `ToggleRequested`
(`TableViewGroupHeaderToggleRequestedEventArgs.GroupKey`). Template parts `PART_ExpanderGutter`,
`PART_ExpanderIcon`; visual states `CommonStates`, `ExpansionStates`, `ExpandabilityStates`.

**`TableViewGroupInfo`** — read-only `INotifyPropertyChanged` projection bound by a
`GroupHeaderTemplate`: `Key`, `ItemCount`, `Level`, `IsExpandable`, `IsExpanded`, and the
culture-formatted `KeyText` / `ItemCountText`. Updated in place across recycling.

**Automation peers** — `TableViewAutomationPeer`, `TableViewRowAutomationPeer`, `TableViewCellAutomationPeer`, `TableViewColumnHeaderAutomationPeer`, `TableViewGroupHeaderAutomationPeer` (providers: `IGridProvider`, `ITableProvider`, `ISelectionProvider`, `IItemContainerProvider`, `IGridItemProvider`, `ITableItemProvider`, `ISelectionItemProvider`, `IValueProvider`, `IInvokeProvider`, `IExpandCollapseProvider`).

**Enums** — `TableViewFrozenEdge` (`None`/`Leading`/`Trailing`), `TableViewHeadersVisibility` (`None`/`Column`), `TableViewGridLinesVisibility` (`All`/`Horizontal`/`None`/`Vertical`), `TableViewDensity` (`Compact`/`Standard`/`Comfortable`), `TableViewEditingUnit` (`Cell`/`Row`), `TableViewEditAction` (`Commit`/`Cancel`/`Discard`).

# Editing

Editing is split into two files on purpose:

- **`TableView_Editing.cpp`** owns the edit **state machine** and is reachable entirely through the programmatic API.
- **`TableView_EditingInput.cpp`** owns the **policy** of which user gestures map onto that API.

The split is what lets a future keyboard or selection layer change gesture routing — which key commits versus moves, whether `Tab` advances the cell — without reopening the state machine, and it keeps the state machine testable without synthesizing input.

## Edit state machine

`EditState` is explicit: `None → Beginning → {None, Editing}` and `Editing → Ending → {Editing, None}`. `Ending → Editing` is the veto / validation-failure / deferral path — the edit stays open. It replaces a set of independent booleans whose combinations encoded the real invariants only implicitly.

## Gesture layer

Begin-edit is driven from **`PointerPressed` with click-count tracking**, not a `DoubleTapped` handler. Marking `PointerPressed` handled suppresses XAML's gesture recognizer, so it never produces `Tapped`/`DoubleTapped` for that element — and a row that participates in selection has to mark the press handled. A `DoubleTapped` handler would therefore work today and silently stop working the moment a selection layer lands. The handler is registered with `handledEventsToo` so it survives that.

The same handler is the only place a pointer establishes the **current cell**, which is also what makes keyboard editing reachable. A primary-button press on a cell drills its row to cell level and focuses **that cell**, so keyboard focus, the UIA focused element, and the current cell all name the cell under the pointer. A press on the row's empty strip, past the last column, focuses the row itself.

Selection is applied on **release**, and only when the release lands on the row that was pressed. A press that is dragged off and released elsewhere selects nothing. A secondary-button (right) press never selects, because it is the context-menu gesture.

Cell resolution walks up from `OriginalSource` to the cell wrapper (`TableViewCell`, a `Grid` whose `Tag` carries the column) and on to the `TableViewRow`. When the walk cannot reach a wrapper — which happens once the row has focus and the press arrives with the row itself as source — it falls back to hit-testing at the pointer position.

## Value transfer and rollback

Editing bindings use `UpdateSourceTrigger=Explicit`, so the typed value lives in the editor until the control pushes it. That is what lets a cancel restore something and a validation failure hold the edit open. On commit the control calls `UpdateSource()` on the binding expressions discovered from the editing element; validation runs **after** the write, and a blocked commit undoes it.

A forced close (the source changes under the edit, or the control turns read-only) commits a valid pending value while the edited item is still in the source and passes the filter, and reports `Cancel` on a failed write or validation error. Because the pending value is still in the editor, a forced close first transfers it to the item, then validates; a value that fails validation is rolled back to the pre-edit value, so the item never keeps a value the control rejected. Inside a layout pass (a recycled or rebuilt row) restoring the editor would re-enter layout, so that rollback is posted to the dispatcher, and it is skipped if the item's value has changed in the meantime — another edit of the same cell is open, or the property no longer holds the rejected value — so a newer write is never overwritten. If the item was removed, filtered out, or `ItemsSource` was replaced, it reports `Cancel` and writes nothing, as WPF `DataGrid` does; an item that is only under a collapsed group or node has not been removed. A forced close during a pending commit deferral can finish as `Cancel` without a second `CellEditEnding`.

There is no item-owned transaction interface in this release (`ITableViewEditableItem` is deferred with row editing). The control snapshots the value at the edited property path, falling back to scraping the editor's bindings — inherently incomplete for an arbitrary editor, so the inability to capture anything is recorded and reported rather than silently ignored.

## Re-entrancy rules (load-bearing)

Row recycling, cell rebuild and dependency-property change callbacks can all run inside a layout pass or a framework callback. Mutating focus from there re-enters XAML and trips the re-entrancy assertion (`0xc0000420`). Therefore:

- `TableViewRow::EndCellEdit` (restores the display child **and** moves focus) is only used on the interactive close paths.
- `TableViewRow::AbandonCellEdit` restores the display child but never touches focus, and is what `TableView::TerminateEditWithoutVisualRestore` drives from `OnRowElementClearing`, `RebuildCells`, `OnIsReadOnlyPropertyChanged` and `OnItemsSourcePropertyChanged`.
- The cell **restamp** fast path deliberately does *not* tear the edit down — it reuses the existing cells, and closing the edit there would make it impossible to keep an editor open through the re-measure that installing the editor itself provokes. Only the destructive rebuild path closes it.
- The focus-loss commit is posted to the dispatcher and re-evaluated once focus settles; deciding synchronously closes the edit inside the very gesture that opened it.

## Key internal components

- **`PART_HeaderScroller`** — horizontal-only header scroller synchronized with the body. Found as `PART_HeaderHost`'s nearest `ScrollViewer` ancestor; the name is for readability only.
- **`PART_HeaderHost`** — `TableViewCellsPanel` header host rebuilt from the non-null columns.
- **`PART_BodyScroller`** — body scroll presenter and source of horizontal offset. Found as `PART_RowsRepeater`'s nearest `ScrollViewer` ancestor; the name is for readability only.
- **`PART_RowsRepeater`** — vertical row realization surface.
- **`PART_CellsHost`** — row-local `TableViewCellsPanel` cell host.
- **`TableViewCellsPanel`** — the custom `Panel` used for `PART_HeaderHost` and each row's `PART_CellsHost`; its `MeasureOverride` reports each cell's measured width and its `ArrangeOverride` places cells at the column's resolved `ActualWidth` (see *Column alignment*).
- **Cell wrappers** — per-row/per-column `Border` elements that host generated cell content, carry gridline/background visuals, and are arranged by the hosting `TableViewCellsPanel` at the owning column's `ActualWidth`.
- **Frozen-column translation/clipping layer** — pins leading columns while clipping non-frozen content out of the pinned band.
- **`GridCoordinateHelper`** — internal C++ helper for row/column ↔ flat-index and focus-navigation math used by keyboard navigation; not projected (no public runtimeclass).
- **`TableViewRowTemplateSelector`** — picks the container kind for an entry in the projection: a `TableViewGroupHeader` for a group entry, a `TableViewRow` for a data row.
- **`RowExpansionModel`** — internal layer-1 store of expand/collapse *intent*, keyed by group identity and holding only the exceptions to a default. Carries no XAML or tabular vocabulary, so hierarchy can reuse it unchanged.

# Telemetry and TiP

TableView reports whether its own work succeeded (seven TiP tests) and how it is used (three events), on the
Tabular providers. It collects no item data and no per-row or per-cell counts (see *Not collected* under
*Events*), and it blames the app only when app code decided an outcome (see *Who is blamed*).

On success an operation with a TiP test pays for its TiP start and stop and for reading the flow direction and the
source's row counts for its metrics; a sort, filter or edit commit, or a request to expand or collapse one group or
tree node, also sets a used-feature bit. Each shaping verb resets and reads a count of app delegates that threw,
each call into `SelectionModel` clears a one-entry note, and while a loaded session is active an expansion request
first looks up whether it changes its group or node. Shaping also keeps two counts for the metrics: the rows the
last refresh read and, for a tree, the nodes the filter keeps. Attribution work runs only when something throws.

## TiP tests

| Test | Runs | Fails when TableView's own code throws (the HRESULT is included) |
|---|---|---|
| `TableViewInitializationTest` (64477764) | Once per template application, from the template applied with a source attached (whichever comes later) to the layout pass of the first row or group header, or at once for an empty view | `template_setup_failed` in `OnApplyTemplate`, `source_attach_failed` while attaching the source |
| `TableViewSortTest` (64745033) | An accepted sort (column header, `SortByColumn`, `ClearSort`, or a `TableViewSource` sort verb), until the sorted rows are published and TableView has finished reacting on that turn (for a column sort, restoring selection, raising `Sorted` and the announcement) | `sort_failed` |
| `TableViewEditBeginTest` (64745034) | A request to edit an editable cell, until its editor is ready | `edit_begin_failed` |
| `TableViewEditCommitTest` (64745035) | A commit, until the value is written and the editor is closed | `edit_commit_failed`, also when TableView's own column cannot rebuild the cell's display after the commit |
| `TableViewFilterTest` (64745036) | `Filter` or `ClearFilter` on the table's `TableViewSource`, until the rows are published and TableView has finished reacting on that turn | `filter_failed` |
| `TableViewGroupExpansionTest` (64745037) | Expanding or collapsing one group, on the turn TableView applies the request | `group_expansion_failed` |
| `TableViewHierarchyExpansionTest` (64745038) | Expanding or collapsing one tree node, on the turn TableView applies the request | `hierarchy_expansion_failed` |

The operation tests share one implementation and report under their own measures (`OperationTestIdentity` in
`TableViewTipTests.h`). Initialize closes Ignored with `app_template_incomplete` when a custom template lacks
`PART_RowsRepeater`, `PART_HeaderRow` or `PART_HeaderHost`, and with `unloaded_before_render` when the table is
unloaded while Initialize waits on rows to show and a non-zero body viewport (a realization fault, or a navigation
faster than the first layout); any other unload closes a running test with `unloaded`. Every test's metrics bucket is the
flow direction and size when it started: 0 left-to-right, 1 left-to-right with a source of at least 1,000 rows
before the filter (every node for a tree), 2 right-to-left, 3 right-to-left with at least 1,000; the result's metrics
carry `rightToLeft`, `largeData`, and the `SourceRowsBucket` and `DataRowsBucket` counts as `sourceRowsBucket` and
`dataRowsBucket`. A source swap before Initialize has its verdict is part of Initialize.

A source swap after Initialize's verdict, selection, a cancelled edit, group configuration, a subtree expansion and an
expansion request that changes nothing (its group or node is already in that state, or is gone) run no TiP test; a
failure in one still writes `TableView_Error` while a loaded session is active (a source swap also before the first
`Loaded`). Measure/arrange, cell realization, and expanding or collapsing all groups or rows are not tracked. A clear
verb, or a `Sort` with `SortDirection.None`, that has nothing to remove is not measured either. Scrolling is not
measured in the field; scroll performance is left to a lab benchmark.
Internal pipeline builds (`IsInternalWinUIBuild` and `TF_BUILD`) fail when Tabular falls back to the public TiP stub;
`UseTestInProductionStub=true` chooses the stub on purpose.

A process exit while a test runs is reported by TiP as that test's failure (`process_shutdown`). On Windows builds
whose Error Reporting recovers TiP results, so is a crash or hang (`crash_observed`, `hang_observed`; these come from
the OS runtime, not the TiP package TableView builds with). These bypass the test's rules, so TableView cannot
reclassify them. `Unloaded` and the destructor close a running test when they run before the exit.

## Who is blamed

- App code is blamed only when it decided the outcome: its exception left the operation, or its write
  stopped it. The test closes Ignored with `application_fault`; the `TableView_Error` diagnostic carries
  the category (callback, validation, write, factory or data).
- An app fault TableView recovers from (a throwing `BeginningEdit` or `CellEditEnding` handler,
  validation read, editing template or display factory) is reported, and the outcome stays TableView's.
- The exception carries the blame. Each call into app code rethrows the app's exception tagged with the category,
  as a `CallerError` with the same HRESULT and error info (running out of memory stays a `std::bad_alloc`), and the
  shaping engine throws the app's collection and contract failures the same way; the catch that decides an outcome
  reads the type. A swallowed app exception, or another exception with the same HRESULT, is never blamed on the app.
  `SelectionModel` calls TableView back across the ABI, which keeps an exception's error object but not its C++
  type, so the call into `SelectionModel` tags the exception again when its error object is the one that left
  TableView's handler.
- A column's display factory and sort member path are the app's code only when the app derived the column class
  (it can override `GenerateElementCore` and `GetSortMemberPathCore`). The editing hooks are TableView's code for
  every column (they are internal), so their failure fails the test, except the content of a `CellEditingTemplate`,
  which is the app's while the editor is prepared (that is when it is realized). A write that does not land (a
  converter throws, or the editor has no binding TableView can write through) is still the app's.
- An app filter predicate, sort or group key selector, or custom comparer that throws is contained by design: the
  row is filtered out, the key is null, the pair compares equal. The verb's outcome stands, and `TableView_Error`
  reports a recovered callback fault with the first HRESULT and how many threw.
- An app fault never overrides TableView's own failure in the same operation; each is reported, subject to
  the diagnostic's once-per-session limit.
- An app-supplied collection that throws when read, and broken caller contracts (one object on two rows, a
  row without an identity, invalid group or parent identities), are data faults. TableView's own invalid
  arguments, and failures reading collections TableView builds (such as the group list), stay TableView's.

Known gaps: app code TableView calls without tagging it (for example a header's `ToString`) counts as
TableView's if it throws inside a measured operation, and so does an element from a column factory that
XAML refuses because a container outside the visual tree holds it. An app exception that reaches TableView
back across any other ABI call loses its tag and counts as TableView's, and so does an app `SelectionChanged`
handler that runs out of memory inside a `SelectionModel` call, since that failure has no error object to match. For
the display factory and the sort member path the column class decides, so a built-in method that throws on an
app-derived column without an override counts as the app's, and a built-in text column's binding converter that
throws while the column builds a cell counts as TableView's. Running out of memory while walking an enumeration-only
source counts as TableView's, because
TableView's own allocation may be what failed. App delegates that throw outside a shaping verb (live shaping, a
source change) are not reported, nor is a `ParentBy` parent key selector that throws (its row becomes a root).
A source setter that throws during a `{Binding}` write never reaches TableView (`UpdateSource` returns normally),
so that commit counts as completed although the value did not land.

## Events

| Event | When | Contents |
|---|---|---|
| `TableView_Loaded` | First `Loaded` in a process while a listener is enabled (a later `Loaded` retries) | `LoadedVersion`, control version, `UTCReplace_AppSessionGuid` |
| `TableView_Instance` | Unloaded or destruction, once per loaded session, at most 64 per process | `InstanceVersion` and the fields below, `UTCReplace_AppSessionGuid` |
| `TableView_Error` | A TableView failure, an app fault, or a caught header-rebuild error | Operation, stage, attribution, category, `HResult` with `HResultAvailable`, `FaultCount` (more than 1 only for contained app delegates), `IsRecoverable` (true when TableView carried on). Logging provider, VERBOSE, `KEYWORD_TABLEVIEW`, no telemetry keyword; the first per operation, attribution and recoverability in a session (within an edit or selection, an app fault that decided the outcome comes first) |

Every event also carries `SchemaVersion`.

`TableView_Instance` fields:

| Field | Values |
|---|---|
| `IsFirstLoad` | True for the instance's first loaded session |
| `ContentShown` | Across the instance's loads: 0 nothing yet; 1 a row or group header was shown; 2 no row was shown and a sample saw a source with no data rows |
| `EnabledFeatures`, `UsedFeatures` | Sorting 1, editing 2, filtering 4, grouping 8, hierarchy 16, live shaping 32 |
| `SourceRowsBucket` | Rows before the filter: every source row, or for a tree every node |
| `DataRowsBucket` | Data rows after the filter, without group headers and whatever is expanded; for a tree, the nodes the filter keeps |
| `PeakRealizedRowsBucket` | Containers the rows repeater has created since the template was applied. It creates one only when none is free and keeps them across loads, so this is the most rows realized at once, plus the most group headers, over the instance's loads |
| `ColumnKinds` | Text 1, template 2, app-derived 4 (a column class that is not TableView's) |
| `SourceKinds` | A collection TableView projected 1, the app's own `TableViewSource` 2 |
| `IsRightToLeft` | The table flows right to left |
| `TemplateIncomplete` | The applied template lacks `PART_RowsRepeater`, `PART_HeaderRow` or `PART_HeaderHost` |

TableView samples at `Loaded` and `Unloaded` while the event can still be written. An event carries the union of
the bits and the larger count, as a bucket: 0 for none, k for 2^(k-1) up to 2^k - 1. A session that ends by
destruction without `Unloaded` carries the `Loaded` sample alone.

Enabled means allowed by the table for sorting (`CanUserSortColumns` and a column that can sort) and editing (not
`IsReadOnly`, and an editable column), and declared on the `TableViewSource` for the other four. Used counts only
after the instance has shown rows: a sort, filter or edit commit that ran; a request to expand or collapse one
group or tree node (expand-all and subtree requests do not count); or an item change that live shaping had to
reshape for. A used feature is always reported enabled.

Three counts answer different questions and are never divided into one another: constructions
(RuntimeProfiler), instances (`TableView_Instance` with `IsFirstLoad`), and TiP results.

Not collected: action counts, cosmetic settings (density, grid lines, header visibility, row height),
tooltips, keyboard paging, column add or remove, automation peers, whether an `EmptyTemplate` is set, and
per-row or per-cell counters.

## Versions

Each starts at 1, the first release.

- A TiP test's data version: when its outcome rules or its data change.
- `InstanceVersion` and `LoadedVersion`: when that event's fields or their meaning change.
- `SchemaVersion`: when any TableView event changes.

# Appendix

## Non-goals

`TableView` v1 does not attempt to replace `DataGrid`. The true v1 non-goals are marquee selection, multi-column sort, column virtualization, row headers, and spreadsheet-like interaction. (Selection, single-column sort, filtering, grouping, and hierarchical rows are v1 features, not non-goals, and are delivered. Cell editing is delivered by this PR.)

## Out of Scope

Delivered later in the v1 stack: column reorder · navigation-state persistence · samples · tests · theme-XBF emission.

Already delivered in the v1 stack: row selection · shaping primitives · filtering · single-column sort · single-level grouping.

Out of v1 / follow-up work (see Non-goals): `Auto` auto-shrink (v1 widths grow monotonically) · trailing frozen columns · multi-column sort · column virtualization · row headers · clipboard · incremental loading · multi-cell row edit transactions · editing a11y announcements.

## Accessibility notes

The UI Automation grid/table provider counts and exposes only **visible** columns: a column with `Visibility.Collapsed` is excluded from the UIA column model, so `ColumnCount`, `GetItem`, the column-header set, and every cell/header column index (`GridItem`/`TableItem` `Column`, `PositionInSet`/`SizeOfSet`) are on a visible-column basis. Consequently, toggling a column's `Visibility` changes the reported UIA column geometry (indices to the right shift). This intentionally diverges from WPF's `DataGridAutomationPeer` (which indexes the full `Columns` collection) so that a collapsed, zero-width column is never surfaced as an addressable off-screen cell. The visual layer still generates a hidden cell per collapsed column (see *Row*), but the automation tree does not expose it.

A cell's column-header context comes from exactly one source: `ITableItemProvider.GetColumnHeaderItems` or the cell name, never both. Screen readers announce the header items when focus enters a new column, so naming a cell "{header}, {value}" while its header is reachable makes the header heard twice. The cell name carries the header only while `HeadersVisibility` hides the column headers, and `GetColumnHeaderItems` is then empty. An empty cell is named "blank" rather than after its header, so it never reads like a cell whose value is the header text. The one exception is a cell holding an interactive control: its name is the column header, because the value is the control's own name and repeating that would be worse.
