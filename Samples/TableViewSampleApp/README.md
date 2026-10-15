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
  table shows per-row-kind context menus without a `RowStyleSelector`: one shared row menu set
  through an implicit `TableViewRow` style, whose `Opening` handler keeps the items tagged for the
  row's kind (leaf or expandable, read from `MenuFlyout.Target`); an implicit `TableViewGroupHeader`
  style gives group headers their own menu and restyles the band, and the leaf items bind
  `Command` / `Visibility` to the row's item
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
row by `TableView.RowStyleSelector`; a menu's `Opening` handler can also vary its items
per row through `MenuFlyout.Target` (see the Hierarchy page). Column cell menus
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

## More detail

See [AGENTS.md](AGENTS.md).
