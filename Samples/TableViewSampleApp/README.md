# TableView sample

A small WinUI 3 desktop app that exercises the live public API of the
`Microsoft.UI.Xaml.Controls.Tabular.TableView` control. The navigation pane groups focused pages for
quick-start usage, full-app scenarios, basics, columns, rows and cells, and power-user behavior.

`TableView` ships in `Microsoft.UI.Xaml.Controls.Tabular.dll`, separate from the main framework DLL,
but its API is published through the WindowsAppSDK NuGet package: type information reaches the
public winmd, activation registrations are emitted, and the control's theme resources ship and
resolve. So this sample is an **ordinary consumer** — it references the package and does nothing
special, exactly like the [ChartApp](../ChartApp) samples.

## Prerequisites

- Visual Studio 2022 with the **Desktop development with C++** and **.NET Desktop** workloads.
- A full repo initialization has been run once from the repo root:

  ```
  .\init.cmd
  ```

## Build

`TableView` is not in any published `Microsoft.WindowsAppSDK.WinUI` package yet, so this sample can
only build against a **locally packed** component. Building the project on its own resolves the
package from the feed and fails with `CS0234: The type or namespace name 'Tabular' does not exist`.

The supported flow packs the component and passes the matching version for you:

```
.\Build.cmd product
.\Build.cmd samples
```

To iterate on just this app, pack once and point the build at that version:

```
.\pack.component.cmd /version 3.0.0-mylocal
.\initrun.ps1 msb /q /restore Samples\TableViewSampleApp\TableViewSampleApp.csproj /p:Platform=x64 /p:WinUIVersion=3.0.0-mylocal
```

Re-run `pack.component.cmd` after every control change — NuGet caches by version, so reuse the
version only if you also clear `packages\microsoft.windowsappsdk.winui\`, and prefer a fresh version
string when in doubt.

## Run

```
BuildOutput\obj\amd64chk\Samples\TableViewSampleApp\TableViewSampleApp.exe
```

Supported launch arguments:

- `--page=<tag>` opens a navigation page directly. Tags are the values in
  `MainWindow.xaml` / `MainWindow.xaml.cs`, for example `Showcase`, `Selection`,
  `GridLinesVisibility`, `Settings`, or `About`.
- `--show-dev-info` shows the developer status bar with the loaded Tabular controls assembly path,
  size, and timestamp. This is intended for local/UIA verification, not end-user docs.

The app also contains an opt-in UIA verification log harness for local automation. It is compiled
only when `TableViewSampleEnableVerificationLogs=true` is passed to MSBuild, and then activated with
`--verify-groups` or `--verify-selection`. Normal public sample builds do not write log files next
to the executable.

The same build also has a context menu self-check. Launch with
`--page=ContextMenus --verify-context-menus`: the page raises real mouse, keyboard (Shift+F10, Menu
key) and UIA `ShowContextMenu` requests against cells, rows and headers, writes PASS/FAIL lines to
`verification-context-menus.txt` next to the executable, then exits. It moves the real mouse pointer
and pins the window on top while it runs, so leave the machine alone.

## Using TableView in your own app

Reference `Microsoft.WindowsAppSDK.WinUI` and use the control. In `App.xaml`, merge
`XamlControlsResources` and `TabularControlsResources` — the latter is Tabular's own theme-resource
dictionary, the exact analogue of `XamlControlsResources` for MUXC. Everything else the control
needs (its default style, theme XBFs and `.pri`) resolves from the package, so there is no URI to
configure and nothing to compile out of the control source tree.

`TabularControlsResources` is required, not optional: `TableView`'s column-header style resolves
`SortIndicatorForeground`, which is defined in Tabular's theme resources because `SortIndicator`
ships in the Tabular DLL rather than in MUXC. Without the merge the app throws
`XamlParseException 0x802B000A` during its first layout pass, which surfaces as a `0xC000027B`
stowed exception a few seconds after launch.

Declare columns, or the table renders rows with no header row and no cells:

```xml
<tabular:TableView x:Name="Table">
  <tabular:TableViewTextColumn Header="Name" Binding="{Binding Name}" />
  <tabular:TableViewTextColumn Header="Age"  Binding="{Binding Age}" />
