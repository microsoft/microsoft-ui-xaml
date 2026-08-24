# TableView sample

A small, self-contained WinUI 3 desktop app that exercises the live public API of the
`Microsoft.UI.Xaml.Controls.Tabular.TableView` control. The left panel lets you tweak columns,
sizing, headers, grid lines, density, backgrounds, and more while the table updates in real time.

`TableView` currently ships as a **split binary** (`Microsoft.UI.Xaml.Controls.Tabular.dll`) that
is not yet available through the WindowsAppSDK NuGet package, so this sample links the locally built
control and needs a few build workarounds (described below). They all disappear once the control
ships in-box. Deeper architecture notes live in [AGENTS.md](AGENTS.md).

## Prerequisites

- Visual Studio 2022 with the **Desktop development with C++** and **.NET Desktop** workloads.
- A full repo initialization has been run once from the repo root:

  ```
  .\init.cmd
  ```

## Build

From the repo root:

**1. Build the Tabular control** — produces `Microsoft.UI.Xaml.Controls.Tabular.dll` and its WinMD,
which the sample consumes:

```
.\initrun.ps1 controls\Build.cmd tabular
```

**2. Build the sample:**

```
.\initrun.ps1 msb /q /restore Samples\TableViewSampleApp\TableViewSampleApp.csproj /p:Platform=x64 /p:RuntimeIdentifier=win-x64
```

### What the sample project does for you

Because `TableView` is a split binary that isn't in WinAppSDK/NuGet yet, the project automates a few
workarounds during build:

- **Stages `Microsoft.UI.Xaml.Controls.Tabular.dll` next to the EXE.**
  *Why:* the control is a separate binary; without the DLL, activation fails with
  `CLASS_E_CLASSNOTAVAILABLE`.
- **Regenerates the CsWinRT projection from the freshly built WinMD.**
  *Why:* keeps the generated projection in sync with the built control and avoids `E_NOINTERFACE`
  caused by stale metadata.
- **Includes the TableView theme resources, sourced directly from the control.**
  *Why:* the split binary doesn't deploy its theme resources to consuming apps; without them the
  control renders unstyled or blank. The sample references the control's resources at their
  canonical locations (and the built `generic.xaml`) rather than checking in copies, so they can
  never drift from the control.
- **Builds as a self-contained, unpackaged app** (`WindowsAppSdkSelfContained=true`,
  `WindowsPackageType=None`).
  *Why:* bundles the required Windows App Runtime dependencies for unpackaged execution.
- **Merges the InteractiveExperiences app manifest.**
  *Why:* registers the activatable classes missing from the local mock package; without it, startup
  fails with `REGDB_E_CLASSNOTREG`.

## Run

The self-contained, unpackaged executable is produced at:

```
BuildOutput\obj\amd64chk\Samples\TableViewSampleApp\TableViewSampleApp.exe
```

Launch it directly — the required control DLL and runtime dependencies are staged alongside it.

## What it demonstrates

The left panel exercises these live-tweakable API surfaces:

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

For the full build architecture — why the projection is regenerated, why the app is built
self-contained and unpackaged, how the theme resources and control DLL are staged, and how the
app manifest is augmented — see [AGENTS.md](AGENTS.md).
