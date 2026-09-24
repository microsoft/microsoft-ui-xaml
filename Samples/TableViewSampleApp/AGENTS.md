# TableView sample — build notes

The end-user quick start is in [README.md](README.md). This file records how the sample is wired
and why, for anyone maintaining it.

## There is nothing unusual here any more

The sample used to hand-wire around the product, because `TableView`'s type information was withheld
from the public winmd and its theme resources did not resolve in a consuming app. It linked raw build
output, regenerated its own CsWinRT projection, injected a XAML metadata provider, seeded an internal
template part into `XamlTypeInfo`, compiled the control's theme resources out of the control source
tree, merged activatable-class registrations into its own app manifest, and staged the control DLL
next to the EXE.

None of that is needed. The project file is now ~60 lines and the sample is an ordinary package
consumer, modelled on the [ChartApp](../ChartApp) samples — the existing pattern in this repo for a
separately-built control set.

If you find yourself reaching for one of those workarounds again, that is a signal the product
regressed. Check, in order:

1. Tabular types are in the public merged winmd (`MergedWinMD`), not gated out.
2. `<ActivatableClass>` registrations are emitted — they derive from the same merge inputs, so a
   withheld winmd silently removes them too.
3. The control's theme XBFs ship in the package and its default-style URI is authority-less
   `ms-appx:///`, matching the path in `AppxPriInitialPath`.

## How it builds

`TableView` is not in any published `Microsoft.WindowsAppSDK.WinUI` package yet, so the sample only
compiles against a locally packed component. `Build.cmd samples` handles that — it runs
`pack.component.cmd` and then builds every sample with the matching `/p:WinUIVersion`:

```
.\Build.cmd product
.\Build.cmd samples
```

Building the project on its own resolves `Microsoft.WindowsAppSDK.WinUI` from the feed instead, and
fails with `CS0234: The type or namespace name 'Tabular' does not exist` — the package is real, it
just predates Tabular in the public winmd. To iterate on this app alone, pack and override:

```
.\pack.component.cmd /version 3.0.0-mylocal
.\initrun.ps1 msb /q /restore Samples\TableViewSampleApp\TableViewSampleApp.csproj /p:Platform=x64 /p:WinUIVersion=3.0.0-mylocal
```

NuGet caches by version under `packages\microsoft.windowsappsdk.winui\`, so re-packing the same
version after a control change is silently ignored — use a fresh version string, or delete that
folder first.

The control DLL, its `.pri` and its theme XBFs arrive from the package. The consuming app's build
expands every referenced `.pri` and re-indexes it into the app's own `TableViewSampleApp.pri`, which
is why that file is several megabytes: it contains MUXC's and Tabular's resources as well as the
sample's.

## Two things worth knowing

**Tabular's theme resources must be merged.** The control resolves its own `generic.xaml` from the
package, but the theme resources that default style depends on live in `TabularControlsResources`,
so `App.xaml` merges that alongside `XamlControlsResources`. `SortIndicator` ships in the Tabular
DLL rather than in MUXC — `controls/Tabular.ProjectImports.targets` is the only importer of
`SortIndicator.vcxitems` — so `SortIndicatorForeground`, which `TableView`'s column-header style
resolves, is absent from MUXC's dictionary. Dropping the merge produces
`XamlParseException 0x802B000A` on the first layout pass, seen as a `0xC000027B` stowed exception
shortly after launch. If a table appears with no header row, that is a different problem: it is
almost always **no declared columns**, not a missing dictionary.

**`TableView` is `[MUX_PREVIEW]`.** C# usage raises `CS8305`, suppressed via `NoWarn` in the project
file. XAML usage raises `WMC1501` once per page; those are deliberately left visible.

## Entry point

`Program.cs` provides `Main` and the project defines `DISABLE_XAML_GENERATED_MAIN`, following the
[DisableXamlGeneratedMain](../DisableXamlGeneratedMain) sample. This is a normal WinUI pattern, not a
workaround.
