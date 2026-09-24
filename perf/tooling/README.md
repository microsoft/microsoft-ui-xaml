# perf/tooling — XAML launch-telemetry tools

Tools for analysing **ETW (`.etl`) traces of WinUI 3 / Windows App SDK application launches**. Two front-ends share one core library:

| Project | Folder | Output | What it is |
|---|---|---|---|
| **XamlTimeline** | [`XamlTimeline/`](XamlTimeline/) | `netstandard2.0` class library | **Shared core** — event model, region parsing, and Track 1 timeline detection used by both front-ends. No UI, no external NuGet dependencies. |
| **XamlTelemetryViewerWInui3** | [`XamlTelemtryViewerWInui3/`](XamlTelemtryViewerWInui3/) | `net8.0-windows` WinUI 3 app | Standalone desktop viewer — load **multiple** `.etl` traces side-by-side (tabbed grid + stacked timeline lanes), filter, and explore a zoomable multi-track timeline; light/dark theme toggle. |
| **AppBootingVisualizerPlugin** | [`AppBootingVisualizerPlugin/`](AppBootingVisualizerPlugin/) | `netstandard2.0` WPA plugin (`.ptix`) | Windows Performance Analyzer plugin — renders the same launch regions and event pairing as a 3-track Gantt inside WPA. |

```
            +---------------------------+
            |        XamlTimeline        |   netstandard2.0, no external deps
            |  TelemetryEvent            |
            |  PhaseDefinition / EventMatcher
            |  TimelineItem / PayloadFilter
            |  RegionsLoader  (+ embedded XamlAppLaunch.regions.xml)
            |  TimelineBuilder (Track 1 detection)
            +-------------+-------------+-+
                          ^             ^
            ProjectReference            ProjectReference
                          |             |
        +-----------------+--+      +------------------------------+
        | XamlTelemetryViewer |      |  AppBootingVisualizerPlugin  |
        | WInui3 (WinUI app)  |      |     (WPA .ptix plugin)       |
        +---------------------+      +------------------------------+
```

## Why a shared library

Both front-ends previously duplicated the same models and pairing logic. The genuinely identical pieces now live once in **XamlTimeline**:

* `TelemetryEvent` — one normalised trace event (was the viewer's `TelemetryEvent` and the plugin's `BootEvent`).
* `TimelineTrack`, `RegionKind`, `PayloadFilter`, `EventMatcher`, `PhaseDefinition`, `TimelineItem`.
* `RegionsLoader` — parses the **WPA Regions of Interest** file (with tool metadata in the standard `<Metadata>` node for track/kind/colour), and exposes the **canonical** `XamlAppLaunch.regions.xml` embedded in the library via `LoadEmbeddedDefault()`. Both apps load the *same* region definitions.
* `TimelineBuilder.BuildTrackOne(...)` — the Track 1 marker/phase detection algorithm (XAML end-heuristic, `${ProcessName}` payload matching, etc.).

XamlTimeline targets **`netstandard2.0`** because that is the common denominator both consumers can reference (the net8 WinUI app and the netstandard2.0 WPA plugin), and it carries **zero external package dependencies** so it adds nothing to the plugin's `.ptix`.

> **Note on `Directory.Build.props` / `.targets`:** XamlTimeline ships its own (near-empty) `Directory.Build.props`/`.targets` that intentionally do **not** chain to the repo root. This isolates it from the repo's build-flavor-specific `obj` redirection so it always restores to a single, stable `obj\project.assets.json` that Visual Studio can find. Do not delete them.

## Building

All three projects live in one solution, [`Tooling.sln`](Tooling.sln). Open it in **Visual Studio** and the WinUI viewer is the default startup project (Deploy enabled); pick a platform (**x64**) and press **F5** to build, deploy, and launch it. The command-line builds below target individual projects.

### The WinUI viewer

```powershell
cd perf\tooling
dotnet build .\Tooling.sln -p:Platform=x64
```

Or in **Visual Studio**: open [`Tooling.sln`](Tooling.sln), set the platform to **x64**, press **F5**. The solution contains the app, its `XamlTimeline` dependency, and the WPA plugin; the WinUI app is the startup project and Deploy is enabled, so it builds, deploys the MSIX package, and launches.

### The WPA plugin

```powershell
cd perf\tooling\AppBootingVisualizerPlugin\AppBootingVisualizerPlugin
.\Pack.ps1                          # -> AppBootingVisualizerPlugin-<version>.ptix
```

`dotnet publish` (which `Pack.ps1` runs) automatically includes `XamlTimeline.dll` in the package. See [`AppBootingVisualizerPlugin/README.md`](AppBootingVisualizerPlugin/README.md) for `plugintool` prerequisites and install steps.

### The shared library on its own

```powershell
cd perf\tooling\XamlTimeline
dotnet build .\XamlTimeline.csproj
```

## Editing region definitions

The launch regions are data-driven by **one** file: [`XamlTimeline/Regions/XamlAppLaunch.regions.xml`](XamlTimeline/Regions/XamlAppLaunch.regions.xml) (embedded in the shared library). Editing it changes both the viewer and the plugin. Rebuild after any change.

See each app's README for the regions XML schema and a worked example.
