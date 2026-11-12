# TableView — Functional Spec

> Summarizes the engineering-relevant requirements. Business and compete analysis is not
> reproduced here; the internal planning documents that own it are tracked by the feature team
> and are deliberately not linked from this repository.

## Purpose

`TableView` is a lightweight, production-grade tabular control for WinUI 3 that modernizes
Windows Shell surfaces (DUI → WinUI 3) and targets parity with **Task Manager** and
**File Explorer** details view. It is a **read-only-first** presentation control — **not** a
full `DataGrid` replacement.

## Goals

- Row/column layout over an `ObservableCollection`-backed model with live, incremental
  updates (add/remove/update) without a full re-render.
- Row virtualization with smooth scrolling: **≥30 FPS at ~500 rows**, low input latency.
- Accessibility: UIA Grid/Table/Selection patterns + Narrator, keyboard parity.
- Light / Dark / High Contrast theming.

## Delivery across the 4-PR stack

| PR | Delivers |
|---|---|
| PR 1 | Empty `Microsoft.UI.Xaml.Controls.Tabular.dll` scaffolding |
| **PR 2 (this)** | Display-only baseline: columns/cells/headers, gridlines, density, row virtualization, leading-frozen columns, keyboard row-focus nav, read-only UIA peers |
| PR 3 | Selection, single-column sort, filtering, grouping, hierarchical rows, column resize/reorder, nav-state; themed rendering. Shipped incrementally: selection, then the shaping engine with filter + sort, then grouping. |
| PR 4 | Tests + `TableViewSamples` e2e app |

## Functional requirements

Each item notes the MLP/v1 vs deferred status and the delivering PR.

