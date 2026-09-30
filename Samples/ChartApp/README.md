# Charts engineering sample

ChartApp is an interactive engineering sample for
`Microsoft.UI.Xaml.Controls.Charts`, with C# and C++/WinRT applications in
packaged and unpackaged forms.

> **Package prerequisite:** this sample requires a WinUI component package that
> contains the Charts API described below, its matching runtime, and its theme
> resources. A supported public package version has not yet been established for
> these scenarios. Do not assume that a package containing only the `Chart` type
> supplies the complete API, or that the repository's default `3.0.0-dev` version
> is available from a remote feed.

## Using the sample

Use the left navigation to choose **Line**, **Area**, **Bar**, **Date & time**,
**Axes & ordering**, **Labels & markers**, or **Live data**. Each page places
the chart preview first, followed by its related controls and readable data.
Navigation preserves the chart instances and the settings you have entered.

| Area | Controls and scenarios |
|---|---|
| Markup charts | Two line series, an area series, and a bar series, using named `Samples` handles. |
| Area appearance | Choose original/blue/green/orange colors and translucent, solid, or outline-only fill. Show/hide the series, value labels, circle markers, or legend. |
| Legend and series | Show/hide the legend, edit its title, select either line series, and change default labels, markers, and their brushes. |
| Individual points | Select an index, add/remove a label or marker override, choose its brush or shape, or clear the selected series' overrides. Selecting another series or index reloads its current settings. |
| Numeric axes | Set or clear the minimum, maximum, and spacing. An empty value restores automatic behavior; invalid input is reported without replacing the current valid setting. |
| Category axes | Choose source-index or category-value ordering and ascending or descending order. |
| Axis presentation | Change visibility, tick labels, tick marks, grid lines, and brush overrides. Choose **Theme** to restore a resource fallback. |
| Bar appearance | Choose original/blue/green/orange colors, show/hide the series, value labels, or legend, and switch between horizontal and vertical layouts without swapping X/Y data dimensions. |
| Code-created chart | Attach a line series before completing its data sources, then supply the observable data. |
| Live data | Update the code-created line and both markup line sources once per second; pause and resume the updates. |
| Date-time charts | Compare 75 daily points with 36 monthly points, each with a target series. Choose Auto/Day/Week/Month/Year intervals and edit the date-label format. |
| Themes | Select Light, Dark, or System and observe sample-level Charts resource overrides. |
| Multiple UI threads | Open, close, and reopen an independently updating chart window on a second UI thread. |

The data is deterministic and synthetic. No network connection or external
data file is required. The primary data summary makes the current values
available as text. Marker shapes, dash styles, titles, and descriptions
supplement color distinctions.

The Area and Bar pages each have a reset button. **Reset area example** restores
the original translucent fill, hides value labels and markers, and shows the
series and legend. **Reset bar example** restores the original colors,
horizontal orientation, visible series and legend, and hidden value labels.
Neither reset replaces the chart, changes its source data, changes another
example, or resets the application theme. Appearance choices survive navigation.

For an outline-only area, the sample sets an explicitly transparent fill color.
A null fill would instead request the chart's palette color. Point-marker colors
follow the area's stroke color when markers are enabled.

## Data and ownership

`Samples` is a source handle, not a collection of point objects. Its `ItemsSource`
references the data collection, and a series' `XValues`/`YValues` reference the
handles. C# uses `ObservableCollection<T>` for fixed data and a small
`ObservableVector<T>` implementation for changing values. It exposes the public
WinRT `IObservableVector<T>.VectorChanged` event explicitly. C++/WinRT uses
`single_threaded_observable_vector<T>`. Updating a value demonstrates collection
notifications without replacing the chart.

The markup chart's axis controls apply to its explicit axes. Other charts own
their axes independently. In particular, the bar series does not share axes with
a differently oriented series.

Point overrides are keyed by absolute index, not a category label. The editors
read the selected series' actual properties and override maps. They do not keep
an independent copy of its presentation state.

Each window's chart objects, collections, and timer remain on their owning UI
thread. Closing a window stops its updates. The secondary UI thread has its own
dispatcher and XAML lifetime; closing the main window shuts that window down as
well.

