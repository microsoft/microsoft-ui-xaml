# TableView — Functional Spec

> **Internal** (mirror-excluded). Summarizes the engineering-relevant requirements; the
> authoritative, Confidential/Internal-Only sources are linked under [References](#references)
> (business/compete analysis is not reproduced here).

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
| PR 3 | Selection, single-column sort, grouping, 2-level hierarchy, column resize/reorder, nav-state; themed rendering |
| PR 4 | Tests + `TableViewSamples` e2e app |

## Functional requirements

Each item notes the MLP/v1 vs deferred status and the delivering PR.

### Core control & data model
- Explicit column model (no auto-generation). — **PR2**
- `ObservableCollection`-backed `ItemsSource`; incremental add/remove/update. — **PR2** render · **PR3** shaped (sort/group)
- Read-only by default (`IsReadOnly` = `true`). — **PR2**
- Custom cell templates (`TableViewTemplateColumn`). — **PR2**
- `GroupedItemsSource` for grouped/banded sections. — **PR3**

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
- 2-level nested rows (hard cap in v1) + grouping. — **PR3**
- &gt;2-level hierarchy, row drag-drop, marquee selection. — *out of scope*

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

### Accessibility (UIA)
- Grid/Table peers; Row peer (`SelectionItem` + `GridItem`); ColumnHeader peer (`Invoke` → sort). — **PR2** read-only Grid/Table · **PR3** Selection + sort invoke
- Narrator, keyboard navigation parity, sort announcements. — **PR2** nav · **PR3** sort

### Reliability & servicing
- Deterministic behavior, unit-test coverage (**PR4**), SFI/security compliance, post-adoption API stability.

## References

Authoritative (Microsoft Confidential — Internal Only):

- Scope & API parity — [`TableView-v1-Scope-API-Parity.docx`](https://microsoft.sharepoint-df.com/teams/ShellFTL/_layouts/15/Doc.aspx?sourcedoc=%7B4261A4D3-3D4B-4ABC-9A42-81FC0D5B0F48%7D&file=TableView-v1-Scope-API-Parity.docx)
- Functional 1-pager (purpose, compete analysis, business impact) — [`1 pager Functional 1 pager Table View Control.docx`](https://microsoft.sharepoint-df.com/teams/ShellFTL/_layouts/15/Doc.aspx?sourcedoc=%7BCBB7B347-128C-4528-838B-0A83AD85BAAF%7D&file=1%20pager%20Functional%201%20pager%20Table%20View%20Control.docx)
- MLP requirements matrix + NFR — [`Table View MLP requirements.xlsx`](https://microsoft.sharepoint-df.com/teams/ShellFTL/_layouts/15/Doc.aspx?sourcedoc=%7BFF1387FE-28FE-44E6-A6FC-09633A857A40%7D&file=Table%20View%20MLP%20requirements.xlsx)

Repo:

- Design (layout + implementation): [`TableView-dev-spec.md`](./TableView-dev-spec.md)
- API surface + samples: [`../../api-specs/TableView/TableView-spec.md`](../../api-specs/TableView/TableView-spec.md)
