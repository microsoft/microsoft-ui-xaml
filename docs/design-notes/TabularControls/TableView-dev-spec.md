# Description

`TableView` is the minimal, display-only tabular base control for `Microsoft.UI.Xaml.Controls.Tabular.dll`. It renders an `ItemsSource` as virtualized rows and uses `Columns` to generate one cell per realized row/column intersection.

This is the minimal, display-only base of the control, and it is read-only. It establishes column ownership, header/body layout, row virtualization, cell generation, leading frozen columns, basic row keyboard navigation, automation peers, density/theming hooks, and the lifetime patterns later work builds on. The interactive v1 features land additively on top of it: selection, filtering, single-column sort and single-level grouping are delivered; two-level hierarchy and column resize/reorder are still outstanding.

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

Single-level grouping is delivered (see *Grouping model* below): group headers are their own container kind interleaved into the flat row projection, not a second nesting level in the layout. Two-level hierarchical presentation is still deferred, because a real nesting level requires different layout, virtualization, focus and accessibility models than a banded header does. Hierarchy deeper than two levels is out of scope for v1.

`TableViewRow` is intentionally a public `unsealed` `Control` despite carrying no v1 dependency properties: it is the realized row container (mirroring `ListViewItem`/`DataGridRow`) and is referenced by the public `TableViewRowAutomationPeer` and `TableViewCellAutomationPeer` constructors, so it cannot be internalized without collapsing the public accessibility surface. Row-level features layer onto it: selection already does, and hierarchy will. Grouping deliberately does **not** — a group header is a `TableViewGroupHeader`, a separate container kind chosen by `TableViewRowTemplateSelector`, so `TableViewRow` never grows an "am I a header?" adaptive branch.


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
      │        └─ TableViewCellsPanel PART_HeaderHost   -> one Grid header cell per column (L->R)
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

