# Charts engineering sample

ChartApp is an interactive engineering sample for
`Microsoft.UI.Xaml.Controls.Charts`. The same scenarios run in four hosts:

| Project | Language | Deployment |
|---|---|---|
| `ChartAppCsUnpackaged` | C# | Unpackaged |
| `ChartAppCsPackaged` | C# | Single-project MSIX |
| `ChartAppCppUnpackaged` | C++/WinRT | Unpackaged |
| `ChartAppCppPackaged` | C++/WinRT | Single-project MSIX |

Start with `ChartAppCsUnpackaged`.

## Package requirement

Charts ships in `Microsoft.Internal.WinUIDetails`. This sample needs a
WinUIDetails build that includes the Charts types it uses, such as
`3.10.0-experimental.20260916.0`. The version is set as
`WinUIDetailsNugetVersion` in [`eng/Versions.props`](../../eng/Versions.props).
Maestro no longer updates it, so it is bumped manually. The version pinned for
public builds does not include Charts yet, and the GitHub pull-request build
skips this sample.

## Building

`buildsamples.cmd` builds all four hosts and publishes the C# hosts. To build
directly from an initialized developer command prompt:

```cmd
msbuild Samples\ChartApp\ChartApp.sln /restore /p:Configuration=Debug /p:Platform=x64
msbuild Samples\ChartApp\ChartAppCsUnpackaged\ChartAppCsUnpackaged.csproj /restore /t:Publish /p:PublishProfile=win-x64.pubxml
msbuild Samples\ChartApp\ChartAppCsPackaged\ChartAppCsPackaged.csproj /restore /t:Publish /p:PublishProfile=win-x64.pubxml
```

Publishing emits the unpackaged C# app's PRI/XBF resources and the packaged C#
app's .msix. Run unpackaged apps from their output folder. To build against a
specific WinUI package, see
[building sample apps](../../docs/building/building-sample-apps.md).