### Core control & data model
- Explicit column model (no auto-generation). — **PR2**
- `ObservableCollection`-backed `ItemsSource`; incremental add/remove/update. — **PR2** render · **PR3** shaped (sort/group)
- Read-only: cells are display-only and there is no edit mode in V1 (see [Editing](#editing)). — **PR2**
- Custom cell templates (`TableViewTemplateColumn`). — **PR2**
- Grouped/banded sections, declared on the data source as `TableViewSource.GroupBy(keySelector)` rather than as a separate `GroupedItemsSource` property. — **PR3, shipped**

### Column capabilities
- Width + `MinWidth`/`MaxWidth` (`Pixel` exact; `Auto` grows to the widest realized cell within a data set; `*` shares the viewport). — **PR2** · UI resize **PR3**
- Reorder (drag + `MoveColumn`). — **PR3**
- Show/hide via `Visibility`. — **PR2**
- Sticky (always-visible) header. — **PR2**
- Single-column sort (`SortByColumn` / header invoke). — **PR3**
- Gridlines via `GridLinesVisibility` (theme-resource driven). — **PR2**
- Leading-prefix frozen columns; trailing reserved. — **PR2** (leading) · trailing deferred
- Multi-column sort. — *deferred (P2 / vNext)*

### Row & hierarchy
- Single + multi selection (Ctrl/Shift). — **PR3**
- Per-row/cell context menus, column-header menus, and table fallback. See [Context menus](#context-menus). — **implemented**
- Checkbox selection (File-Explorer-specific). — **PR3** / surface
- Single-level grouping with expand/collapse group headers. — **PR3, shipped** (see [Grouping](#grouping))
- Hierarchical rows from a flat source via `TableViewSource.ParentBy(keySelector, parentKeySelector)` / `ClearParentBy()`, no depth limit; row expand/collapse, `TableView.ExpandAllRows()` / `CollapseAllRows()`; per-row programmatic expand/collapse is out of scope for v1. See [Hierarchical data rows design](Hierarchical-Data-Rows-Parent-Key-Design.md). — **PR3**
- Opt-in live shaping (`TableViewSource.IsLiveShaping`): reshapes when an item's `PropertyChanged` moves a sort, filter, group or parent value. — **PR3**
- Row drag-drop, marquee selection. — *out of scope*

### Grouping

Grouping is declared on the data source, not on the control: the control renders whatever shape
the source publishes, so the same engine serves an app that never touches `TableView`.

- Single-level grouping via `TableViewSource.GroupBy(keySelector)`; `ClearGroupBy()` removes it.
  A second `GroupBy` replaces the first — grouping axes never stack. — **shipped**
- The group key is any object. Its **identity** is what buckets rows, and it must be a stable,
  non-empty string. Value-typed keys (`String`, `Int32`, `Int64`, `Guid`, `Boolean`, enums) get a
  built-in identity; a reference-typed key needs the
  `GroupBy(keySelector, groupIdentitySelector)` overload. — **shipped**
- An unresolvable or colliding group identity **fails fast** (`hresult_invalid_argument` on the
  call that builds the projection). It does not silently fall back to an ungrouped table: a
  grouping request that quietly did nothing is the harder bug to find. A `null` key and an
  empty-string key are both unresolvable — `String` is a supported key type, but `""` is not a
  usable identity — so an app grouping on a nullable or blank-able property supplies its own
  fallback label. — **shipped**
- Composition with the other shaping verbs is order-sensitive and deliberate: a sort declared
  **before** `GroupBy` orders the groups themselves; a sort declared **after** `GroupBy` orders
  rows **within** each group. A filter always runs first, and a group whose last row is filtered
  out disappears. — **shipped**
- Every group header is expand/collapse-capable: click or tap the band, or use the
  `ExpandCollapse` UIA pattern. `TableView.ExpandAllGroups()` / `CollapseAllGroups()` are the
  bulk programmatic counterparts and are no-ops when the source is not grouped. — **shipped**
- Expansion is **intent keyed by group identity**, not state on a group object, so a collapse
  survives a re-sort, a re-filter and a regroup that re-mints every group. Newly arriving groups
  inherit the current default rather than an intent nobody expressed. — **shipped**
- Header presentation: `TableView.GroupHeaderTemplate` fills the content region of the
  control-owned `TableViewGroupHeader`, which keeps the chevron and the themed band. The template
  binds against a `TableViewGroupInfo` projection (`Key`, `ItemCount`, `Level`, `IsExpandable`,
  `IsExpanded`, and the culture-formatted `KeyText` / `ItemCountText`). — **shipped**
- Accessibility: `TableViewGroupHeaderAutomationPeer` exposes `ExpandCollapse` and `GridItem`; a
  non-expandable group reports `LeafNode` rather than dropping the pattern. — **shipped**
- Per-group programmatic expand/collapse (by key, from app code), multi-level grouping, and
  group-level aggregates/summaries. — *deferred*
- Incremental (non-rebuild) maintenance of a grouped projection: a source change under grouping
  currently rebuilds rather than splicing. Correct, but O(N) per change. — *deferred*

### Editing

Deferred to V2 / future DataGrid. TableView V1 has no editing API and no edit mode: no
`IsReadOnly`, `IsEditing`, `CommitEdit`/`CancelEdit`, `BeginningEdit`/`CellEditEnding`/
`CellEditEnded` (and their args and `TableViewEditAction`), `CellEditingTemplate` or
`CommitCellEditCore`, and no built-in `TextBox` editor in `TableViewTextColumn`. The named
scenarios need at most an app-owned rename of one field, which composes from a
`TableViewTemplateColumn` and app code; see
[Rename-in-place guidance](../../api-specs/TableView/TableView-spec.md#rename-in-place-guidance).

- Live controls inside a `CellTemplate` (check box, picker, combo box) are supported and are
  bound by the app. — **v1**
- Cell editing: gestures (double-click / double-tap, `F2`, `Enter`, `Esc`, focus-loss commit),
  a programmatic `BeginEdit` and current-cell surface, vetoable and async-deferrable edit events,
  `INotifyDataErrorInfo` validation, buffered rollback, per-row editors, row transactions, and
  editing a11y announcements. — *deferred to V2 / future DataGrid*

### Virtualization & performance
- Row virtualization (realize only visible rows + cache). — **PR2**
- ≥30 FPS smooth scroll at 400–500 rows; high-frequency updates (Task Manager metrics); low latency. — **PR2/PR3**
- Column virtualization intentionally omitted (typical ~5–50 columns).

### Styling & theming
- Light / Dark / High Contrast theme tokens + inline fallbacks (`DividerStrokeColorDefault` gridlines, `CardStrokeColorDefault` outer border). — **PR2** (full self-themed Theme-XBF emission deferred)
- Cell-level styling: custom cells via `TableViewTemplateColumn` + built-in text-cell defaults (left, vertically centered). A public per-column alignment/weight API — **deferred (PR3+)**.

### Tooltips
- Cell: opt-in via `TableViewColumn.CellToolTipBinding`; the binding is evaluated against each row's data item and its value is the tooltip content. No binding means no tooltip and no per-cell cost. Needed because text cells render with `CharacterEllipsis` and no wrapping, so over-wide values are otherwise unreadable. — **PR3**
- Author precedence: a tooltip set inside a cell's own content template opens over that content; the control's tooltip covers the rest of the cell, and the control never touches a tooltip it did not attach.
- Content: a string, or any content a `ToolTip` can host. A `UIElement` is parented by that cell's `ToolTip`, so a converter returns a fresh element per evaluation. Computed content is authored with an `IValueConverter`.
- Accessibility: string tooltip text is published as the cell's `AutomationProperties.HelpText`, and `TableViewCellAutomationPeer` suppresses it at UIA query time when it merely repeats the cell's own text, so the value is not announced twice. Suppression is gated on the ownership record, so text the app set is never dropped.
- Recycling: a recycled row never shows a previous item's cell tooltip. No invalidation API is needed — the binding tracks the row's `DataContext`, so a recycled row re-resolves through the same inheritance that refreshes its cell text, and a source `PropertyChanged` updates a live tooltip in place.
- Because the control never calls into app code while realizing a cell, there is no re-entrancy surface, no drain budget, and no coalescing machinery.
- Column header: opt-in via `TableViewColumn.HeaderToolTip`, whose value is the tooltip content. Covers the whole header cell, including its padding and sort affordance, except the resize gripper: while the mouse is on the gripper the tooltip is suppressed for all input (pointer and keyboard focus alike), so the resize cursor is not reset under a resting pointer. It returns when the pointer leaves the gripper and shows on the next entry into the header. Header cells are rebuilt rather than recycled, so the value is read when the header is built and re-applied in place when it changes — no binding, no invalidation. String content is reported as the header's UIA help text by `TableViewColumnHeaderAutomationPeer`, joined with the sort state when the column has one; the header peer is virtual, so `AutomationProperties.HelpText` on the element would never reach a client. — **PR4**
- Group-header tooltips are **deferred**. Grouping ships, but the group header has no tooltip opt-in: `TableViewColumn.CellToolTipBinding` and `HeaderToolTip` are column-scoped, and the group header is a `TableViewGroupHeader` whose content is app-templated, so an app that wants a tooltip there puts one in its `GroupHeaderTemplate` today. A control-owned group-header tooltip API needs its own opt-in property and is not in this release.

> Tooltips are **not** gated on text truncation. No WinUI control keys tooltips off `IsTextTrimmed`; the shipped pattern is to gate on a cheap content predicate (non-empty string) or an explicit opt-in.

### Context menus

- Opt-in column properties: `TableViewColumn.CellContextFlyout` and `HeaderContextFlyout` are nullable `FlyoutBase` dependency properties, read on each request. The cell flyout is shared across that column's data cells; it never applies to group headers. — **implemented**
- Cell precedence: application flyout inside the cell content, column `CellContextFlyout`, application row-template/row flyout on the ancestor path, then inherited `TableView.ContextFlyout`. Header precedence: application flyout inside header content, column `HeaderContextFlyout`, then table fallback. Content search excludes nested items hosts' items and nested TableViews, and discovers text-control flyouts only when locally set rather than built-in or style-set.
- Row-only requests resolve menus along the ancestor path from the cells host through the template root and row, then table fallback, in addition to native template routing; lookup does not descend into arbitrary cells and column menus do not apply. Group headers use their own/style `ContextFlyout`, then table fallback. Empty table space uses table fallback.
- Data context: cell menu items inherit the requested row's item, not `SelectedItem` or the column object. Header menus do not automatically receive a row item or column object. Group-header menus inherit `TableViewGroupInfo`; a header reused for another group gets a fresh projection, preserving the original identity in already-open menus. Same-group metadata updates remain observable in place. Table fallback uses the table's data context. Explicit application binding sources retain precedence.
- Input and placement: right-click and press-and-hold use pointer position; Shift+F10, Menu key, and UIA `ShowContextMenu` use the focused element or peer target. Column menus target the cell/header wrapper; content menus retain their owning element as target. A row-owned or table-fallback menu resolved for a non-pointer cell/header request uses the requested cell bounds for placement while keeping its owner as target.
- Focus and selection: TableView-routed requests focus the target cell, row, or header without changing selection; a body-cell request also sets the current cell. This remains intentional when no menu resolves: focus follows interaction, not menu availability. Normal focus-driven bring-into-view behavior remains applicable to partially visible cells. An unhandled mouse/pen right-button press reaching a data row establishes cell focus/current-cell state before a native template menu opens, without handling the press or arming selection. This respects nested-table ownership and child-handled presses, and preserves focus already inside the target cell. Other native template-menu input can still bypass the context-request focus move; touch/hold, keyboard, and UIA behavior are unchanged.
- Virtualization: unavailable pooled group-header peers reject `ShowContextMenu` with `ElementNotAvailable`. Open group menus retain their original group projection rather than retargeting when a header is recycled.
- Full resolution, binding, and targeting: [API specification: Context menus](../../api-specs/TableView/TableView-spec.md#context-menus).

### Accessibility (UIA)
- Grid/Table peers; Row peer (`SelectionItem` + `GridItem`); ColumnHeader peer (`Invoke` → sort). — **PR2** read-only Grid/Table · **PR3** Selection + sort invoke
- Narrator, keyboard navigation parity, sort announcements. — **PR2** nav · **PR3** sort

### Keyboard focus and accessibility model

The whole `TableView` is one tab stop: the column header row. `Tab` and `Shift+Tab` land on the
header in the current column, else the first actionable one; the next `Tab` leaves the table.
`Down` moves into the rows (the remembered row if in view, else the selected row, else the first row
in view; a row selected after the remembered one wins), and `Up` from the first row returns to the
header row. When no header is actionable or headers are hidden, the rows are the stop, entered in
the same order; an empty table's `EmptyTemplate` content is the stop instead (its last control for
`Shift+Tab`).

Takes keyboard focus:

- Actionable headers: sort (with a sort key), resize (while the column can actually be resized),
  context menu, tooltip, or interactive content.
- Rows.
- Actionable cells: interactive content (a control, an inline `Hyperlink`, or a nested
  `TableView`), context menu, or tooltip. A plain text cell is not actionable.

Nothing else: `Focus()` on an inactive header or cell does not move focus there, and UIA `SetFocus`
fails with `UIA_E_INVALIDOPERATION`.

Controls in a cell: `Enter` moves into them, `Esc` returns to the cell.

Screen readers still reach inactive headers and cells: they stay in the UIA grid (`GridItem`,
`TableItem`, header association), so the row name, scan mode, and Narrator table navigation
(`Ctrl+Alt+Arrow`) read them.

Why: Windows guidance asks for one tab stop per collection and tab stops only for actionable items
([Keyboard interactions](https://learn.microsoft.com/windows/apps/design/input/keyboard-interactions)),
and APG lets header cells that don't sort or filter stay unfocusable
([APG grid pattern](https://www.w3.org/WAI/ARIA/apg/patterns/grid/)). Making inactive data cells
unfocusable deliberately departs from APG's focusable-cell advice; screen readers reach them
through table navigation instead.
### Reliability & servicing
- Deterministic behavior, unit-test coverage (**PR4**), SFI/security compliance, post-adoption API stability.

## References

Repo:

- Design (layout + implementation): [`TableView-dev-spec.md`](./TableView-dev-spec.md)
- API surface + samples: [`../../api-specs/TableView/TableView-spec.md`](../../api-specs/TableView/TableView-spec.md)

Planning material that is not in this repository — the v1 scope and API-parity document, the
functional 1-pager, and the MLP requirements matrix — is owned by the feature team. Everything
those documents say that constrains the implementation is restated here, so this file is the
working contract for anyone building or reviewing the control.