`PART_HeaderRow` provides the header-surface background and a 1px bottom gridline. `PART_HeaderScroller` is horizontal-only (`HorizontalScrollMode="Auto"`, horizontal scrollbar hidden, vertical disabled). `PART_HeaderHost` is a `TableViewCellsPanel` (inherently horizontal); `RebuildHeaders` fills it with one `Grid` header cell per non-null column (each cell mirrors the column's `Visibility`), in `Columns` order, left-to-right. Header cells use the density row minimum height, so the header band matches body rows. Header content comes from `TableViewColumn.Header`, displayed via `HeaderTemplateSelector` when set, otherwise `HeaderTemplate`, otherwise the default text presenter.

## Body band

`ScrollViewer` `PART_BodyScroller` → `Grid` `PART_BodyContent` → `ItemsRepeater` `PART_RowsRepeater`.

`PART_BodyScroller` enables both horizontal and vertical scrolling with auto scrollbars. `PART_RowsRepeater.Layout` is a vertical `StackLayout` (`HorizontalCacheLength`/`VerticalCacheLength` = `2.0`; `ItemTemplate` = `TableViewRow`). `PART_EmptyStatePresenter` is a `ContentControl` overlaid in the same `PART_BodyContent` cell, `Collapsed` unless the empty state is shown.

## Row

`Border` `PART_RootBorder` → `TableViewCellsPanel` `PART_CellsHost`.

`PART_RootBorder` is the row chrome: it supplies the default 40px minimum row height, a 1px bottom gridline, and the `CommonStates` visual states (`Normal`, `PointerOver`, `Pressed`, `Disabled`) that animate the row background. `PART_CellsHost` is a `TableViewCellsPanel` (inherently horizontal); `RebuildCells` fills it with one cell-wrapper `Border` per non-null column (mirroring the column's `Visibility`), in `Columns` order, left-to-right. Each wrapper hosts the element returned by `TableViewColumn.GenerateElement(dataItem)` when non-null. The row's `CornerRadius` (default `ControlCornerRadius`) is consumed only by the system focus visual (`UseSystemFocusVisuals="True"`, which rounds to the focused element's corners), so row/keyboard focus is rounded; it is intentionally **not** bound to `PART_RootBorder`, keeping the row background and 1px bottom gridline rectangular across the gapless row stack (no corner notches between adjacent rows).

## Column alignment

The header band and each realized row host their cells in a `TableViewCellsPanel` — a custom `Panel` whose `MeasureOverride` measures every cell twice (first unconstrained, to learn its measured width for Auto sizing; then a second pass constrained to the column's resolved `ActualWidth`) and whose `ArrangeOverride` places each cell at that `ActualWidth` (`x` = sum of preceding visible columns' widths; a collapsed column arranges to zero width). The second, constrained measure matters: without it, a cell whose content is wider than its column keeps a `DesiredSize` larger than the column, so XAML's arrange would expand the cell's *render* size to that desired width and clip it to the column slot — which clips away the cell's right border (the vertical gridline). Constraining the measure keeps the ellipsized content and the border inside the column so the boundary gridline renders. The header band and every realized row stay aligned with no cross-row coordination because every panel reads the *same* shared `TableViewColumn.ActualWidth`. Cells carry **no** `Width` binding: an explicit `Width` would defeat the panel's unconstrained measured-width measurement, so the panel arranges to the resolved width directly.

**Layout ownership — table-level.** Column-width resolution is a single **table-level** decision, computed once per layout pass. `TableView::MeasureOverride` measures the template subtree — which realizes the viewport rows and runs each `TableViewCellsPanel::MeasureOverride`, caching every cell's measured width per column — then calls `ResolveColumnWidths` exactly once: it pulls the max measured width per column across the header host and the currently realized rows and resolves Pixel/Auto/Star. Cells and rows only *measure and cache*; they never decide or push a width, so a column's state is written once per pass rather than once per realized cell. When a resolved width changes, `TableView` directly invalidates the header + realized row panels (no cross-object event), which re-arrange to the new `ActualWidth`; a stable data set settles in one extra pass. The row's cell host is always the controlled `TableViewCellsPanel`, so a custom row template can restyle cell *content* without breaking column alignment.

## Column width sizing

Column widths are resolved by the table in a single pass owned by `TableView::MeasureOverride`; the engine lives in `TableView_Layout.cpp`. Each layout pass runs a four-step pipeline and writes each column's read-only `ActualWidth`, which each cells panel reads when it arranges:

1. **Measure + cache** — `TableView::MeasureOverride` measures the template subtree; each realized `TableViewCellsPanel` (header + rows) measures its cells unconstrained and caches the measured width per column (`MeasuredWidthForColumn`). Cells do not push to the table.
2. **Resolve (once)** — `ResolveColumnWidths` pulls the max measured width per column across the header host and the realized rows, then turns `Width`/`MinWidth`/`MaxWidth` plus the body viewport into `ActualWidth`: `Pixel` → the given pixels; `Auto` → the pulled max (grow-only, see below); `*` → a proportional share of the viewport left after the fixed columns, with min/max clamping and re-division (the WPF `ComputeStarColumnWidths` shape).
3. **Share** — the resolved `ActualWidth` on each `TableViewColumn` is read by every header/row `TableViewCellsPanel` when it arranges; if a width changed, `TableView` directly invalidates those panels so they re-arrange atomically.
4. **Arrange** — each `TableViewCellsPanel.ArrangeOverride` places its cells left-to-right at those widths (leading frozen cells are then pinned to the horizontal scroll offset).

Only realized rows contribute, so `Auto` sizes to the widest *realized* cell. `Auto` is **grow-only** within a data set: the per-column accumulator only increases and is reset only on data-set boundaries — an `ItemsSource` change, the `Columns` collection being *replaced*, a `CellTemplate` change, or a header change. An *incremental* `Columns` add/remove does **not** reset the pre-existing columns' accumulators (their row data is unchanged, so resetting them would let an unrelated column shrink to only the currently realized rows); the newly inserted column simply starts fresh and grows from content. Density and `Min`/`MaxWidth` changes re-resolve without resetting. `*` needs a finite width to divide: the horizontally-scrolling body panel is measured at infinite width, so the engine pulls `ScrollViewer.ViewportWidth` explicitly and re-resolves on the body scroller's `SizeChanged`. `Auto` auto-shrink and column virtualization are out of scope for v1.

**Known layout limitations (v1).**
- **`*` requires a bounded viewport.** Star columns divide the body `ScrollViewer.ViewportWidth`. In a width-to-content host (the table given an infinite width constraint, e.g. inside an auto-sized `StackPanel`), there is no finite width to divide, so `*` columns stay at their provisional default width. Give the table a bounded width — or use `Pixel`/`Auto` columns — in width-to-content layouts.
- **Fixed columns wider than the viewport.** When the `Pixel` + `Auto` (fixed) columns already exceed the viewport, the space left for `*` is zero, so `*` columns collapse to their `MinWidth` and the body scrolls horizontally. This matches WPF `DataGrid`'s star-vs-fixed behavior.
- **Width-dependent cell height.** Cells are measured unconstrained (infinite width) to learn their measured width, then arranged at the resolved column width. A `CellTemplate` whose *height* depends on its *width* (a wrapping `TextBlock`, an aspect-fit image) is measured for its single-line/unwrapped height and is clipped when arranged narrower. The built-in `TableViewTextColumn` uses single-line `CharacterEllipsis`, so it is unaffected; authors of wrapping `TableViewTemplateColumn` content should set an explicit row height or avoid width-dependent wrapping in v1.

## Column resize (`ResizeGripper`)

Interactive column resize is a **policy split** between a framework-internal primitive and the table. `ResizeGripper` (`Microsoft.UI.Private.Controls`, shipped only in `Microsoft.UI.Xaml.Controls.Tabular.dll`) is a *delta source*: it owns the gesture and reports how far the user has dragged. It owns no value, no bounds and no layout, because `TableViewColumn` already owns `Width`/`MinWidth`/`MaxWidth`/`ActualWidth` — a second copy of that state is what the primitive deliberately avoids.

**The primitive owns** the hit target, its own presentation (visual states plus the `ResizeGripperSeparatorBrush`/`ResizeGripperSeparatorThickness` theme keys), the resize cursor, the input protocol, and the gesture state machine: `IsDragging`, a 0.5 DIP deadband, rejection of non-finite deltas, re-entrancy guards, and cancel semantics.

**The table owns** every policy decision: whether a column gets a gripper at all (`CanUserResizeColumns` + `TableViewColumn.CanResize`), the gripper's placement and width (`TableViewResizeGripperWidth`), the anchor captured at `DragStarted`, clamping the reported delta to `MinWidth`/`MaxWidth`, writing `Width`, reverting on cancel, and the UIA width announcement.

**Protocol.** `BeginDrag()` → zero or more `DragDelta` → `EndDrag(canceled)`. `DragDelta` carries `Delta` (since the last raise) and `TotalDelta` (from the start of the gesture), so a host applies `TotalDelta` against the anchor it captured rather than accumulating. `DragCompleted.Canceled` distinguishes a torn-down gesture (Escape, a canceled contact, the header rebuilt mid-drag) from a release: on cancel the host restores the **authored `GridLength`**, so a canceled drag cannot silently rewrite an `Auto` or `*` column as fixed pixels. `BeginDrag`/`TryDrag`/`EndDrag` are public so a host can drive the identical gesture from its own input handling — that is how keyboard resize stays indistinguishable from a pointer drag.

**Input.** Pointer input arrives as manipulation events, so pointer capture, touch and pen contacts, and multi-pointer arbitration come from the framework's gesture recognizer rather than from each host. `ManipulationMode` is a single translate axis and is set in the constructor, not the default style: a host whose style failed to load must not silently lose pointer resize. Because the gripper travels with the edge it drags, `ManipulationStarting` reparents the measurement frame — the default container is the element itself, whose motion would feed back into the gesture and stall it.

**Axis and RTL.** `DragOrientation` is the axis the drag is **measured along**, which for a divider is perpendicular to how it looks: `Horizontal` drags left/right. It is named `DragOrientation` rather than `Orientation` precisely because the primitive owns no shape — the host sizes it — so the control's own layout axis is not something it can describe. This is also the opposite of the Windows Community Toolkit's `SizerBase.Orientation`, which describes the bar; the distinct name prevents a silent mix-up. The primitive normalizes to a **logical** delta, so positive always grows in reading order and both input paths agree; only the horizontal axis mirrors under RTL.

**Keyboard.** The header cell, not the gripper, is the keyboard target — column commands belong there, and one tab stop per column would sit between the user and the data. The table therefore sets `IsTabStop(false)` on the gripper and routes Left/Right into `TryKeyboardStep(VirtualKey)`, which owns direction, the RTL mirror, `KeyboardIncrement` and the Shift multiplier so both keyboard paths cannot drift apart.

## Row virtualization

Rows are virtualized by `ItemsRepeater` on the vertical axis. With the vertical `StackLayout`, only rows whose realization rect intersects the body viewport — plus two viewports of cache on each side — are materialized as `TableViewRow` instances; off-screen rows return to the recycle pool. `StackLayout` is used instead of a uniform two-axis layout because row heights vary with density and template content.

On recycle, `ItemsRepeater` re-points a pooled row's `DataContext` to the new item and cell **data** updates **reactively** through `DataContext` inheritance + bindings — cells are **not** re-stamped imperatively (mutating a live cell during the repeater's measure pass re-enters framework layout and fails fast). `TableViewTextColumn` binds `TextBlock.Text` to the consumer `Binding` (resolved against the inherited `DataContext`); `TableViewTemplateColumn` binds its `ContentPresenter.Content` to the cell-wrapper `Border`'s inherited `DataContext` — bound to the *wrapper*, not the presenter's own `DataContext`, because `ContentPresenter` pins its `DataContext` to its `Content` (a self-referential binding would freeze after the first item). Consequently `RebuildCells` is needed only for structural changes (columns / template / density); a pure recycle refreshes only index-dependent visuals — alternating-row banding (`RefreshRowBackground`) and frozen-column pinning. Custom columns (overridable `GenerateElementCore`) must bind reactively to the inherited `DataContext` for the same reason.

Column virtualization is intentionally not implemented: each realized row renders a cell for every non-null column (collapsed columns get a hidden cell), which is the right trade for typical 5–50 column tables. Because every realized row materializes one cell wrapper per non-null column, `TableView` targets typical application tables (~5–50 columns) and is **not** designed for spreadsheet-scale column counts (100+). Column virtualization is independent of the width engine: `Auto`/`*` sizing ships in this PR (see *Column width sizing*) without column virtualization, which stays out of scope — each realized row still renders one cell per non-null column.

## Sticky headers

The C++ control drives `PART_HeaderScroller`'s horizontal offset from `PART_BodyScroller.ViewChanged`, so the header band tracks horizontal body scrolling. The sync is one-way (only the body's `ViewChanged` is handled; the header is never the source) and skips near-equal offsets (<0.5px) to avoid `ViewChanged` ping-pong.

## Frozen leading columns

Leading-frozen header and body cells are counter-translated against the horizontal scroll offset. Only the contiguous leading prefix from column 0 is pinned; a later `Leading` column is treated as non-frozen. This requires `ElementCompositionPreview.SetIsTranslationEnabled` (a `UIElement.Translation` X/Y change is otherwise a no-op). Non-frozen cells are clipped out of the pinned band so horizontally scrolled content does not draw underneath the frozen leading cells. This translation/clip approach is a v1 rendering detail; later interactive features (resize, reorder, selection) must not assume it.

# Implementation Details

The render pipeline is `TableView.ItemsSource` → `PART_RowsRepeater` → `TableViewRow` → per-column cell wrapper → `TableViewColumn.GenerateElement(dataItem)`.

`TableView` owns `PART_RowsRepeater.ItemsSource` (the template does not bind it directly). `Unloaded` clears the repeater source so queued repeater cache-build work cannot run against a detached subtree.

Cell generation is column-specific:

- `TableViewTextColumn` creates a `TextBlock` and applies `Binding`.
- `TableViewTemplateColumn` instantiates `CellTemplate`; a null template produces an empty cell.
- Custom columns override `GenerateElementCore(Object dataItem)`.

`RowBackground` and `AlternatingRowBackground` provide opt-in row banding; when both are null, rows stay unbanded. `GridLinesVisibility` controls row/cell gridline borders. `Density` selects row minimum height and built-in cell/header padding for `Compact`, `Standard`, or `Comfortable`.

Keyboard handling is row-oriented. `TableView` listens to bubbling `KeyDown` so template-column descendants handle input first; unhandled `Up`, `Down`, `Home`, `End`, `PageUp`, and `PageDown` move focus between rows. The internal `GridCoordinateHelper` performs the row/column ↔ flat-index math, wrap behavior, and overflow guards.

Accessibility exposes a read-only UIA grid/table model:

- `TableViewAutomationPeer`: `IGridProvider`, `ITableProvider`, `IItemContainerProvider`
- `TableViewRowAutomationPeer`: `DataItem` control type, no grid/table provider (exposes its cell peers as children)
- `TableViewCellAutomationPeer`: `IGridItemProvider`, `ITableItemProvider`
- `TableViewColumnHeaderAutomationPeer`: column-header name/bounds

Lifetime rules: columns and rows use weak owner back-pointers (`GetOwningTableView()` resolves a strong owner for synchronous work); runtime classes use `ReferenceTracker` where required; cross-object events use `auto_revoke`; recycled rows reset transient visual state before reuse.

Theme values resolve through `TabularSurfaces` resources and re-resolve across theme (and high-contrast) changes. Dark `TabularSurfaceGridLineBrush` is `#29FFFFFF`; the C++ fallback uses the same 16% white.

## Sort ownership and reconciliation

Sort has two front-ends and exactly one axis is ever in force; they reconcile rather than stack.

- `TableView.SortByColumn` (and header click) declares the control's own axis and publishes
  `TableViewColumn.SortDirection`, which is what draws the header chevron.
- `TableViewSource.Sort` declares an axis on the source. The path overload
  (`Sort(sortMemberPath, direction)`) names a property, so the control can match it against a
  column's `SortMemberPath` and light that column's chevron. The delegate overload
  (`Sort(keySelector, direction)`) is opaque — the key may not correspond to a column at all
  (computed key, multi-field, custom comparer) — so no chevron is shown.

Reconciliation rules:

- Sorting through the control clears any axis the app declared on the source.
- Declaring a sort on the source clears the control's axis. A path-declared axis that matches a
  column lights that column and raises `Sorted` with it; otherwise every `SortDirection` is
  cleared and `Sorted` carries a null column.
- `ClearSort()` clears every axis, including one the app declared.

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

Only **row** identity degrades, and the asymmetry is deliberate: a row identity the engine cannot
derive is a property of the app's data that the app may not control, and the `Unshaped` mirror
still shows every row; a bad *group* identity comes from the `GroupBy(...)` selector the app just
wrote, and an ungrouped table is an outcome it would not notice.

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
restored by identity too (`CaptureGroupHeaderFocusForRestore` /
`RestoreGroupHeaderFocusIfPending`), so a keyboard user does not lose focus to the top of the
table on every collapse.

**Header presentation.** `TableViewGroupHeader` is a templated `ContentControl`, not a code-built
visual tree: the chevron gutter (`PART_ExpanderGutter` / `PART_ExpanderIcon`) and the themed band
live in the control-owned `ControlTemplate`, while `TableView.GroupHeaderTemplate` fills only the
content region — so an app template cannot accidentally drop the expander. Its `Content` is a
`TableViewGroupInfo` projection that is updated **in place** and raises `PropertyChanged`, so
recycling a header does not re-evaluate every binding in the template. `KeyText`/`ItemCountText`
are computed lazily through a `DecimalFormatter`, so a template that binds only `Key`/`ItemCount`
pays nothing for them.

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
public `Control`; cell wrappers are `Border`s; header cells are `Grid`s), so the additions below
need no new base API or re-plumbing.

