# TableView gallery

A multi-page WinUI 3 desktop gallery that exercises the live public API of
`Microsoft.UI.Xaml.Controls.Tabular.TableView`. It replaces the old single-window Playground
with scenario pages, live examples, options, and embedded source snippets.
The 27 gallery destinations are joined by **Grouped rows** and the existing
**Filter / sort / group** page, for 29 navigation destinations.

Grouping uses the current `TableViewSource.GroupBy` API; row identity comes from the item
objects, not an app-supplied `KeyBy` selector. Selection remains `None` or `Single`;
this integration does not add multi-selection or change the control's visual defaults.

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

## More detail

See [AGENTS.md](AGENTS.md).

## Sample accessibility

The gallery labels its own controls separately from TableView's automation providers.
Visible labels use `AutomationProperties.LabeledBy`; icon-only commands and template
editors have purpose names. Existing content/header-based names remain in use where
they already describe the control. The sample currently authors its UI strings inline;
it does not have a localized string-resource catalog.

In File Explorer, Enter opens the selected folder and Alt+L focuses the folder path.
The performance page identifies each Run button by its scenario and announces completed
results through polite live regions, outside the measured interval. Run all announces
one completion rather than each intermediate result. Table selection, sorting, editing,
and timer-driven telemetry do not receive duplicate sample-level announcements.

Density & read-only uses ordinary two-way TextBox templates, not transactional
`CellEditingTemplate` bindings. The Cell editing page exercises the framework's built-in
text-column editor. Sample labeling does not fix or replace the control's focus,
commit/cancel, or UI Automation event implementation.

A real sample build and runtime checks are still required: keyboard traversal in both
directions, folder activation, screen-reader names and results, high text scaling,
and the control-owned editing/provider scenarios.

For live-data and recycled-row checks, the Virtualization page supports 100, 1,000,
10,000, or 50,000 rows. Select a row and use **Update role**, **Insert row**, or
**Remove row**, then scroll away and back. Updates raise `INotifyPropertyChanged`;
insert/remove mutate the bound `ObservableCollection` without replacing surviving
row objects or IDs. **Reset rows** restores the chosen dataset. Verify actual cell
values, selection, and focus rather than treating the status text as a provider test.
An emptied dataset stays empty when revisiting the page; inserting then continues
the existing ID sequence.

Empty state, Text wrap, Interactive cell flyouts, and Cell editing provide the
other data/template/editing fixtures. Real Windows contrast themes and text scaling
are tested through Windows Settings, not an app-simulated palette. Nested grouping
and multi-selection are not part of this sample's current API scope.

**Template cells is display-only.** Its four embedded pickers/checkboxes are explicitly
disabled, non-tabstop, and OneWay-bound; they must not edit through mouse, keyboard,
or automation. Use Interactive cell flyouts for enabled child controls and Cell editing
for built-in text-column transactions. A successful table-row navigation check does
not prove keyboard navigation to a specific cell or successful editor focus.

The display-only date/time pickers use live, value-bound accessible names with column
context. The date follows the current culture; the time retains the sample's explicit
24-hour clock with the culture's time separator. Native roles and value providers are
not replaced. Actual Narrator row/cell announcements and native Value-pattern behavior
must be tested independently; a correct bound Name is not a speech-test pass.
