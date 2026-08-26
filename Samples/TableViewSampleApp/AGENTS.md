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

```
.\initrun.ps1 msb /q /restore Samples\TableViewSampleApp\TableViewSampleApp.csproj /p:Platform=x64
```

`Microsoft.WindowsAppSDK.WinUI` resolves at `$(WinUIVersion)` through Central Package Versions. To
test a locally built control, pack the component and override that version:

```
.\pack.component.cmd /version 3.0.0-mylocal
... /p:WinUIVersion=3.0.0-mylocal
```

The control DLL, its `.pri` and its theme XBFs arrive from the package. The consuming app's build
expands every referenced `.pri` and re-indexes it into the app's own `TableViewSampleApp.pri`, which
is why that file is several megabytes: it contains MUXC's and Tabular's resources as well as the
sample's.

## Two things worth knowing

**Nothing needs merging.** The control resolves its own `generic.xaml` and theme resources from the
package, so `App.xaml` merges only `XamlControlsResources`. Verified by removing every Tabular
dictionary from this sample and confirming all eight column headers and the rows still render, and
again by the `TableViewApp` matrix (C#/C++ x packaged/unpackaged), none of which merge anything
Tabular-specific. If a table appears with no header row, the cause is almost always **no declared
columns**, not a missing dictionary.

**`TableView` is `[MUX_PREVIEW]`.** C# usage raises `CS8305`, suppressed via `NoWarn` in the project
file. XAML usage raises `WMC1501` once per page; those are deliberately left visible.

## Entry point

`Program.cs` provides `Main` and the project defines `DISABLE_XAML_GENERATED_MAIN`, following the
[DisableXamlGeneratedMain](../DisableXamlGeneratedMain) sample. This is a normal WinUI pattern, not a
workaround.

## Known issue

An `EmptyTemplate` containing a `FontIcon` can crash at startup while the empty state is first shown.
Prefer text or shape content in the `EmptyTemplate` until this is resolved.