**v1 (this base):**
- `RowBackground` / `AlternatingRowBackground` — opt-in row banding (both null = unbanded, WPF
  `DataGrid` parity; `AlternatingRowBackground` overrides odd rows only when set).
- `GridLinesVisibility` — row/cell gridline borders.
- `Density` — row min-height + built-in cell/header padding preset.
- `TabularSurfaces` theme resources — the theme-aware (light/dark/high-contrast) source of the
  default brushes, re-resolved on theme and high-contrast changes.

**Rich styling (later):** delivered as **per-element `Style` dependency properties**, matching WPF
`DataGrid` (`RowStyle` / `CellStyle` / `ColumnHeaderStyle`) — **not** a monolithic `TableStyle`.
`RowStyle` targets the realized `TableViewRow`; `CellStyle` the per-cell container (the cell-wrapper
`Border`; cell *content* is customized via the column's template / `GenerateElementCore`, matching
WPF where `CellStyle` targets `DataGridCell`, not the content); `ColumnHeaderStyle` the generated
header cell. They form a cohesive set and land together, since shipping one alone would leave an
asymmetric styling surface.

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

**`TableViewTextColumn`** — `TableViewColumn` that generates text cells. `Binding` is a plain CLR property. Its editing element is a `TextBox` bound with a clone of `Binding` forced to `TwoWay` + `UpdateSourceTrigger=Explicit`.

