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
| PR 3 | Selection, single-column sort, filtering, grouping, 2-level hierarchy, column resize/reorder, nav-state; themed rendering. Shipped incrementally: selection, then the shaping engine with filter + sort, then grouping. |
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
- Width + `MinWidth`/`MaxWidth` (pixel in v1; `Auto`/`*` fall back to default width). — **PR2** · UI resize **PR3**
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
- 2-level nested rows (hard cap in v1). — **PR3**
- &gt;2-level hierarchy, row drag-drop, marquee selection. — *out of scope*

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
- Programmatic API: `BeginEdit`, `CommitEdit`, `CancelEdit`, `IsEditing`, `CurrentItem`/`CurrentColumn`/`SetCurrentCell`. — **cell editing**
- Vetoable lifecycle events `BeginningEdit`, `CellEditEnding`, `RowEditEnding`, plus `EditEnded`; the two `*EditEnding` args expose `GetDeferral()` so a handler can validate or save asynchronously without blocking the UI thread. — **cell editing**
- Rollback: `ITableViewEditableItem` on the data item when the app wants to own the transaction; otherwise the control snapshots the edited value and restores it on cancel. — **cell editing**
- Validation: `INotifyDataErrorInfo` on the data item, scoped to the property the column edits, blocks a commit. — **cell editing**
- Multi-cell row transactions (`CancelEdit(Row)` rolling back siblings already committed). — *deferred*
- Editing a11y announcements (live-region/UIA notification on begin/commit/cancel). — *deferred, needs localized strings*

### Virtualization & performance
- Row virtualization (realize only visible rows + cache). — **PR2**
- ≥30 FPS smooth scroll at 400–500 rows; high-frequency updates (Task Manager metrics); low latency. — **PR2/PR3**
- Column virtualization intentionally omitted (typical ~5–50 columns).

### Styling & theming
- Light / Dark / High Contrast theme tokens + inline fallbacks (`#29FFFFFF` dark gridline). — **PR2** (full self-themed Theme-XBF emission deferred)
- Cell-level styling: custom cells via `TableViewTemplateColumn` + built-in text-cell defaults (left, vertically centered). A public per-column alignment/weight API — **deferred (PR3+)**.

### Tooltips
- Cell: opt-in via `TableViewColumn.CellToolTipBinding`; the binding is evaluated against each row's data item and its value is the tooltip content. No binding means no tooltip and no per-cell cost. Needed because text cells render with `CharacterEllipsis` and no wrapping, so over-wide values are otherwise unreadable. — **PR3**
- Author precedence: a tooltip set inside a cell's own content template opens over that content; the control's tooltip covers the rest of the cell, and the control never touches a tooltip it did not attach.
- Content: a string, or any content a `ToolTip` can host. A `UIElement` is parented by that cell's `ToolTip`, so a converter returns a fresh element per evaluation. Computed content is authored with an `IValueConverter`.
- Accessibility: string tooltip text is published as the cell's `AutomationProperties.HelpText`, and `TableViewCellAutomationPeer` suppresses it at UIA query time when it merely repeats the cell's own text, so the value is not announced twice. Suppression is gated on the ownership record, so text the app set is never dropped.
- Recycling: a recycled row never shows a previous item's cell tooltip. No invalidation API is needed — the binding tracks the row's `DataContext`, so a recycled row re-resolves through the same inheritance that refreshes its cell text, and a source `PropertyChanged` updates a live tooltip in place.
- Because the control never calls into app code while realizing a cell, there is no re-entrancy surface, no drain budget, and no coalescing machinery.
- Column header: opt-in via `TableViewColumn.HeaderToolTip`, whose value is the tooltip content. Covers the whole header cell, including its padding and sort affordance. Header cells are rebuilt rather than recycled, so the value is read when the header is built and re-applied in place when it changes — no binding, no invalidation. String content is reported as the header's UIA help text by `TableViewColumnHeaderAutomationPeer`, joined with the sort state when the column has one; the header peer is virtual, so `AutomationProperties.HelpText` on the element would never reach a client. — **PR4**
- Group-header tooltips are **deferred**. Grouping ships, but the group header has no tooltip opt-in: `TableViewColumn.CellToolTipBinding` and `HeaderToolTip` are column-scoped, and the group header is a `TableViewGroupHeader` whose content is app-templated, so an app that wants a tooltip there puts one in its `GroupHeaderTemplate` today. A control-owned group-header tooltip API needs its own opt-in property and is not in this release.

> Tooltips are **not** gated on text truncation. No WinUI control keys tooltips off `IsTextTrimmed`; the shipped pattern is to gate on a cheap content predicate (non-empty string) or an explicit opt-in.

### Accessibility (UIA)
- Grid/Table peers; Row peer (`SelectionItem` + `GridItem`); ColumnHeader peer (`Invoke` → sort). — **PR2** read-only Grid/Table · **PR3** Selection + sort invoke
- Narrator, keyboard navigation parity, sort announcements. — **PR2** nav · **PR3** sort

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
