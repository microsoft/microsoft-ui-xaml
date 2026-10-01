# Charts engineering sample

ChartApp is an interactive engineering sample for
`Microsoft.UI.Xaml.Controls.Charts`. It ships the same scenarios through four
hosts so both language projections and both deployment shapes are exercised:

| Project | Language | Deployment |
|---|---|---|
| `ChartAppCsUnpackaged` | C# | Unpackaged |
| `ChartAppCsPackaged` | C# | Single-project MSIX |
| `ChartAppCppUnpackaged` | C++/WinRT | Unpackaged |
| `ChartAppCppPackaged` | C++/WinRT | Single-project MSIX |

For exploring Chart features, start with `ChartAppCsUnpackaged`.

## Required package version

The Charts API this sample consumes is delivered through
`Microsoft.Internal.WinUIDetails`. Use **`3.10.0-experimental.20260916.0`**
(Sep 16) or newer. Earlier builds, including the repository's currently pinned
`3.10.0-experimental.20260819.0` (Aug 19), ship an incomplete `Charts.winmd`
(missing `AreaSeries`, `BarSeries`, and the axis types) and cannot compile this
sample. The Sep 16 version arrives via Maestro PR
[microsoft/microsoft-ui-xaml#11952](https://github.com/microsoft/microsoft-ui-xaml/pull/11952);
until it merges, update `eng/Version.Details.xml` locally to that version.

> **Restore gap:** the repository's default `Microsoft.WindowsAppSDK.Foundation`
> `3.0.0-dev.experimental11` packages are not available from a public feed. Build
> against an internal mirror that supplies the matching runtime, resources, and
> projections. Do not mix metadata from one package with binaries from another.

## Building and publishing

The repository `buildsamples.cmd` already builds and publishes every host. To
build a single host directly, initialize the repository, then run `msbuild`
from a developer command prompt (`/restore` pulls the packages; pick your
`Platform`, e.g. `x64`):

```cmd
REM C# unpackaged — build, then publish to emit PRI/XBF resources
msbuild Samples\ChartApp\ChartApp.sln /restore /p:Configuration=Debug /p:Platform=x64
msbuild Samples\ChartApp\ChartAppCsUnpackaged\ChartAppCsUnpackaged.csproj /restore /t:Publish /p:PublishProfile=win-x64.pubxml

REM C# packaged — single-project MSIX only emits its .msix when published
msbuild Samples\ChartApp\ChartAppCsPackaged\ChartAppCsPackaged.csproj /restore /t:Publish /p:PublishProfile=win-x64.pubxml

REM C++/WinRT unpackaged and packaged — built by the solution
msbuild Samples\ChartApp\ChartApp.sln /restore /p:Configuration=Debug /p:Platform=x64
```

Launch unpackaged apps from their complete publish output directory so the
runtime binaries and resources stay together. Packaged apps additionally require
the repository-supported signing and deployment setup. See the repository
[getting-started guide](../../GettingStarted.md) and
[sample-build guidance](../../docs/building/building-sample-apps.md) for the
expected .NET SDK, Visual Studio workloads, and Windows SDK.
