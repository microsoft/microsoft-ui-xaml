# TableView sample — build & setup notes (for agents and maintainers)

This document explains **how** the sample is built and **why** each non-obvious step exists. The
end-user quick start lives in [README.md](README.md); this file is the deep reference for anyone
(human or automation) that needs to reproduce, debug, or maintain the build.

## Why this sample is unusual

`TableView` currently ships as a **split binary**: the control lives in
`Microsoft.UI.Xaml.Controls.Tabular.dll`, separate from the main framework DLL, and it is **not
yet exposed through the WindowsAppSDK NuGet package**. A normal WinUI app can't just add a package
reference and use it. To consume the locally built control, the sample:

1. links the freshly built native control DLL,
2. regenerates its WinRT projection from the freshly built WinMD, and
3. reconstructs, at build and startup time, the pieces the WindowsAppSDK packaging would normally
   provide (activatable-class registrations, theme resources, default styles).

Every workaround below disappears once `TableView` is delivered in-box through WindowsAppSDK.

## Environment setup

1. Install Visual Studio 2022 with the **Desktop development with C++** and **.NET Desktop**
   workloads.
2. From the repo root, run a full initialization once so packages are restored and the build
   environment is provisioned:

   ```
   .\init.cmd
   ```

   For one-shot commands in an already-initialized enlistment, `initrun.ps1 <command>` sets up the
   environment for a single invocation without persisting a shell.

## Step 1 — Build the Tabular control

```
.\initrun.ps1 controls\Build.cmd tabular
```

Produces:

- `BuildOutput\obj\amd64chk\controls\dev\dll-tabular\Microsoft.UI.Xaml.Controls.Tabular.dll`
- `BuildOutput\obj\amd64chk\controls\dev\dll-tabular\Merged\*.winmd` (the metadata the sample
  projects against)
- `BuildOutput\obj\amd64chk\controls\dev\dll-tabular\generic.xaml` (the default Style /
  ControlTemplate slice the sample compiles)

The sample consumes these outputs directly, so the control must be built **before** the sample.

## Step 2 — Build the sample

```
.\initrun.ps1 msb /q /restore Samples\TableViewSampleApp\TableViewSampleApp.csproj /p:Platform=x64 /p:RuntimeIdentifier=win-x64
```

The project (`TableViewSampleApp.csproj`) performs several workarounds during this build. Each
is described below with the failure it prevents.

### a. Stage the control DLL next to the executable

The native `Microsoft.UI.Xaml.Controls.Tabular.dll` is copied next to the app. TableView's
runtime classes are registered (see step *e*) but cannot activate without the DLL present.

*Prevents:* `CLASS_E_CLASSNOTAVAILABLE` (`0x80040111`) when a `TableView` is first activated.

### b. Regenerate the CsWinRT projection from the built WinMD

The mock WindowsAppSDK projection can carry stale `Microsoft.UI.Xaml.Controls.Tabular` metadata.
When the built control's interface IIDs diverge from the stale projection, calls such as
`TableView.get_Columns` fail their `QueryInterface`. The project regenerates the projection with
CsWinRT from the freshly built WinMD (`CsWinRTFilters` include the
`Microsoft.UI.Xaml.Controls.Tabular` namespace and the Tabular XAML metadata provider types) so
the projected IIDs match the control exactly.

*Prevents:* `E_NOINTERFACE` from stale metadata.

### c. Include the TableView theme resources — sourced from the control, never checked in

The split binary's theme resources are **not** deployed to consuming apps, so the sample compiles
and merges them itself. To guarantee they can never drift from the control, the sample references
the canonical sources directly instead of checking in copies:

- `TabularSurfaces_themeresources.xaml` ← `controls\dev\CommonStyles\TabularSurfaces_themeresources.xaml`
- `TableView_themeresources.xaml` ← `controls\dev\TableView\TableView_themeresources.xaml`
- `generic.xaml` (default `Style` + `ControlTemplate`) ← the freshly built
  `BuildOutput\...\dll-tabular\generic.xaml`

These are wired via `<Page>`/`<Content>` items with `<Link>` metadata (see the theme-resources
`ItemGroup` in the csproj), app-compiled to the same ms-appx paths the app expects, and merged at
startup by `App.xaml` / `App.xaml.cs`.