</tabular:TableView>
```

`TableView` is `[MUX_PREVIEW]`, so C# usage raises `CS8305` and XAML usage raises `WMC1501`
("for evaluation purposes only"). This sample suppresses `CS8305`; the XAML warnings are left
visible on purpose.

## What it demonstrates

- A basic showcase table plus Task Manager, File Explorer, and File properties scenarios
- Selection, sorting, filtering, keyboard navigation, and accessibility behavior
- Column layout &amp; sizing (Pixel / Star widths, Min/Max clamps, drag-to-resize) and column
  lifecycle (add, remove, reorder at runtime), plus header visibility
- Tooltips, context menus, grid lines, cell templating, text wrapping, row height, cell editing,
  empty state, density, and grouped rows
- Hierarchical rows (`TableViewSource.ParentBy`): one flat list, two relations, composed with
  filter, sort and grouping, and `IsLiveShaping` reshaping both trees when a row's property changes.
  The table pages' Shaping section also offers Hierarchy and Grouped hierarchy (ParentBy, plus
  GroupBy on the roots) over their own data: people by manager, files by folder, processes by app;
  pages whose rows have no parent relation show those modes disabled with the reason. The Manager
  table shows per-row-kind context menus through `TableView.RowStyleSelector` (leaf rows,
  expandable rows and group headers each get their own `Style`)
- Right-to-left layout, virtualization, performance notes, theme settings, and About/build details
- `TableViewTextColumn` and `TableViewTemplateColumn` usage with source snippets embedded in the
  sample assembly. The snippets are generated from `// <snippet>` regions in the pages: after editing
  a region, run `powershell -File tools\Update-Snippets.ps1` and include the result. The build checks
  that every snippet is current and that every `Snippet` / `SourceSnippet` / `AdditionalSnippet` a
  page names exists: a warning locally, an error in CI.

The pages are code-behind on purpose (no view models): each one demonstrates TableView API calls,
not app architecture.

## Context menus

The **Context menus** page (`ContextMenus`) demonstrates per-column cell/header
flyouts and row fallback with the shared people data: a **Name** cell menu whose
items bind to the row item, a shared row menu (**Copy row**, **Move to next
department**, **Remove from list**) set through an implicit `TableViewRow` style,
per-column header menus that hide columns,
and the table's own `ContextFlyout` as the final fallback. The rail switches the
cell, row and header menus off.

There is no row menu property: rows get their menu from their own `ContextFlyout`,
set through an implicit `TableViewRow` style, a row template, or a `Style` picked per
row kind by `TableView.RowStyleSelector` (see the Hierarchy page). Column cell menus
take precedence over it; the native `TableView.ContextFlyout` is the final
fallback. A column header uses its
`HeaderContextFlyout`, otherwise native fallback. Group headers always use native
fallback. Setting a menu property to null (or removing the row style) removes that
candidate.

Keyboard requests (Shift+F10, Menu key) follow the focus level: with focus on a
cell, that cell's menu opens; with focus on the row itself (press Left from the
first cell), the row menu opens even if the remembered current column has a cell
menu. UI Automation `ShowContextMenu` on a row peer likewise opens the row menu.

A `ContextFlyout` you set inside a cell or header (`CellTemplate`,
`HeaderTemplate`, or an implicit style) always wins over the column properties,
for mouse, keyboard (Shift+F10, Menu key) and UI Automation alike. It covers the
whole cell or header, padding included.

The built-in copy/select flyouts on text controls don't count.

Context requests move body focus without changing selection. Headers do not move
body focus.
Menus are not stamped on realized cells; each menu inherits the requested row's
item as its `DataContext` while open. A target invalidated while focus moves is
not retargeted: custom display is suppressed. A body request with no applicable
custom candidate still permits native fallback if focus invalidates its target.

### Context menu diagnostics

The **Context menu diagnostics** page (`ContextMenuDiagnostics`, under Power user)
is a manual verification harness for the cases below. It is not a usage example
and has no source snippet. Its table uses `PersonData.Many(300)` with **First
name** (editable), **Office**, **Salary** and **Bio** columns.

The scrollable diagnostic toolbar keeps the table usable in smaller windows.
Right-click a **First name** cell and choose **Show first name value**; the result
appears below the table. Its `CommandParameter` binds to `FirstName` on the menu
item's inherited row `DataContext`. It changes no data, selection, or clipboard.
A null data row has no value, so the action is disabled. There is no automatic
cell-value API: bind to the row item.

