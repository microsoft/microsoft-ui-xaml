# Chart review app

ChartReviewApp is the test app used for the UX, craft and accessibility reviews of
`Microsoft.UI.Xaml.Controls.Charts`. It starts from the C# unpackaged host of the
[Charts engineering sample](../ChartApp/README.md) and adds pages that reviewers need.

| Page | What it is for |
|---|---|
| Line, Area, Bar, Date & time, Axes & ordering, Labels & markers, Live data | The engineering sample's pages: change series, axes, labels and markers and watch the chart update |
| Design reference | The eight chart frames from the Fluent design, drawn with product defaults only, for side-by-side comparison |
| Accessibility | Cards A1 to A6: generated and app-supplied names and descriptions, axis titles in point values, dates, a hidden series, a disabled chart |
| Scenarios (30) | The 30 accessibility assessment scenarios (`scenarios.json`), each with fixed data; live scenarios have a Live updates toggle, Step and Reset |
| About this build | Build information and the current theme, contrast theme, text size and display scale, with a button that copies them for a bug report |

The editor pages are accessible themselves: card titles are headings, related controls are named
groups, status changes are announced, and layouts widen and stack with the Windows text size.

## Package requirement

The same as the [Charts engineering sample](../ChartApp/README.md#package-requirement): Charts ships in
`Microsoft.Internal.WinUIDetails`, and the version pinned for public builds does not include Charts
yet. This sample is not part of `buildsamples.cmd`, so pull-request builds do not build it.

## Building

From an initialized developer command prompt:

```cmd
msbuild Samples\ChartReviewApp\ChartReviewApp\ChartReviewApp.csproj /restore /t:Publish /p:Configuration=Release /p:Platform=x64 /p:PublishProfile=win-x64.pubxml
```

Run `ChartReviewApp.exe` from the publish folder.

## Command-line options

| Option | Use |
|---|---|
| `--page=<name>` | Start page: `line`, `area`, `bar`, `datetime`, `axes`, `presentation`, `live`, `design`, `accessibility`, `scenarios`, `about` |
| `--scenario=<id>` | Open one of the 30 scenarios, for example `--scenario=keyboard-before-after-chart` |
| `--theme=light` or `--theme=dark` | App theme |

Test hooks, for automation rather than reviewers:

| Option | Use |
|---|---|
| `--focus-walk=<n>[:<AutomationId>]` | Moves focus n times the way Tab does, logs each stop to `%TEMP%\ChartsReviewApp-focus.txt`, then focuses the named element |
| `--dump-descriptions=<path>` | Opens every scenario, writes each chart's name, role and full description, then exits |
| `--window-size=<width>x<height>` | Window size in physical pixels, beyond the screen if needed, to check layouts for larger displays |
| `--check-clipping=<path>` | Opens every page and lists single-line text that needs more width than it has, then exits |
| `--dump-series-colors=<path>` | Rewrites `<path>` twice a second with the app-set series colors of the Line, Area and Bar charts and the current surface, to check contrast without a screen capture |

If the app closes unexpectedly, the exception is written to `%TEMP%\ChartsReviewApp-crash.txt`. If the Chart
DLL or its `.pri` is missing next to the app, the app says which file is missing in that file and in a
message box, then exits.

## Build information

About this build reads an optional `BuildInfo.json` next to the project (copied to the output when
present) to show which Charts build a review package used. A plain build shows `unknown` for those
fields; the loaded Charts DLL path is always shown.
