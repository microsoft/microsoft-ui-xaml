# TableView consumer matrix

Four minimal apps that consume `TableView` the way a real customer would — **C# and C++, packaged and
unpackaged** — to prove the control works in every host shape without app-side workarounds.

| | Packaged | Unpackaged |
|---|---|---|
| **C#** | `TableViewAppCsPackaged` | `TableViewAppCsUnpackaged` |
| **C++** | `TableViewAppCppPackaged` | `TableViewAppCppUnpackaged` |

Modelled on the [ChartApp](../ChartApp) samples, which are the existing pattern in this repo for a
separately-built control set.

## What they prove

Each app references `Microsoft.WindowsAppSDK.WinUI` **and nothing else**. `App.xaml` merges
`XamlControlsResources` plus `TabularControlsResources`. There is no projection regeneration, no
manifest injection, no theme resources compiled from the control source, no staged DLL, and no type
seed — the control resolves its own default style and theme resources from the package.

`TabularControlsResources` is Tabular's own theme-resource dictionary, the exact analogue of
`XamlControlsResources` for MUXC, and merging it is a normal part of consuming the control set
rather than a workaround. It is required: `TableView`'s column-header style resolves
`SortIndicatorForeground` from it, and `SortIndicator` ships in the Tabular DLL rather than in MUXC
(`controls/Tabular.ProjectImports.targets` is the only importer of `SortIndicator.vcxitems`).
Without the merge the app throws `XamlParseException 0x802B000A` — "Cannot find a Resource with the
Name/Key SortIndicatorForeground" — during the first layout pass, which surfaces as a
`0xC000027B` stowed exception a few seconds after launch.

Each app instantiates `TableView` **twice on purpose**:

- once from **markup**, which exercises XamlTypeInfo, the metadata provider and XAML-driven activation;
- once from **code-behind**, which exercises direct WinRT activation with no markup involved.

Those are separate paths and either can fail alone, so a conformance check wants both.

## Build and run

`TableView` is not in any published `Microsoft.WindowsAppSDK.WinUI` package yet, so these apps only
compile against a locally packed component — `Build.cmd samples` packs it and passes the matching
version:

```
.\Build.cmd product
.\Build.cmd samples
```

Building one project on its own resolves the package from the feed instead and fails with
`CS0234: The type or namespace name 'Tabular' does not exist`. To iterate on a single app, pack once
and override the version:

```
.\pack.component.cmd /version 3.0.0-mylocal
.\initrun.ps1 msb /restore Samples\TableViewApp\TableViewAppCsPackaged\TableViewAppCsPackaged.csproj /p:Platform=x64 /p:WinUIVersion=3.0.0-mylocal
```

NuGet caches by version, so re-pack under a fresh version string after changing the control.

The unpackaged apps run straight from their output directory. The packaged apps produce a loose
layout; register it with `Add-AppxPackage -Register <outdir>\AppxManifest.xml` and launch from Start.

## The C# and C++ apps bind identically

All four apps use the same markup and the same code-behind shape: `TableViewTextColumn.Binding`
with a classic `{Binding}`. Keeping the matrix identical is the point — a difference between the
cells would make it unclear whether a rendering difference came from the language, the packaging,
or the control.

Classic `{Binding}` resolves properties through reflection in .NET, but C++/WinRT has no
reflection: it needs an `IXamlType`, which the XAML compiler emits only for types it encounters
**in markup**, or an `ICustomPropertyProvider` implementation. A `Person` built purely from code
has neither, so `{Binding}` would silently resolve nothing — every cell renders empty while
headers, rows, grid lines and theming all look perfectly correct.

`[bindable]` on the `Person` runtimeclass in `MainWindow.idl` is what closes that gap: it makes
C++/WinRT generate the `ICustomPropertyProvider` implementation, so the same `{Binding}` markup
works in both languages. If a C++ consumer prefers compile-time binding, `x:Bind` inside a
`TableViewTemplateColumn.CellTemplate` remains a valid alternative that needs no attribute.

## Preview-API warnings are left visible here

`TableView` is `[MUX_PREVIEW]`, so C# usage raises `CS8305` and XAML usage raises `WMC1501`
("for evaluation purposes only"). These apps **do not** suppress them: they are small, the warning
count stays low, and a consumer reading a conformance sample should see that the API is preview.

`TableViewSampleApp` takes the opposite choice deliberately — it exercises the whole API surface, so
un-suppressed `CS8305` fires 324 times and would bury every other warning. That difference is
intentional, not an oversight.