**`TableViewTemplateColumn`** — `TableViewColumn` that generates templated cells. DPs `CellTemplate` and `CellEditingTemplate`.

**`TableViewRow`** — `Control` representing one realized item row. `GetOwningTableView()`.

**`TableViewGroupHeader`** — `ContentControl` representing one realized group-header band under a
grouped source. DPs `IsExpanded`, `IsExpandable`; event `ToggleRequested`
(`TableViewGroupHeaderToggleRequestedEventArgs.GroupKey`). Template parts `PART_ExpanderGutter`,
`PART_ExpanderIcon`; visual states `CommonStates`, `ExpansionStates`, `ExpandabilityStates`.

**`TableViewGroupInfo`** — read-only `INotifyPropertyChanged` projection bound by a
`GroupHeaderTemplate`: `Key`, `ItemCount`, `Level`, `IsExpandable`, `IsExpanded`, and the
culture-formatted `KeyText` / `ItemCountText`. Updated in place across recycling.

**Automation peers** — `TableViewAutomationPeer`, `TableViewRowAutomationPeer`, `TableViewCellAutomationPeer`, `TableViewColumnHeaderAutomationPeer`, `TableViewGroupHeaderAutomationPeer` (providers: `IGridProvider`, `ITableProvider`, `IGridItemProvider`, `ITableItemProvider`, `IItemContainerProvider`, `IExpandCollapseProvider`).

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