The time-range controls illustrate the relationship between data density and
tick intervals. A year interval is not useful for a 75-day view, while day/week
ticks can be too dense for a three-year view. The sample displays guidance for
these choices rather than silently selecting another interval.

## Projects

Open `ChartApp.sln` and select the desired startup project:

| Project | Language | Deployment |
|---|---|---|
| `ChartAppCsPackaged` | C# | Single-project MSIX |
| `ChartAppCsUnpackaged` | C# | Unpackaged |
| `ChartAppCppPackaged` | C++/WinRT | Single-project MSIX |
| `ChartAppCppUnpackaged` | C++/WinRT | Unpackaged |

### Why four projects?

This is one engineering sample with a two-language, two-deployment consumer
matrix, not four different feature sets. C# and C++/WinRT exercise different
language projections; packaged and unpackaged applications use different
activation, resource and deployment paths. Keeping all four catches problems
that one host shape alone would miss.

The matrix predates this expansion. The repository's
[TableView sample](../TableViewApp/TableViewApp.sln) uses the same four-project
structure; its [introduction PR](https://github.com/microsoft/microsoft-ui-xaml/pull/11691)
explicitly follows the ChartApp pattern. This is an existing consumer-sample
precedent, not a requirement that every control have four sample applications.

For exploring Chart features, start with `ChartAppCsUnpackaged`. The other
projects demonstrate the same functionality through their respective host
configurations.

Each project merges `XamlControlsResources` and `XamlChartsResources` in
`App.xaml`, followed by sample Light/Dark theme overrides. `MainWindow.xaml`
defines the markup charts and editors; code-behind creates the additional line
and date-time charts and manages their data.

The C# projects share the window and helper source in `ChartAppCsUnpackaged`
through ordinary linked `Page`/`Compile` items. Their application classes,
identities, and deployment settings remain separate.

The unpackaged C# project enables MSIX tooling to generate and publish its
PRI/XBF resources. `WindowsPackageType=None` still keeps the application
unpackaged; disabling the tooling can leave a published executable without its
window resources.

## Build prerequisites

Follow the repository's [getting-started guide](../../GettingStarted.md) and
[sample-build guidance](../../docs/building/building-sample-apps.md). Use the
repository-selected .NET SDK, Visual Studio native/.NET workloads, and Windows
SDK rather than independently upgrading the sample's dependencies.

Before building, obtain a compatible composed `Microsoft.WindowsAppSDK.WinUI`
package through the supported repository workflow. It must provide:

- `Chart.Data`, `Chart.Axes`, and `Chart.Series`;
- `Samples.ItemsSource`, `LineSeries`, `AreaSeries`, and `BarSeries`;
- `CategoryAxis`, `LinearAxis`, `DateTimeAxis`, `DataLabelOverride`,
  `DataMarkerOverride`, presentation properties, and `XamlChartsResources`;
- matching managed/native projections, runtime binaries, resources, and
  activation information for the selected architecture.

The solution preserves the repository's central package versions and project
imports. It does not introduce another package source or override that version
policy. Do not mix metadata from one package with runtime binaries from another.

The existing repository `buildsamples.cmd` already builds `ChartApp.sln` and
publishes both C# projects. Merely building the product, or the default
`Build.cmd` target, does not build the sample. The packaged C# project requires
its publish step to emit an MSIX.

Once a compatible package is available and the repository is initialized, build
the chosen project using its existing configuration and platform settings.
Start with x64; project configurations alone are not a claim that every listed
architecture has been exercised. Packaged apps additionally require the
repository-supported signing and deployment setup. Launch unpackaged apps from
their complete output directory so that runtime binaries and resources remain
together.

## Scope and limitations

This is an engineering sample, not a chart-validation harness. It does not assert
rendering, numerical, performance, or accessibility conformance.

Chart-specific per-point automation and keyboard interaction are not supplied by
this sample. Accessible chart descriptions and a readable data table supplement
the visual examples.

The sample does not add chart types or chart-specific zoom/selection APIs.
Its controls configure the available public Charts surface. It does not contain
a test-result dashboard or generate test-result files.

See the [Charts API proposal](https://github.com/microsoft/microsoft-ui-xaml/pull/11769)
for the broader proposed API; proposal status does not establish package or
release availability.