*Prevents:* the control rendering unstyled or blank; its native `MeasureOverride` throwing on
first layout when the template is missing.

### d. Build self-contained and unpackaged

The project sets `WindowsAppSdkSelfContained=true`, `WindowsPackageType=None`. This bundles the
WindowsAppRuntime framework natives and emits the in-process-server activation entries that the
packaged bootstrap would otherwise inject.

*Prevents:* startup failure (`0xC000027B`) on machines lacking a matching framework package. This
is standard WindowsAppSDK behavior for unpackaged apps, not a defect.

### e. Merge the InteractiveExperiences (IXP) app manifest

`Build\MergeIxpAppManifest.ps1` merges the WindowsAppSDK InteractiveExperiences component package
`appxfragment` into the app's side-by-side manifest, adding the lifted-WinRT activatable-class
registrations (CoreMessaging, Dispatching, Input, Windowing, etc.) that the mock aggregator's
runtime MSIX omits. It also emits registrations for the split Tabular runtimeclasses (enumerated
from the built Tabular WinMDs), and stages the per-RID native component DLLs next to the executable.

*Prevents:* `REGDB_E_CLASSNOTREG` at startup when `DispatcherQueueController` is activated, and
`CLASS_E_CLASSNOTAVAILABLE` when the Tabular metadata provider / `TableView` is activated.

### f. Register the Tabular metadata provider and merge styles at runtime

`App.xaml.cs`:

- Injects the Tabular DLL's XAML metadata provider into the generated app provider's
  `OtherProviders` (after `InitializeComponent`) so a runtime `XamlReader.Load` can resolve
  split-only types such as `TabularControlsResources`. If the provider can't be loaded, the sample
  skips the explicit resource merges rather than proceeding without it.
- Defers merging `TabularControlsResources` and the control's `generic.xaml` styles to
  `OnLaunched`, because `Application.Resources` is not reachable from the `App` constructor in this
  self-contained split configuration (accessing it early throws `E_UNEXPECTED` / `0x8000FFFF`).

### g. Seed the internal template part into XamlTypeInfo

`_SplitTypeSeed.xaml` references `TableViewRow` inside a never-loaded `DataTemplate` so the XAML
compiler emits an app-level `XamlTypeInfo` entry for that internal template part.

*Prevents:* "type `TableViewRow` not found" at inflation time (`0xC000027B`).

### Type disambiguation in code

`MainWindow.xaml.cs` uses `using` aliases to bind `TableView*` names to the real
`Microsoft.UI.Xaml.Controls.Tabular.*` types, disambiguating them from the stale mock
`Microsoft.UI.Xaml.Controls.*` projection that the mock framework DLL still carries.

## Step 3 — Run and verify

Launch:

```
BuildOutput\obj\amd64chk\Samples\TableViewSampleApp\TableViewSampleApp.exe
```

The app writes a diagnostic log next to the executable, `tabular-split-diag.txt`. A healthy
startup looks like:

```
[TabularSplit] Tabular metadata provider registered; OtherProviders count = 2.
[TabularSplit] Merge: XamlReader.Load OK; TabularControlsResources constructed natively.
[TabularSplit] Merge: TabularControlsResources merged; count = 4.
[TabularSplit] Merge: Tabular control styles merged; count = 5.
```

If the window appears and the log shows the four lines above with no `THREW`/`UnhandledException`
entries, the split-binary consumption is working.

## Configuration → flavor mapping

The project maps `Configuration` to the matching native control flavor so it stages the correct
bits:

- `Debug` → `amd64chk`
- `Release` → `amd64fre`

Build the Tabular control in the flavor that matches the configuration you build the sample in.

## Mock WindowsAppSDK package version

Samples that consume the locally built split-binary controls need a mock `WindowsAppSdkPackageVersion`
so the app resolves the local mock NuGet layout. This is defined **in the sample's own csproj**, so
adding the sample requires no repo-wide build changes.

## Known issue

An `EmptyTemplate` containing a `FontIcon` can crash at startup while the empty state is first
shown. Prefer text or shape content in the `EmptyTemplate` until this is resolved.