The same handler is the only place a pointer establishes the **current cell**, which is also what makes keyboard editing reachable: pointer focus lands on the row, not on a tagged cell, so without it the current column stays null and the no-argument `BeginEdit()` silently fails.

Cell resolution walks up from `OriginalSource` to the cell wrapper `Border` (whose `Tag` carries the column) and on to the `TableViewRow`. When the walk cannot reach a wrapper — which happens once the row has focus and the press arrives with the row itself as source — it falls back to hit-testing at the pointer position.

## Value transfer and rollback

Editing bindings use `UpdateSourceTrigger=Explicit`, so the typed value lives in the editor until the control pushes it. That is what lets a cancel restore something and a validation failure hold the edit open. On commit the control calls `UpdateSource()` on the binding expressions discovered from the editing element; validation runs **after** the write, and a blocked commit undoes it.

Rollback prefers `ITableViewEditableItem` on the data item. Without it the control snapshots the value at the column's `GetEditingPropertyPathCore()` path, falling back to scraping the editor's bindings — inherently incomplete for an arbitrary editor, so the inability to capture anything is recorded and reported rather than silently ignored.

## Re-entrancy rules (load-bearing)

Row recycling, cell rebuild and dependency-property change callbacks can all run inside a layout pass or a framework callback. Mutating focus from there re-enters XAML and trips the re-entrancy assertion (`0xc0000420`). Therefore:

- `TableViewRow::EndCellEdit` (restores the display child **and** moves focus) is only used on the interactive close paths.
- `TableViewRow::AbandonCellEdit` restores the display child but never touches focus, and is what `TableView::TerminateEditWithoutVisualRestore` drives from `OnRowElementClearing`, `RebuildCells`, `OnIsReadOnlyPropertyChanged` and `OnItemsSourcePropertyChanged`.
- The cell **restamp** fast path deliberately does *not* tear the edit down — it reuses the existing cells, and closing the edit there would make it impossible to keep an editor open through the re-measure that installing the editor itself provokes. Only the destructive rebuild path closes it.
- The focus-loss commit is posted to the dispatcher and re-evaluated once focus settles; deciding synchronously closes the edit inside the very gesture that opened it.

## Key internal components

- **`PART_HeaderScroller`** — horizontal-only header scroller synchronized with the body.
- **`PART_HeaderHost`** — `TableViewCellsPanel` header host rebuilt from the non-null columns.
- **`PART_BodyScroller`** — body scroll presenter and source of horizontal offset.
- **`PART_RowsRepeater`** — vertical row realization surface.
- **`PART_CellsHost`** — row-local `TableViewCellsPanel` cell host.
- **`TableViewCellsPanel`** — the custom `Panel` used for `PART_HeaderHost` and each row's `PART_CellsHost`; its `MeasureOverride` reports each cell's measured width and its `ArrangeOverride` places cells at the column's resolved `ActualWidth` (see *Column alignment*).
- **Cell wrappers** — per-row/per-column `Border` elements that host generated cell content, carry gridline/background visuals, and are arranged by the hosting `TableViewCellsPanel` at the owning column's `ActualWidth`.
- **Frozen-column translation/clipping layer** — pins leading columns while clipping non-frozen content out of the pinned band.
- **`GridCoordinateHelper`** — internal C++ helper for row/column ↔ flat-index and focus-navigation math used by keyboard navigation; not projected (no public runtimeclass).
- **`TableViewRowTemplateSelector`** — picks the container kind for an entry in the projection: a `TableViewGroupHeader` for a group entry, a `TableViewRow` for a data row.
- **`RowExpansionModel`** — internal layer-1 store of expand/collapse *intent*, keyed by group identity and holding only the exceptions to a default. Carries no XAML or tabular vocabulary, so hierarchy can reuse it unchanged.

