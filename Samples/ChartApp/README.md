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

The Charts API this sample consumes ships in `Microsoft.Internal.WinUIDetails`,
which is produced by the repository's shared ("mono") build. Since the mono-build
migration the WinUIDetails version is defined centrally in
[`eng/Versions.props`](../../eng/Versions.props) (`WinUIDetailsNugetVersion`);
there is no longer a per-dependency `eng/Version.Details.xml` to edit, and the
earlier Maestro delivery PR no longer applies.

The sample requires a WinUIDetails build whose `Charts.winmd` includes the full
control surface — `AreaSeries`, `BarSeries`, and the axis types. Packages that
predate those types ship an incomplete `Charts.winmd` and cannot compile this
sample. Build against a WinUIDetails package that supplies matching native
metadata and managed projection, and do not mix metadata from one package with
binaries from another.

> **Note:** the exact supported WindowsAppSDK / WinUIDetails version for the
> shared pipeline is still being confirmed. The validated version and
> smoke-test results for all four hosts will be recorded in the PR once the
> pipeline path is finalized.

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
