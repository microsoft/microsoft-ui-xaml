# TableView sample

A small WinUI 3 desktop app that exercises the live public API of the
`Microsoft.UI.Xaml.Controls.Tabular.TableView` control. The left panel lets you tweak columns,
sizing, headers, grid lines, density, backgrounds, and more while the table updates in real time.

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

- Column `Width` (Auto / Pixel / Star), `MinWidth`, `MaxWidth`
- Add / remove / hide columns
- `HeadersVisibility`
- `GridLinesVisibility`
- `Density` (Compact / Standard / Comfortable)
- `RowBackground` / `AlternatingRowBackground`
- `FrozenEdge`
- `IsReadOnly`
- `HeaderTemplate`
- `EmptyTemplate`
- `TableViewTextColumn` and `TableViewTemplateColumn` (custom cell content)

The **Filter / sort / group** page exercises the data-shaping surface:

- `TableViewSource.Filter` / `ClearFilter` — text and predicate filters
- `TableViewSource.GroupBy` / `ClearGroupBy` — grouping by a value-type key
- `TableViewSource.Sort` / `ClearSort` — programmatic sort with `SortDirection`
- `TableView.CanUserSortColumns`, `SortByColumn`, `ClearSort`, `Sorting` / `Sorted`
- `GroupHeaderTemplate` (custom vs. built-in), `ExpandAllGroups` / `CollapseAllGroups`

## Context menus

The **Context menus** page demonstrates per-column cell/header flyouts and row
fallback. Column cell menus take precedence over `RowContextFlyout`; the native
`TableView.ContextFlyout` is the final fallback. `ContextFlyoutRequested` can
replace the resolved flyout. Setting it to null permits native fallback; setting
`Handled` suppresses all further display. No candidate means no override event.

Context requests move body focus without changing selection. Headers do not move
body focus. Direct element flyouts keep native priority on their routed path.
Menus are not stamped on realized cells. A target invalidated during an app
callback is not retargeted; custom display is suppressed unless the app explicitly
chooses null fallback. A body request with no applicable custom candidate before
focus still permits native fallback if focus invalidates its target, without an
override event. The event is synchronous: later changes to retained args
do not change its completed decision.

The scrollable diagnostic toolbar keeps the table usable in smaller windows.
Status shows request/open counts, selection, item, column, and menu binding.
`fixtureIndex` identifies the requested item in the original 300-row fixture
(`-1` for headers, null items, or replacement data).
`opens` counts menus made by this page, including inner/direct menus, but not the
XAML-defined `APP-OWNED` menu or an editor's built-in context menu. A visible
`binding=PASS` compares the menu item's inherited data context to the request item.
Use **Reset** between independent cases. Reset restores ordinary data, headers,
templates, LTR and menu candidates, cancels pending focus mutation, and releases
retained args; it preserves the selection-mode checkbox and deliberately does not
clear direct menus on existing elements. **Navigate away/back after direct-menu
or nested-table cases** to create clean containers.