# Appendix

## Non-goals

`TableView` v1 does not attempt to replace `DataGrid`. The true v1 non-goals are marquee selection, multi-column sort, column virtualization, row headers, hierarchy deeper than two levels, and spreadsheet-like interaction. (Selection, single-column sort, filtering, grouping, and two-level hierarchy are v1 features, not non-goals; selection, sort, filtering and single-level grouping are delivered, and two-level hierarchy lands later. Cell editing is delivered by this PR.)

## Out of Scope

Delivered later in the v1 stack: two-level hierarchy · column reorder · navigation-state persistence · samples · tests · theme-XBF emission.

Already delivered in the v1 stack: row selection · shaping primitives · filtering · single-column sort · single-level grouping.

Out of v1 / follow-up work (see Non-goals): `Auto` auto-shrink (v1 widths grow monotonically) · trailing frozen columns · multi-column sort · column virtualization · row headers · clipboard · incremental loading · multi-cell row edit transactions · editing a11y announcements.

## Accessibility notes

The UI Automation grid/table provider counts and exposes only **visible** columns: a column with `Visibility.Collapsed` is excluded from the UIA column model, so `ColumnCount`, `GetItem`, the column-header set, and every cell/header column index (`GridItem`/`TableItem` `Column`, `PositionInSet`/`SizeOfSet`) are on a visible-column basis. Consequently, toggling a column's `Visibility` changes the reported UIA column geometry (indices to the right shift). This intentionally diverges from WPF's `DataGridAutomationPeer` (which indexes the full `Columns` collection) so that a collapsed, zero-width column is never surfaced as an addressable off-screen cell. The visual layer still generates a hidden cell per collapsed column (see *Row*), but the automation tree does not expose it.
