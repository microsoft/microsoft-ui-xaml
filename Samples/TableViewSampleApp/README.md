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

From the repo root:

```
.\initrun.ps1 msb /q /restore Samples\TableViewSampleApp\TableViewSampleApp.csproj /p:Platform=x64
```

The sample resolves `Microsoft.WindowsAppSDK.WinUI` at `$(WinUIVersion)`. To run against a locally
built control, pack the component and point the build at that version:

```
.\pack.component.cmd /version 3.0.0-mylocal
.\initrun.ps1 msb /q /restore Samples\TableViewSampleApp\TableViewSampleApp.csproj /p:Platform=x64 /p:WinUIVersion=3.0.0-mylocal
```

## Run

```
BuildOutput\obj\amd64chk\Samples\TableViewSampleApp\TableViewSampleApp.exe
```

## Using TableView in your own app

Two things, both ordinary:

1. Reference `Microsoft.WindowsAppSDK.WinUI`.
2. Merge the control's theme dictionary alongside `XamlControlsResources`, so its brushes and
   metrics are in scope:

   ```xml
   <Application.Resources>
     <ResourceDictionary>
       <ResourceDictionary.MergedDictionaries>
         <XamlControlsResources xmlns="using:Microsoft.UI.Xaml.Controls" />
         <TabularControlsResources xmlns="using:Microsoft.UI.Xaml.Controls.Tabular" />
       </ResourceDictionary.MergedDictionaries>
     </ResourceDictionary>
   </Application.Resources>
   ```

   Without the second dictionary the control still gets its template, but the column header row and
   the theme brushes will be missing.

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

## Known issue

Using an `EmptyTemplate` that contains a `FontIcon` can crash at startup while the empty state is
first shown. Prefer text or shape content in the `EmptyTemplate` until this is resolved.

## More detail

See [AGENTS.md](AGENTS.md).