Status shows open counts, selection, the opened menu, and its inherited item.
`fixtureIndex` identifies the opened menu's item in the original 300-row fixture
(`-1` for headers, null items, or replacement data).
`opens` counts menus made by this page, including inner/direct menus, but not the
XAML-defined `APP-OWNED` menu or an editor's built-in context menu.
Use **Reset** between independent cases. Reset restores ordinary data, headers,
templates, LTR and menu candidates, and cancels pending focus mutation; it
preserves the selection-mode checkbox and deliberately does not clear direct
menus on existing elements. **Navigate away/back after direct-menu or
nested-table cases** to create clean containers.

| Case | Reproduction and expected result |
|---|---|
| B1 | Right-click First name, Escape, Shift+F10, Escape, Apps key: one CELL open per gesture, correct bound item. |
| B2 | Select one row, right-click another: focus moves, selection does not. Repeat with SelectionMode.None. |
| B3 | Invoke First name, then trailing blank space in the same row: ROW. Clear row menu: NATIVE. |
| B4 | Scroll horizontally/vertically, invoke visible cells: the menu's item matches the target row. |
| B5 | Office/Salary uses ROW. Clear row menu and invoke again: NATIVE. |
| B9 | Right-click Bio text: APP-OWNED, no outer menu. A First name TextBox editor keeps native editing behavior; see limitation below. |
| B10 | Right-click a different First name row, Escape, F2: editing starts on the context row/cell, not the previously selected row. |
| B11 | Invoke First name on successive rows: CELL binding text changes to the requested item. |
| H1 | First name/Office header pointer or focused-header Shift+F10/Apps: HEADER/OFFICE HEADER, null item. Salary header has no candidate: NATIVE. |
| H3 | Direct header menu, invoke First name header: DIRECT HEADER. Recreate page afterwards. |
| H4 | Start First name editing with F2, invoke Office header: OFFICE HEADER remains eligible. |
| H5 | Blank header space or Group by Office then group header: NATIVE. |
| H6 | Hide headers, invoke former header position: no stale header menu. Show headers, invoke First name: HEADER once. |
| H7 | Repeat B1 and B5 after header cases: body behavior is unchanged. |
| L4 | Focus Mutate on next focus and invoke it, then right-click First name without left-clicking: synchronous GettingFocus replaces items; zero opens for the configured stale request. GotFocus itself is deferred until after Focus returns and cannot exercise this synchronous gate. |
| B5 + L4 | Reset, Clear row menu, focus Mutate on next focus and invoke it, then right-click Office: one NATIVE open, selection unchanged. Repeat after Reset and Null item row (wait for realizedNull=PASS), targeting the blank Office cell in row zero. The null row's First name cell remains configured and must still suppress with zero opens. |
| L6 | Direct row menu gives DIRECT ROW. Nested table then right-click Inner body gives INNER, not the outer ROW; Clear inner menus gives native outer fallback. |
| L7 | Retemplate three times; scroll past row 150 and back; invoke First name after each: one open each, current binding. |
| L8 | Developer-only native debugger case: a same-owner nested request must suppress. This intentionally has no shipping probe/button. |
| A1-A2 | Physical touch-hold and gamepad Menu on body/header: applicable menu once, selection unchanged. Require actual hardware; keyboard is not a substitute. |
| A3 | RTL + frozen First name, horizontal scroll, invoke First name/body/header: inspect target and mirrored platform placement. |
| A4 | Inspect an open menu with UIA/Narrator: CELL presenter and named CELL/current-item entries; only one popup. |
| A5 | Escape from CELL, then F2: row focus returns and First name editing works. |
| A7 | Plain flyout, invoke First name: PLAIN FLYOUT. |
| A8 | Group by Office, invoke group header: NATIVE. |
| Null-item regression | Null item row creates an actual null source item at row zero. Wait for realizedNull=PASS; right-click its blank First name/Office: CELL/ROW with a null item. Reset then reapply Null item row between cases. |

These are reproducible diagnostics, not a claim that every hardware/accessibility
case has passed. Known editing limitation: opening the built-in TextBox popup can
cause existing focus-loss commit/teardown before the context-menu pipeline runs.
That pre-existing editor behavior is separate from header eligibility while editing.
The existing frozen-column layout explicitly disables pinning in RTL. The A3
button exposes this limitation: First name can scroll out of view, so combined
RTL/frozen/scroll coverage must not be reported as passed.

## More detail

See [AGENTS.md](AGENTS.md).
