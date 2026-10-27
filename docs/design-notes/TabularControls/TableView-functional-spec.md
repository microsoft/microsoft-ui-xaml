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
- Read-only by default (`IsReadOnly` = `true`). — **PR2**
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
- Per-row/cell context menu; checkbox selection (File-Explorer-specific). — **PR3** / surface
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
- Opt-in cell editing (`IsReadOnly = false` on the control; per-column `IsReadOnly`). — **cell editing**
- Editors supplied by the column: `TableViewTextColumn` produces a `TextBox`; `TableViewTemplateColumn` uses `CellEditingTemplate`. — **cell editing**
- Gestures: double-click / double-tap and `F2` begin an edit; `Enter` commits; `Esc` cancels; moving focus off the cell commits. — **cell editing**
- Programmatic API: `CommitEdit()` and `CancelEdit()` close the open cell edit and return `false` when it stays open (veto or failed validation); read-only `IsEditing` reports whether a cell editor is open. Editing **starts only from user gestures**: there is no public `BeginEdit` and no public current-cell surface (`CurrentItem` / `CurrentColumn` / `SetCurrentCell`) in this release. — **cell editing** · programmatic begin + current cell *deferred* (land together when a scenario such as a context menu or view-model command needs them)
- Vetoable lifecycle events `BeginningEdit` and `CellEditEnding`. Both are synchronous: a handler sets `Cancel` before it returns, and there is no `GetDeferral()`. `CellEditEnding` reports the outcome through `EditAction` and is raised for every close, so there is no separate post-close event. — **cell editing** · `RowEditEnding`, `EditEnded` and async deferrals *deferred*
- Rollback: the built-in text editor uses `UpdateSourceTrigger=Explicit`, so a cancelled edit never reaches the data item; application editing templates must buffer the same way. — **cell editing** · item-owned transactions (`ITableViewEditableItem`, the WinRT counterpart of `IEditableObject`) *deferred* with row editing
- Validation: `INotifyDataErrorInfo` on the data item, scoped to the property the column edits, blocks a commit. — **cell editing**
- Multi-cell row transactions (`CancelEdit(Row)` rolling back siblings already committed). — *deferred*
- Editing a11y announcements (live-region/UIA notification on begin/commit/cancel). — *deferred, needs localized strings*

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
- Column header: opt-in via `TableViewColumn.HeaderToolTip`, whose value is the tooltip content. Covers the whole header cell, including its padding and sort affordance. Header cells are rebuilt rather than recycled, so the value is read when the header is built and re-applied in place when it changes — no binding, no invalidation. String content is reported as the header's UIA help text by `TableViewColumnHeaderAutomationPeer`, joined with the sort state on sortable columns, and dropped when it only repeats the header's name. A realized header owns its `HeaderItem` peer; when there is neither sort state nor tooltip text, the peer falls back to the base `FrameworkElementAutomationPeer` help text, which is the header element's `AutomationProperties.HelpText`. For a `TableView`-owned peer (the one `GetColumnHeaders` returns before the header is realized, or any peer built with the public constructor, which stays `TableView`-owned even after realization) that fallback returns the **TableView's** `AutomationProperties.HelpText` instead (a known bug). **planned** An app-set header `AutomationProperties.HelpText` is never lost: the peer joins, in order, the app's `HelpText`, the tooltip text, and the sort state. — **PR4**
- Group-header tooltips are **deferred**. Grouping ships, but the group header has no tooltip opt-in: `TableViewColumn.CellToolTipBinding` and `HeaderToolTip` are column-scoped, and the group header is a `TableViewGroupHeader` whose content is app-templated. A tooltip placed inside `GroupHeaderTemplate` opens only on hover over (or focus of) that inner element, not when the group header takes keyboard focus. To get keyboard parity, set `ToolTipService.ToolTip` on the `TableViewGroupHeader` itself through an implicit `TableViewGroupHeader` style (there is no `GroupHeaderStyle` property); the group header is the focus target, so that tooltip also opens on keyboard focus. A style setter cannot bind to the group, so this gives every group header the same static text; per-group content has no supported keyboard-accessible path yet (see the open question under [Keyboard focus and accessibility model](#open-questions)). A control-owned group-header tooltip API needs its own opt-in property and is not in this release.

> Tooltips are **not** gated on text truncation. No WinUI control keys tooltips off `IsTextTrimmed`; the shipped pattern is to gate on a cheap content predicate (non-empty string) or an explicit opt-in.

Keyboard access to tooltips is specified under
[Keyboard access to tooltips](#keyboard-access-to-tooltips).

### Accessibility (UIA)
- Grid/Table peers; Row peer (`SelectionItem` while selectable, `ExpandCollapse` on hierarchical rows); Cell peer (`GridItem` + `TableItem`); ColumnHeader peer (`Invoke` → sort, `Transform` → resize). — **PR2** read-only Grid/Table · **PR3** Selection + sort invoke
- Narrator, keyboard navigation parity, sort announcements. — **PR2** nav · **PR3** sort
- Keyboard focus and screen-reader model: see [Keyboard focus and accessibility model](#keyboard-focus-and-accessibility-model)
  below. It states what ships today, what is planned, and the standards behind each choice. The
  key-by-key routing that ships today is in the dev spec, under *Implementation Details*.

### Keyboard focus and accessibility model

This section is the contract for how keyboard and assistive-technology users move through a
`TableView`, and why. Each rule is tagged **current** (ships today) or **planned** (agreed, not yet
implemented). [Current vs. planned](#current-vs-planned) lists every difference in one place.

The model in one sentence: **the table is one tab stop, arrow keys move inside it, and only things
that do something take keyboard focus**. Column headers with nothing to do are labels, not
controls. They stay in the UIA tree so screen readers can announce them, but they are not focus
targets.

#### Tab: one tab stop for the whole table

- **planned** The whole `TableView` is **one** tab stop (roving focus). `Tab` and `Shift+Tab` enter
  it, and from navigation mode the next `Tab` or `Shift+Tab` leaves it. `Tab` never moves between
  headers, rows or cells.
- **planned** Entering the table with `Tab` or `Shift+Tab` lands on the first of these that exists:
  1. the row that last had focus if it is realized and enabled, or the cell in the remembered
     column of that row when focus left at cell level;
  2. the selected row, if it is in the viewport;
  3. the first enabled row in the viewport (the last one for `Shift+Tab`).

  "Row" here includes a group header, by design rather than by accident: when the entry order
  resolves to a group header (for example the first body element in the viewport), entry lands on
  it at row level. Entry never lands on a group header outside the viewport.
- **planned** When the body has **no** focusable row (empty, everything filtered out, or every row
  disabled), the table's tab stop is the remembered actionable header, else the first actionable
  header. Without this, headers would be unreachable, because the only path to them is `Up` from
  the first row. When there is neither a focusable row nor an actionable header, the table is not a
  tab stop.
- **planned** Otherwise `Tab` never lands on a column header, and never on a control inside a cell
  template. Focus that leaves the table from a header returns to the remembered row or cell on the
  next `Tab`.
- **current** The table has **two** tab stops: the header band, then the body. `TableView` sets
  `TabFocusNavigation="Once"` on `PART_HeaderHost` in `OnApplyTemplate`; the `ItemsRepeater` that
  hosts the rows scopes the body the same way.
- **current** Body entry from outside the table depends on what XAML's tab walk aims at. When it
  aims at a data row, the row redirects entry to the data row that last had focus (a group
  header is never remembered) if it is realized and enabled (or to the cell in the remembered column when focus left at cell level), otherwise to the
  first (`Tab`) or last (`Shift+Tab`) enabled data row in the viewport. When it aims at a group
  header, entry is **not** redirected and lands on that group header, which can be outside the
  viewport (for example `Shift+Tab` from after a grouped table, or `Tab` into a grouped table whose
  column headers are hidden). `Tab` from the header band lands on the first enabled row or group
  header in the viewport (a group header is entered at row level); at cell level, on the cell in
  the current column of that row. Entry never
  consults the selection.
- **current** Cell content is never a tab stop from row or cell level. The row takes authored
  content out of the tab walk for one `Tab` press, so a `ComboBox` or `Button` in a cell is not a
  `Tab` target.

Why: the table is one composite control. Windows guidance asks that a collection of related items
be a single tab stop with arrow-key "inner navigation", and APG's grid pattern puts only one
element of a grid in the tab sequence. This is a deliberate trade-off. Windows guidance also says
actionable items should be tab stops, and an actionable header is actionable. The planned model
reaches actionable headers with `Up` from the first row rather than giving them a second tab stop,
so every `Tab` press past the table costs one stop, not two. The alternative is listed under
[Open questions](#open-questions).

#### Column headers: focusable only when actionable

A header is **actionable** when at least one of these is true. Every input is static
configuration, so a header does not flicker in and out of the focus order while a sort is
temporarily refused.

- **Sortable**: all of
  - `TableView.CanUserSortColumns` is `true`;
  - `TableViewColumn.CanSort` is `true`;
  - the column has a sort key: `CustomSortComparer`, or a non-empty `GetSortMemberPathCore()`
    (which returns `SortMemberPath`, or the `Binding` path on `TableViewTextColumn`).

  This is the sort engine's static column check (`CanSort` plus a sort key) plus the user-sort
  switch. It leaves out the checks that depend on the data source at the moment of sorting: whether
  the source is owned by another `TableView`, whether a `TableViewSource` can apply a sort right
  now, and whether a newly assigned source is still pending. `Enter`, `Space` and UIA `Invoke`
  make those checks when they run.

  **current** UIA `Invoke` first fails with `UIA_E_ELEMENTNOTENABLED` when the header is disabled,
  then with `UIA_E_INVALIDOPERATION` when the header does not pass its own sortable check (`CanUserSortColumns`, `CanSort`, visible, table loaded). Past that check it
  calls the sort and ignores the result, so `Invoke` (like `Enter` and `Space`) returns
  successfully without sorting when:
  - the column has no sort key;
  - a newly assigned source is still pending;
  - the source refuses (owned by another `TableView`, or a `TableViewSource` that cannot apply a
    sort now). This applies only to a step to `Ascending` or `Descending`; a step back to
    unsorted (`None`) is never refused by the source;
  - an open cell edit cannot be committed (validation or a vetoed `CellEditEnding`), so the sort
    is dropped;
  - a cell edit is mid-transition: if it is already closing, the sort is queued and runs once the
    edit closes; if it is still opening, the sort is dropped;
  - an app `Sorting` handler cancels, or the column is removed before the sort applies.

  **planned** `Invoke` fails with `UIA_E_INVALIDOPERATION` when no sort can happen: no sort key
  (the header is then not sortable at all), a pending or refusing source, or an edit that cannot be
  committed (the edit stays open and keeps focus) or that is still opening. A sort queued behind a
  closing edit is a successful `Invoke`; the sort runs when the edit closes. A sort cancelled by the app's
  `Sorting` handler is also a successful `Invoke` with no change, because the app chose it.
- **Resizable**: `TableView.CanUserResizeColumns` and `TableViewColumn.CanResize` are both `true`.
- **Has a context menu**: a context flyout would open for the header. That is a `ContextFlyout` on
  the header or on content inside it (from `HeaderStyle`, `ColumnHeaderStyle` or `HeaderTemplate`;
  these take precedence), else `TableViewColumn.HeaderContextFlyout`, else `TableView.ContextFlyout`,
  which an unhandled header request reaches by bubbling. A table-wide `TableView.ContextFlyout`
  therefore makes **every** header actionable. A `ContextRequested` handler that the app
  attaches itself cannot be detected; see the opt-in under [Open questions](#open-questions).
- **Has focusable template content**: `HeaderTemplate` or `HeaderTemplateSelector` produces a
  focusable, enabled element (for example a filter `Button`).
- **Has a tooltip**: the header element has a tooltip. That is a `ToolTipService.ToolTip` the app
  set on the header (for example through `HeaderStyle` or `ColumnHeaderStyle`), which takes
  precedence over the column's, or else a `TableViewColumn.HeaderToolTip` that is not `null` or an
  empty string. Both are detectable with `ToolTipService.GetToolTip` on the header. The header is
  informative rather than operable, but keyboard focus is the only way a sighted keyboard user can
  open its tooltip, and APG only says non-functional headers *need not* be focusable, not that they
  must not be.

Rules:

- **planned** Only actionable headers are focus targets. A non-actionable header is a label: it is
  not reachable by keyboard, and arrow navigation skips it. The control enforces this without
  writing `IsTabStop`, so the header style keeps owning it: when focus is about to land on a
  non-actionable header, `GettingFocus` redirects it with `TrySetNewFocusedElement` to the next
  valid target in the same direction (an actionable header, the body, or, for `Tab`, the next
  element after the table), so `Tab` never gets stuck. `TableViewColumnHeaderAutomationPeer`
  overrides `IsKeyboardFocusableCore` to report the same answer.
- **current** An `IsTabStop="False"` set through `HeaderStyle` or `ColumnHeaderStyle` takes the
  header out of keyboard navigation: `Tab` does not stop on it and `Left`/`Right` skip it. Its
  actions stay available by pointer and UIA. **planned** This continues to win over actionability.
  Making a non-actionable header focusable is not supported; see
  [Open questions](#open-questions).
- **planned** Actionability is re-evaluated when any input above changes (for example
  `CanUserSortColumns`, `CanSort`, `SortMemberPath`, `CanResize`, a flyout, `HeaderToolTip`, or a
  `ToolTipService.ToolTip` set on the header by a style). If the focused header stops being actionable, focus moves to the nearest actionable
  header in the same direction the user last moved, else to the first row. If there is neither,
  focus stays on that header until the user moves it; the control does not push focus out of the
  table on its own. The header peer raises property-changed
  events for `IsKeyboardFocusable` and `HelpText`. WinUI has no property-changed identifiers for
  pattern availability, so clients re-query `Invoke` and `Transform` support; see
  [Open questions](#open-questions).
- **current** `Up` from the first row, or from a group header in the first position, moves to the
  header band. At cell level it lands on the header in the current column; at row level, on the
  remembered header column, else the first header. The key is consumed only when a header takes
  focus: when column headers are hidden, or the header in that column refuses focus (for example
  a style-set `IsTabStop="False"`), `Up` is not handled and XAML directional focus can move focus
  out of the table. **planned** It lands on the nearest actionable header to that column. When no
  header can take focus, `Up` there does nothing and the key is consumed, so directional focus
  cannot leave the table.
- **current** `Down` from a header returns to the first row. At cell level it lands in the
  header's column. `Up` in the header band does nothing.
- **current** `Left` and `Right` move between headers and do not wrap; they mirror under RTL.
  **planned** They move between actionable headers only.
- **current** `Enter` and `Space` sort (`Space` fires on key-up). `Alt+Left` and `Alt+Right` resize
  (`Alt+Shift` for a large step). The context-menu key or `Shift+F10` opens the header's context
  flyout.
- **planned** Focusable header template content follows the cell rules below: the header takes
  focus, `Enter` or `F2` moves into its content, and `Esc` or `F2` returns to the header. `Enter`
  on the header itself still sorts when the header has no focusable content. When it has both,
  `Enter` moves into the content and `Space` sorts.
- **planned** Non-actionable headers stay in the UIA tree as `HeaderItem` elements. They are
  announced through the cell-to-column-header association (see below), not through keyboard focus.
- **current** Every visible header is focusable, including one with `CanSort="False"` and nothing
  else to do, and the header band is its own tab stop. There is no interaction mode for header
  template content.

Why: a focus stop that does nothing costs keyboard users a key press and tells screen-reader users
there is a control where there is none. APG makes headers that don't sort or filter optional focus
targets, and its data-grid examples make headers focusable only when they sort. Windows asks that
non-actionable items not be tab stops.

#### Rows and cells

- **current** Focus is row-level by default. `Up`, `Down`, `PageUp` and `PageDown` move between rows.
  At row level, `Home` and `End` go to the first or last row, and `Ctrl+Home` and `Ctrl+End` enter
  cell level on the first cell of the first row or the last cell of the last row.
- **current** `Right` on a row moves to its first cell, and `Left` on the first cell returns to the
  row. At cell level, `Left` and `Right` move between cells without wrapping, `Up` and `Down` keep
  the column, `Home` and `End` go to the first or last cell of the row, and `Ctrl+Home` and
  `Ctrl+End` go to the first or last cell of the table. All horizontal keys mirror under RTL.
- **current** `Down` on the last row is not consumed, so XAML directional focus can move focus out
  of the table; the same happens for `Up` on the first row when no header takes focus.
  **planned** Both are always consumed.
- **current** Read-only cells **are** reachable by arrow keys, whether the table, the column, or
  both are read-only. A sighted keyboard user can reach per-cell content and cell tooltips, and a
  screen-reader user can read cell by cell. Cells are never tab stops.
- **current** Selection follows focus. `Ctrl` with a navigation key moves focus without selecting,
  and `Space` selects the focused data row from row or cell level. On a group header `Space` toggles
  expansion instead (see below).

Why: the row-then-cell model is APG's **treegrid** pattern, where focus can rest on a row and
`Right` and `Left` move into and out of its cells. APG's grid pattern also requires every data cell
to be focusable or to contain a focusable element, so content is not invisible in a screen reader's
application mode. Starting at row level keeps the common case (pick a row) to one key per row, as
in `ListView`.

#### Tree rows and group headers

- **current** On a hierarchical row (`Level` > 0), `Right` expands a collapsed row and otherwise
  moves to the first cell. `Left` collapses an expanded row and otherwise moves focus to the parent
  row. `Multiply` (numeric keypad `*`) expands the row's whole subtree. `Shift` with `Left` or
  `Right` keeps the plain row/cell move. All of these mirror under RTL.
- **current** Group headers take row-level focus and have no cell level. `Up`, `Down`, `PageUp` and
  `PageDown` treat them as rows, and `Up` from a group header in the first position moves to the
  header band. `Right` expands and `Left` collapses (mirrored under RTL); `Enter` and `Space`
  toggle. `Space` on a group header toggles it rather than selecting.
- **current** UIA: a group header is a `Group` element with `ExpandCollapse` and `GridItem`; a
  hierarchical row adds `ExpandCollapse` to its row peer.
- **current** `Tab` entry from outside the table that XAML aims at a group header is not
  redirected, so it can land on a group header outside the viewport (see
  [Tab](#tab-one-tab-stop-for-the-whole-table)).
- **planned** Group headers become deliberate entry targets: entry from outside always goes
  through the entry order above, whether XAML aimed at a data row or a group header, and lands on a
  group header only when that order resolves to one.

#### Controls inside cell templates

Template content stays **inside** the table's single tab stop. A cell has two modes, as in the APG
grid pattern:

- **Navigation mode** (the default): arrow keys move between cells.
- **Interaction mode**: keys go to the cell's controls.

The cell always takes focus first, whatever it hosts. This is a deliberate deviation from APG's
"Whether to Focus on a Cell or an Element Inside It", which recommends putting focus directly on a
cell's single non-arrow-key widget. `TableView` keeps the cell as the focus target so that `Space`
means "select this row" everywhere in the body, the focused element is always the one that
carries `GridItem` and `TableItem`, and the model matches `ListViewItem`, where the item takes
focus and its content is reached explicitly. The cost is that a screen reader announces the cell,
not the control's role: a cell whose content is an interactive control is named after its column
header, and the control is a child element in the UIA tree, where Narrator reaches it by item
navigation. Whether the cell should also surface the control's presence on focus is listed under
[Open questions](#open-questions).

| Cell content | Reaching the cell | Using it |
|---|---|---|
| One simple control (`Button`, `CheckBox`, `HyperlinkButton`, `ToggleSwitch`) | Arrowing focuses the cell. | **current** `Enter` moves focus into the control. Then `Enter` or `Space` activates it. **planned** `F2` also moves focus into it. |
| A control that uses arrow keys or text input (`ComboBox`, `TextBox`, `Slider`), or several controls | Arrowing focuses the cell. | `Enter` (**current**) or `F2` (**planned**) enters interaction mode and focuses the first control. **planned** `Tab` and `Shift+Tab` cycle through the controls in that cell. `Esc` (**current**) or `F2` (**planned**) returns to navigation mode on the cell. |

- **current** In interaction mode, arrow keys the hosted control does not handle are consumed, so
  focus cannot jump to a geometrically nearby cell. `Tab` is left to the hosted control and follows
  the normal XAML tab order. It is not confined to the cell.
- **current** On an editable cell `F2` begins a cell edit, not interaction mode. **planned** `F2`
  begins an edit when the cell has an editor, and enters interaction mode otherwise.
- **current** `Esc` is checked in this order: a hosted control that handles `Esc` (for example a
  `ComboBox` closing its drop-down) keeps it; then an open edit is cancelled; then interaction
  mode is left. **planned** No state traps the keyboard: `Esc` or `F2` always gets the user back to
  navigation mode.
- **planned** `Tab` never moves focus from navigation mode to a `ComboBox` or other control in a
  cell. The only way in is `Enter` or `F2`.

Why: APG's grid pattern describes a mode switch for cells with editable content, several widgets,
or a widget that uses arrow keys: `Enter` or `F2` goes in, `Esc` or `F2` comes back. Without it, a
`ComboBox` would claim the arrow keys and strand grid navigation.

#### Screen readers (UIA)

- **current** `TableViewAutomationPeer` is a `DataGrid` with the `Grid` and `Table` patterns,
  `Selection` while `SelectionMode` allows selection, `ItemContainer`, and `Scroll` (delegated to
  the body's `ScrollViewer`). `Table.GetColumnHeaders` returns one `HeaderItem` per visible logical
  column, whether or not its header has been realized, and none while `HeadersVisibility` hides the
  column headers.
- **current** Rows are `DataItem` elements with `SelectionItem` while selectable, `ExpandCollapse`
  on hierarchical rows, and `VirtualizedItem` when virtualized.
- **current** Cells are `DataItem` elements with localized control type "cell", `GridItem` and
  `TableItem`, `Value` on editable text cells, and `VirtualizedItem` when virtualized.
  `TableItem.GetColumnHeaderItems` returns the cell's column header, so a screen reader can announce
  the column when focus moves to a new column. A cell's name does not repeat its header while
  headers are shown (see the dev spec's *Accessibility notes*).
- **current** Headers are `HeaderItem` elements. They expose `Invoke` when `CanUserSortColumns` and
  `CanSort` are true, the column is visible and the table is loaded, and `Transform` when the column is resizable.
  `HelpText` is the string `TableViewColumn.HeaderToolTip` (if any; dropped when it only repeats
  the header's name), joined with the sort state on sortable columns; with neither, it falls back
  to the base help text (see [Tooltips](#tooltips)). The peer reads only `HeaderToolTip`: a tooltip set on the header through a style is not reported to UIA, and
  when a style tooltip wins over `HeaderToolTip` the peer still reports `HeaderToolTip`.
- **planned** The peer reads the effective tooltip of the realized header
  (`ToolTipService.GetToolTip` on the header: a string, or a `ToolTip` whose `Content` is a string),
  otherwise `HeaderToolTip` when the header is not realized, so a header that is actionable only
  because of a style-set tooltip is announced with that text. The full `HelpText` is the app's
  `AutomationProperties.HelpText` on the header, then the tooltip text, then the sort state.
- **planned** `Invoke`, and the sort state in `HelpText`, follow the **Sortable** test above. A
  header that is only resizable exposes `Transform` and not `Invoke`. Today a column with
  `CanSort="True"` and no sort key advertises an `Invoke` that cannot sort.
- Narrator's table commands are designed to work without giving headers keyboard focus:
  `Ctrl+Alt+Arrow` moves cell to cell, `Ctrl+Shift+Alt+Up` reads the current column header, and
  scan mode reads the table. They rely on the `Table`/`TableItem` association above. Verify them
  manually with Narrator; there is no automated coverage.

The control type and patterns already tell a screen reader what a header can do (`HeaderItem`
with `Invoke` is sortable; with `Transform`, resizable). The control does not add instructional
`HelpText` such as "Press Enter to sort". An app that wants it can put it in `HeaderToolTip`, which
is published as `HelpText`.

#### Keyboard access to tooltips

Three opt-ins: `TableViewColumn.HeaderToolTip` (header), `TableViewColumn.CellToolTipBinding`
(cell), and a row-level tooltip, proposed as `TableView.RowToolTipBinding` in a separate change.
`TableView` uses the platform `ToolTip` for all three.

- **current** Header and cell tooltips open on keyboard focus. The control attaches each tooltip to
  the header or to the cell wrapper, and keyboard navigation focuses both with
  `FocusState.Keyboard`. `ToolTipService` handles the owner's `GotFocus`, which bubbles, so a
  tooltip opens when the owner **or a descendant** takes keyboard focus, or takes focus that UI
  Automation set (for example a screen reader's `SetFocus`). A cell tooltip therefore
  also opens when a control inside the cell is focused in interaction mode. A tooltip opened by
  keyboard focus treats `PlacementMode.Mouse` as `Top`, because there is no pointer position.
- **planned** A header with a tooltip (`HeaderToolTip`, or a `ToolTipService.ToolTip` set on the
  header by a style) counts as actionable, so it stays focusable and its tooltip stays reachable by
  keyboard, unless a style sets `IsTabStop="False"`.
- **current** String content is also published to UIA as `HelpText` (cells through
  `AutomationProperties.HelpText`, headers through the header peer for `HeaderToolTip` only; see
  [Screen readers (UIA)](#screen-readers-uia)), so screen-reader users get the text without the
  popup.
- **current** Row-level focus shows no tooltip: no row tooltip exists, and a tooltip an app puts on
  an element inside its cell template opens only when that element itself (or a descendant) has
  focus or hover, not when the cell or row is focused.
- **planned** The proposed `RowToolTipBinding` opens only at row level, when the row itself has
  focus. When a cell or its content has focus, the cell tooltip (if any) wins and the row tooltip
  stays closed. Because the platform opens an owner's tooltip on descendant focus too, this needs
  either an owner-only check on the focus event's original source or a host element that only the
  row itself focuses; the implementing change picks one.
- **current** The platform `ToolTip` behaves as follows (WinUI `ToolTipService`):
  - **No timeout.** `ToolTipService` reads `SPI_GETMESSAGEDURATION` when it opens a tooltip, but
    never starts its close timer with that value, so an open tooltip stays open until something
    below closes it.
  - **Hoverable.** After the pointer leaves the owner, the tooltip stays open while the pointer is
    in the safe zone: the owner, the tooltip, and the convex hull around both. A tooltip opened by
    keyboard focus is not dismissed by the safe-zone check until the pointer actually moves.
  - **Dismissal.** It closes when the owner loses focus, when the pointer leaves the safe zone, or
    when `Ctrl` is pressed and released on its own. It has no `Esc` handling.

  Against WCAG 1.4.13 (dismissible without moving focus, hoverable, persistent), the remaining gap
  is that `Esc`, the conventional dismiss key, does nothing; `Ctrl` does dismiss it. That is listed
  under [Open questions](#open-questions).
- **planned** Where layout allows, avoid truncation (for example an `Auto` width or a resizable
  column) rather than relying on a tooltip to show the full value.

#### Current vs. planned

| Area | Current | Planned |
|---|---|---|
| Tab stops | Two: header band, then body | One: the body, or the headers when the body has no focusable row |
| `Tab` entry into the body from outside | When XAML aims at a data row: remembered row (or its remembered-column cell), else first or last enabled data row in the viewport. When it aims at a group header: that group header, not redirected, possibly outside the viewport | Always the entry order: remembered row or cell, else selected row in the viewport, else first or last row in the viewport; group headers only when that order resolves to one |
| Which headers take focus | Every visible header the style lets take focus | Actionable headers only (static sort configuration, resize, context menu, focusable template content, a header tooltip); a style-set `IsTabStop="False"` still wins; non-actionable headers redirected past in `GettingFocus` |
| `Up` from the first row or first-position group header | Header in the current column (cell level) or remembered/first header (row level); not consumed when headers are hidden or that header refuses focus | Nearest actionable header; if none, nothing, and the key is consumed |
| `Down` on the last row | Not consumed | Consumed |
| Header `Invoke` and sort `HelpText` | Disabled header: `UIA_E_ELEMENTNOTENABLED`; then `CanUserSortColumns && CanSort`, column visible, table loaded; a refused sort is a silent no-op | The **Sortable** test: also a sort key; a sort that cannot happen fails with `UIA_E_INVALIDOPERATION`; a queued or app-cancelled sort succeeds |
| Header `HelpText` | `HeaderToolTip` and sort state; else base help text (the TableView's for a `TableView`-owned peer: before realization via `GetColumnHeaders`, or any constructor-built peer) | App `HelpText`, then the effective tooltip of the realized header (otherwise `HeaderToolTip`), then the sort state |
| Actionability changes at run time | n/a | Focus moves to the nearest actionable header or first row, else stays until the user moves it; `IsKeyboardFocusable` and `HelpText` change events raised |
| Header template content | No interaction mode | `Enter`/`F2` in, `Esc`/`F2` out, as for cells |
| Entering a cell's controls | `Enter` | `Enter` or `F2` (`F2` still begins an edit on an editable cell) |
| `Tab` in interaction mode | Follows XAML tab order; can leave the cell | Cycles within the cell |
| Leaving interaction mode | `Esc` | `Esc` or `F2` |
| Tooltips | Header and cell tooltips open on keyboard focus (including descendant focus); every header the style lets take focus is focusable, so its tooltip is reachable (a style-set `IsTabStop="False"` wins); no row tooltip | Unchanged for headers: a header with a tooltip (`HeaderToolTip` or a style-set `ToolTipService.ToolTip`) stays focusable, and a style-set `IsTabStop="False"` still wins. Row tooltip through the proposed `RowToolTipBinding`, row level only, cell tooltip wins |

Unchanged: the cell takes focus before its controls, read-only cells reachable by arrows, row-level
focus by default, tree-row and group-header keys, `Enter`/`Space` sort, `Alt+Arrow` resize,
keyboard access to cell tooltips and to tooltips on headers that stay focusable (`HeaderToolTip` or
a style-set `ToolTipService.ToolTip`), the UIA tree shape, cell-to-header
association, and platform `ToolTip` dismissal behavior.

#### Rationale and standards

**Microsoft guidance**

- [Keyboard interactions](https://learn.microsoft.com/windows/apps/design/input/keyboard-interactions):
  "Ensure actionable items are identified as tab stops (and non-actionable items are not)";
  "Provide arrow key navigation for 'inner navigations'"; and "Define a single tab stop for a
  collection of related controls". The single tab stop and actionable-only headers come from this
  page. Reaching actionable headers by arrow rather than by `Tab` is the trade-off explained under
  [Tab](#tab-one-tab-stop-for-the-whole-table).
- [Keyboard accessibility](https://learn.microsoft.com/windows/apps/design/accessibility/keyboard-accessibility):
  focus order should follow the visual order. Grids and tables are called out as a common source of
  mismatch.
- [Focus navigation](https://learn.microsoft.com/windows/apps/design/input/focus-navigation):
  `TabFocusNavigation` and directional (arrow) navigation as the platform tools for a single tab
  stop with arrow-key inner navigation.
- [HeaderItem control type](https://learn.microsoft.com/windows/win32/winauto/uiauto-supportheaderitemcontroltype):
  implement `Invoke` only "if the header item control can be clicked to sort the data", and
  `Transform` if it can be resized. This is why a resizable-only header does not expose `Invoke`.
- [Table](https://learn.microsoft.com/windows/win32/winauto/uiauto-implementingtable) and
  [TableItem](https://learn.microsoft.com/windows/win32/winauto/uiauto-implementingtableitem)
  control patterns: `GetColumnHeaders` and `GetColumnHeaderItems` are how a screen reader learns a
  cell's header without the header having focus.
- [Tooltips](https://learn.microsoft.com/windows/apps/design/controls/tooltips): a tooltip appears
  when the user moves focus to, presses and holds, or hovers over its control.
- [Narrator keyboard commands](https://support.microsoft.com/accessibility/windows/narrator/appendix-b-narrator-keyboard-commands-and-touch-gestures):
  table navigation (`Ctrl+Alt+Arrow`) and "Read current column header" (`Ctrl+Shift+Alt+Up`).

**W3C**

- [WAI-ARIA APG treegrid pattern](https://www.w3.org/WAI/ARIA/apg/patterns/treegrid/): focus can
  rest on a row, `Right` moves from a row into its first cell, and `Left` from the first cell moves
  back to the row; `Right` and `Left` also expand and collapse rows. This is the basis for row-level
  focus, cell drill-in, and tree-row keys.
- [WAI-ARIA APG grid pattern](https://www.w3.org/WAI/ARIA/apg/patterns/grid/): only one focusable
  element of a grid is in the tab sequence; every cell is focusable or contains a focusable element,
  except that header cells that don't provide functions such as sort or filter need not be
  focusable; `Enter` or `F2` moves into a cell's widgets, and `Esc` or `F2` restores grid
  navigation. This is the basis for interaction mode. `TableView` deviates from its advice to focus
  a single simple widget directly (see
  [Controls inside cell templates](#controls-inside-cell-templates)).
- [APG data grid examples](https://www.w3.org/WAI/ARIA/apg/patterns/grid/examples/data-grids/):
  the first example's headers are not focusable and are still announced during navigation; the
  sortable example makes its headers focusable because they sort.
- WCAG 2.1:
  [2.1.1 Keyboard](https://www.w3.org/WAI/WCAG21/Understanding/keyboard.html) (every action,
  including sort and resize, works from the keyboard, which is why headers become the tab stop when
  there are no rows),
  [2.1.2 No Keyboard Trap](https://www.w3.org/WAI/WCAG21/Understanding/no-keyboard-trap.html)
  (`Tab` always leaves the table from navigation mode; `Esc` or `F2` always leaves interaction
  mode),
  [2.4.3 Focus Order](https://www.w3.org/WAI/WCAG21/Understanding/focus-order.html),
  [1.3.1 Info and Relationships](https://www.w3.org/WAI/WCAG21/Understanding/info-and-relationships.html)
  (cell-to-header association),
  [4.1.2 Name, Role, Value](https://www.w3.org/WAI/WCAG21/Understanding/name-role-value.html)
  (patterns that match behavior, such as `Invoke` only when it sorts), and
  [1.4.13 Content on Hover or Focus](https://www.w3.org/WAI/WCAG21/Understanding/content-on-hover-or-focus.html)
  (tooltips; see the `Esc` gap above).

**Other frameworks.** Only behavior stated in each framework's public documentation or source is
listed. "—" means the source does not document it.

| Framework | Header focus | Sort / resize by keyboard | Row vs. cell focus | Read-only cells reachable | Header association for screen readers | Tooltips on keyboard focus |
|---|---|---|---|---|---|---|
| Win32 ListView ([overview](https://learn.microsoft.com/windows/win32/controls/list-view-controls-overview), [LVN_COLUMNCLICK](https://learn.microsoft.com/windows/win32/controls/lvn-columnclick)) | Separate header control whose items can hold focus ([HDM_SETFOCUSEDITEM](https://learn.microsoft.com/windows/win32/controls/hdm-setfocuseditem)) | Sort is app-implemented on column click | Item (row) | n/a: item-level focus | — | — |
| WPF DataGrid ([keyboard](https://learn.microsoft.com/dotnet/desktop/wpf/controls/default-keyboard-and-mouse-behavior-in-the-datagrid-control), [cell peer](https://learn.microsoft.com/dotnet/api/system.windows.automation.peers.datagridcellitemautomationpeer)) | Not in the default key map | Not in the default key map | Cell; `Tab` moves cell to cell and leaves after the last cell | Yes: arrows move cell to cell; `IsReadOnly` only gates `F2` | `ITableItemProvider.GetColumnHeaderItems` on the cell peer | — |
| WinForms DataGridView ([keyboard](https://learn.microsoft.com/dotnet/desktop/winforms/controls/default-keyboard-and-mouse-handling-in-the-windows-forms-datagridview-control)) | Not needed for sort or resize | `F3` sorts the current column; `Alt+Left/Right` resizes it | Cell; `Tab` moves cell to cell unless `StandardTab` | Yes | — | — |
| Windows Community Toolkit DataGrid ([source](https://github.com/CommunityToolkit/WindowsCommunityToolkit/blob/main/Microsoft.Toolkit.Uwp.UI.Controls.DataGrid/DataGrid/DataGrid.cs)) | Inside the control's one tab stop (`TabNavigation="Once"`): `Tab` from a cell that is not editing moves to the current column's header; `Down` returns to the rows | `Enter` on the focused header sorts | Cell | — | — | — |
| Fluent UI React v9 DataGrid ([source](https://github.com/microsoft/fluentui/tree/master/packages/react-components/react-table)) | — | — | `focusMode`: `cell` (default), `composite` (row then cell), `none`; cell `focusMode="group"` for nested controls | Yes with `cell` focus mode | — | — |
| AG Grid ([keyboard](https://www.ag-grid.com/javascript-data-grid/keyboard-navigation/)) | All headers; `Up` from the first row enters the header | `Enter` sorts; `Alt+Left/Right` resizes; `Alt+Down` opens the menu | Cell; `Tab` moves across cells and headers | Yes | — | — |
| MUI X Data Grid ([accessibility](https://mui.com/x/react-data-grid/accessibility/)) | Headers take focus | `Enter` sorts the focused header; `Ctrl+Enter` opens its menu | Cell; one tab stop by default (`tabNavigation="none"`), per APG | Yes | — | — |
| Qt QTableView / QHeaderView ([QHeaderView](https://doc.qt.io/qt-6/qheaderview.html), [source](https://github.com/qt/qtbase/blob/dev/src/widgets/itemviews/qheaderview.cpp)) | Never: `QHeaderView` sets `Qt::NoFocus` | Sections are clickable (`setSectionsClickable`); no keyboard path documented | — | — | — | — |
| GTK 4 ColumnView ([ColumnView](https://docs.gtk.org/gtk4/class.ColumnView.html), [ListTabBehavior](https://docs.gtk.org/gtk4/enum.ListTabBehavior.html)) | — | Sort by clicking a header | `tab-behavior`: `ALL`, `ITEM` (one row, then out) or `CELL` (one cell, then out; arrows move between cells) | — | — | — |

What the comparison shows: no framework surveyed needs a separate tab stop per header. MUI X (by
default) and GTK `ITEM`/`CELL` keep the whole grid at one tab stop, as this model does. AG Grid
puts each header in the `Tab` sequence, and does the same for every cell, which this model rejects
for the trap reason above. Qt never focuses headers, and WinForms sorts and resizes from the
current cell, so header focus is not required for a usable grid. The Windows Community Toolkit
DataGrid reaches headers with `Tab` inside its single tab stop, which is the alternative listed
under [Open questions](#open-questions). This model keeps focus on *actionable* headers because the
sort and resize controls live on the header visually, and UIA's `HeaderItem` already models them
there.

#### Open questions

- **Reaching headers with `Tab`.** The planned model reaches headers with `Up` from the first row.
  Two alternatives follow "actionable items are tab stops" more literally and make headers easier
  to discover:
  - make the header band its own tab stop only while at least one header is actionable; or
  - as in the Windows Community Toolkit DataGrid, let `Tab` from a row or cell in navigation mode
    move to the current column's header inside the same tab stop, and `Tab` again leave the table.

  Both cost a second `Tab` press on most sortable tables. Not decided.
- **Opting a header in.** An app that handles `ContextRequested` itself, or wants a
  non-actionable header to be focusable for another reason, has no way to say so: the control
  cannot detect handlers, and `IsTabStop="True"` is already the style default. Options are an
  explicit property (for example on `TableViewColumn`) or treating any `ContextRequested` handler
  on the header as actionable. Not decided.
- **Pattern-availability events.** UIA defines property-changed events for `Invoke` and
  `Transform` availability, but WinUI exposes no identifiers for them, so the header peer cannot
  raise them when a header becomes sortable or resizable. Does this need a platform addition?
- **Announcing controls in cells.** Because the cell takes focus first, a screen reader announces
  the cell (named after its column header), not the control's role and state. Should the cell
  surface the hosted control on focus (for example through its `ItemStatus` or a description),
  or is item navigation to the child element enough?
- **Row name with column names.** Should a row's UIA name include column names ("Name: Alice,
  Role: Engineer") rather than only cell values? It helps row-level reading but lengthens every
  announcement and duplicates the `TableItem` association. Not decided; this spec does not change
  row naming.
- **`Esc` to dismiss a tooltip (WCAG 1.4.13).** The platform `ToolTip` closes on `Ctrl` alone, on
  focus loss, and when the pointer leaves the safe zone, but not on `Esc`. Should `TableView`
  close an open tooltip on `Esc`? If so, that `Esc` must come before the edit-cancel and
  leave-interaction-mode checks, and must not also do either. Or is this a platform `ToolTip`
  fix?
- **`Esc` inside a dialog.** In a `ContentDialog`, `Esc` also closes the dialog. Should the table
  mark `Esc` handled only when it actually cancelled an edit, left interaction mode or closed a
  tooltip, so the dialog never closes on the same press? What happens on a second `Esc`?
- **Per-group tooltips.** An implicit `TableViewGroupHeader` style can only give every group header
  the same static tooltip. Per-group, keyboard-accessible content needs either a control-owned
  opt-in (for example a group-header tooltip binding evaluated against `TableViewGroupInfo`) or a
  supported hook where an app sets `ToolTipService.ToolTip` in code when a group header container
  is realized. Not decided.
- **`Ctrl+Home` and headers.** Should `Ctrl+Home` stop at the first actionable header (AG Grid
  style) or stay on the first cell (APG, WPF)? The current and planned behavior is the first cell.

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