| Case | Reproduction and expected result |
|---|---|
| B1 | Right-click Name, Escape, Shift+F10, Escape, Apps key: one CELL request/open per gesture, correct bound item. |
| B2 | Select one row, right-click another: focus moves, selection does not. Repeat with SelectionMode.None. |
| B3 | Invoke Name, then trailing blank space in the same row: ROW with null column, not the old Name column. Clear row menu: NATIVE without another override. |
| B4 | Scroll horizontally/vertically, invoke visible cells: column and item match the target. |
| B5 | City/Score uses ROW. Clear row menu and invoke again: NATIVE, zero custom requests after Reset. |
| B6-B8 | Name with Replace / Null / Suppress: REPLACEMENT / NATIVE / no menu; one callback each. |
| B9 | Right-click Notes text: APP-OWNED, zero outer requests. A Name TextBox editor keeps native editing behavior; see limitation below. |
| B10 | Right-click a different Name row, Escape, F2: editing starts on the context row/cell, not the previously selected row. |
| B11 | Invoke Name on successive rows: CELL binding text changes to the requested item. |
| H1 | Name/City header pointer or focused-header Shift+F10/Apps: HEADER/CITY HEADER, null item. |
| H2 | Header Replace / Null / Suppress: REPLACEMENT / NATIVE / none. Score header has no candidate: NATIVE without callback. |
| H3 | Direct header menu, invoke Name header: DIRECT HEADER without custom callback. Recreate page afterwards. |
| H4 | Start Name editing with F2, invoke City header: CITY HEADER remains eligible. |
| H5 | Blank header space or Group by City then group header: NATIVE, no custom callback. |
| H6 | Hide headers, invoke former header position: no stale header request. Show headers, invoke Name: HEADER once. |
| H7 | Repeat B1, B5, B7 and B8 after header cases: body behavior is unchanged. |
| L1-L2 | Remove column / Hide column, invoke Name cell; recreate and repeat on header: one callback, zero opens, no fallback. |
| L3 | Swap items on Name suppresses display; Swap items + null opens NATIVE despite mutation. |
| L4 | Focus Mutate on next focus and invoke it, then right-click Name without left-clicking: synchronous GettingFocus replaces items; zero custom requests and zero opens for the configured stale request. GotFocus itself is deferred until after Focus returns and cannot exercise this synchronous gate. |
| B5 + L4 | Reset, Clear row menu, focus Mutate on next focus and invoke it, then right-click City: zero custom requests, one NATIVE open, selection unchanged. Repeat after Reset and Null item row (wait for realizedNull=PASS), targeting the blank City cell in row zero. The null row's Name cell remains configured and must still suppress with zero requests/opens. |
| L5 | Retemplate in callback, invoke Name cell/header: one callback, no show. Choose Keep and invoke fresh target: one show. |
| L6 | Direct row menu gives DIRECT ROW without callback. Nested table then right-click Inner body gives INNER without outer callback; Clear inner menus gives native outer fallback, still no outer callback. |
| L7 | Retemplate three times; scroll past row 150 and back; invoke Name after each: one request/open each, current binding. |
| L8-L9 | Developer-only native debugger cases: same-owner nested request must suppress; throwing from the override must propagate and unwind the guard. These intentionally have no shipping probe/button. |
| A1-A2 | Physical touch-hold and gamepad Menu on body/header: applicable menu once, selection unchanged. Require actual hardware; keyboard is not a substitute. |
| A3 | RTL + frozen Name, horizontal scroll, invoke Name/body/header: inspect target and mirrored platform placement. |
| A4 | Inspect an open menu with UIA/Narrator: CELL presenter and named CELL/current-item entries; only one popup. |
| A5 | Escape from CELL, then F2: row focus returns and Name editing works. |
| A6 | Retain args, invoke Name, Escape, Change retained args: no additional callback/open or late menu. |
| A7 | Plain flyout, Name with Keep / Null / Suppress: PLAIN FLYOUT / NATIVE / none. |
| A8 | Group by City, invoke group header: NATIVE without column/row callback. |
| Null-item regression | Null item row creates an actual null source item at row zero. Wait for realizedNull=PASS; right-click its blank Name/City with Keep / Replace / Null / Suppress and mutation modes. Reset then reapply Null item row between cases. |

These are reproducible diagnostics, not a claim that every hardware/accessibility
case has passed. Known editing limitation: opening the built-in TextBox popup can
cause existing focus-loss commit/teardown before the context-menu pipeline runs.
That pre-existing editor behavior is separate from header eligibility while editing.
The existing frozen-column layout explicitly disables pinning in RTL. The A3
button exposes this limitation: Name can scroll out of view, so combined
RTL/frozen/scroll coverage must not be reported as passed.

## More detail

See [AGENTS.md](AGENTS.md).
