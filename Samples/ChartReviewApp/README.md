# Chart review app

ChartReviewApp is the test app used for the UX, craft and accessibility reviews of
`Microsoft.UI.Xaml.Controls.Charts`. It starts from the C# unpackaged host of the
[Charts engineering sample](../ChartApp/README.md) and adds pages that reviewers need.

| Page | What it is for |
|---|---|
| Line, Area, Bar, Date & time, Axes & ordering, Labels & markers, Live data | Adapted copies of the engineering sample's pages (see [Differences from the engineering sample](#differences-from-the-engineering-sample)): change series, axes, labels and markers and watch the chart update |
| Design reference | The eight chart frames from the Fluent chart design, for side-by-side comparison with the design. Colors, sizes and line thickness are product defaults; the app sets the data, the axes (including major grid lines on the value axis) and the orientation |
| Accessibility | Cards A1 to A6: generated and app-supplied names and descriptions, axis titles in point values, dates, a hidden series, a disabled chart |
| Scenarios (30) | 30 accessibility test scenarios (`scenarios.json`), each with fixed data; live scenarios have a Live updates toggle, Step and Reset |
| About this build | Build information and the current theme, contrast theme, text size and display scale, with a button that copies them for a bug report |

The editor pages are accessible themselves: card titles are headings, related controls are named
groups, status changes are announced, and layouts widen and stack with the Windows text size.

## Differences from the engineering sample

`MainWindow.xaml`, `MainWindow.xaml.cs`, `SecondaryChartWindow.cs` and `ObservableVector.cs` started as copies
of `ChartAppCsUnpackaged` and are not linked to it, so a fix in one does not reach the other. On purpose, this app:

- Has no Charts brush overrides in `App.xaml`, so reviewers see the control's own defaults, including in contrast themes.
- Sizes its windows in DIPs rather than physical pixels.
- Switches app-set series and axis colors to dark-surface variants in Dark and dark contrast themes.
- Refuses Y-axis settings that would draw more than 200 ticks.
- Shows data lists one text element per line, so Narrator reads each line on its own.
- Carries the accessibility fixes listed above (headings, named groups, announcements, focus).

Like `ChartAppCsUnpackaged`, the window code keeps the engineering sample's `ChartsSample` namespace, and
only `App` is in the project's own namespace, `ChartReviewApp`.

## Package requirement

The same as the [Charts engineering sample](../ChartApp/README.md#package-requirement): Charts ships in
`Microsoft.Internal.WinUIDetails`, and the sample needs a WinUIDetails build that includes Charts, set as
`WinUIDetailsNugetVersion` in [`eng/Versions.props`](../../eng/Versions.props). The version pinned for public
builds does not include Charts yet.

This sample is not built by `buildsamples.cmd` or the sample-app pipeline, so no pull-request or CI build
catches a break when the Charts API changes. After such a change, follow [How to validate](#how-to-validate).

## Building

From an initialized developer command prompt:

```cmd
msbuild Samples\ChartReviewApp\ChartReviewApp\ChartReviewApp.csproj /restore /t:Publish /p:Configuration=Release /p:Platform=x64 /p:PublishProfile=win-x64.pubxml
```

The publish profiles write to `$(ArtifactsBinDir)Samples\ChartReviewApp\`, which is
`BuildOutput\bin\amd64fre\Samples\ChartReviewApp\` for the command above (the `amd64fre` part follows the
platform and configuration, for example `x86fre`, `arm64fre` or `amd64chk` for Debug). Run `ChartReviewApp.exe`
from that folder.

If the local WinUI build does not include Charts, the restore or the compile fails (NU1102 for
`Microsoft.WindowsAppSDK.WinUI` 3.0.0-dev, or CS0234 for `Microsoft.UI.Xaml.Controls.Charts`). To build against
an experimental `Microsoft.WindowsAppSDK.WinUI` package that includes Charts instead, add
`/p:WinUIVersion=<version>` and, if the package is not on a configured feed,
`/p:RestoreAdditionalProjectSources=<folder>`.

## How to validate

1. Build with the command above. It must finish with no errors. The XAML compiler reports WMC1501 warnings
   (evaluation only) for the Charts types; the matching C# warning, CS8305, is suppressed by the project.
2. Run `ChartReviewApp.exe`. The Line page opens with the Monthly profit and expenses chart drawn.
3. Select each page in the navigation pane: every page shows its charts with no error in the status bar.
4. On Scenarios (30), open a scenario such as Live updating series, turn on Live updates and check that
   the chart and the Updates count change, then select Reset.
5. On Live data, select Toggle secondary UI thread: a second window opens with its own updating chart.
   Close it, then close the app; `%TEMP%\ChartReviewApp-crash.txt` gets no new entry.

## Command-line options

| Option | Use |
|---|---|
| `--page=<name>` | Start page: `line`, `area`, `bar`, `datetime`, `axes`, `presentation`, `live`, `design`, `accessibility`, `scenarios`, `about` |
| `--scenario=<id>` | Open one of the 30 scenarios, for example `--scenario=keyboard-before-after-chart` |
| `--theme=light` or `--theme=dark` | App theme |

Test hooks, for automation rather than reviewers:

| Option | Use |
|---|---|
| `--focus-walk=<n>[:<AutomationId>]` | Moves focus n times the way Tab does, logs each stop to `%TEMP%\ChartReviewApp-focus.txt`, then focuses the named element |
| `--dump-descriptions=<path>` | Opens every scenario, writes each chart's name, role and full description, then exits |
| `--window-size=<width>x<height>` | Window size in physical pixels, beyond the screen if needed, to check layouts for larger displays |
| `--check-clipping=<path>` | Opens every page and lists single-line text that needs more width than it has, then exits |
| `--dump-series-colors=<path>` | Rewrites `<path>` twice a second with the app-set series, label, marker and override colors of the Line, Area, Bar and code-created charts and the current surface, to check contrast without a screen capture |

If the app closes unexpectedly, the exception is written to `%TEMP%\ChartReviewApp-crash.txt`. A test hook
that fails, for example because it cannot write its output path, writes the error there too. If the Chart
DLL or its `.pri` is missing next to the app, the app says which file is missing in that file and in a
message box, then exits.

## Build information

About this build reads an optional `BuildInfo.json` next to the project (copied to the output when
present) to show which Charts build a review package used. A plain build shows `unknown` for those
fields; the loaded Charts DLL path is always shown.
