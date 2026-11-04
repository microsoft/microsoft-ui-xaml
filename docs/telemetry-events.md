# TraceLogging telemetry events

## Table of Contents

- [Syntax](#syntax)
- [Viewing/monitoring telemetry events](#viewingmonitoring-telemetry-events)

## Syntax

A typical event in source will look like this:
```
TraceLoggingWrite(
    g_hTraceProvider,
    "NameOfEvent",
    TraceLoggingDescription("Description of Event"),
    TraceLoggingUInt32(value, "FieldName"[, Optional description]),
    TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
    TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
```

The parameters are:

`g_hTraceProvider`: This is a reference to the provider.  Defined in the module by `TRACELOGGING_DEFINE_PROVIDER` macro.

`"NameOfEvent"`: simply the name of the event as a string literal.  Keep it simple, use alpha-numeric characters.

`TraceLoggingDescription()`: Macro for event description.

`TraceLoggingUInt32()`: (and other types).  Includes the value, string literal for field name, and optional 
description.  You may have multiple fields per event.

`TelemetryPrivacyDataTag()`: The data tag for the event.  This describes how the data will be interpreted.  For this 
project, as a platform and NuGet package, PDT_ProductAndServicePerformance is used.

`TraceLoggingKeyword()`: Ordered by increasing population Telemetry (Insider population), Measures (small percentage 
of retail population), Critical (all retail machines that opt in to telemetry)

## Viewing/monitoring telemetry events

You may use 'Diagnostic Data Viewer' available in the Windows Store to view the telemetry events from this and other 
components.

## WinUI app startup

`Microsoft.UI.Xaml` (`{2dc72f6e-e4d1-5f58-3245-09a4243799dd}`) emits
`WinUIAppStartup` after the first successful XAML runtime initialization in a process.
Additional windows, UI threads, and XAML shutdown/reinitialization do not emit it again.
Loading the DLL without initializing XAML, failed initialization, and XBF-only initialization
do not emit this event.

The payload contains only `BinaryVersion` and the standard `PartA_PrivTags`
(`PDT_ProductAndServicePerformance`). The event uses informational level and
`MICROSOFT_KEYWORD_MEASURES`. Like other ETW events, it is only delivered to listeners
enabled at the time of emission; enabling a listener later does not replay startup.
This event does not change diagnostic-data consent, sampling, or collection policy.
Privacy approval and production collection configuration must be completed separately.

### Relationship to existing telemetry

Existing startup-related diagnostics are not equivalent to this process-level usage event:

- The manifest `ApplicationStartup`, `ApplicationStarted`, and `InitializeCore` events use
  Appmodel/Core performance keywords rather than Measures. They trace root-visual/application
  operations and individual core initialization, not a process-wide successful-startup count.
- `XamlTelemetry::CreateDesktopWindow` emits verbose start/stop pairs for each desktop window;
  it does not cover windowless XAML initialization or provide process-wide deduplication.
- `DXamlInitializeCoreTest` reports initialization health, including failures and reason flags,
  through TestInProduction. Builds using the repository's TiP stub emit no TiP results; this
  is not a replacement for a dedicated Measures event from `Microsoft.UI.Xaml`.
- `DesktopWindowXamlSource-LoadedFrameworks` and `DesktopWindowXamlSource-NewMaxActive`
  describe hosting/framework inventory and instance high-water marks. `TableView_Loaded`
  describes the first load of that specific control, not general XAML startup.
- `RuntimeProfiler` reports accumulated constructor/member counts on timer/suspend/teardown
  boundaries, rather than immediately after successful initialization.

These events remain unchanged. The new event supplies the explicit successful-initialization,
once-per-process contract; consumers should not sum it with the related diagnostic events
as though they were additional app launches.

### Local emission regression test (x64)

Build the runtime, controls, Tabular controls, and `MUXControlsTestApp`. Prepare a local,
self-contained unpackaged test app layout with the native DLLs and resources from that
same build; the test app project can otherwise retain native binaries from its SDK package.
The test rejects a layout whose XAML/controls DLL hashes differ from the expected build.
Run from a terminal with ETW session permissions:

```powershell
.\initrun.ps1 pwsh -NoProfile -File .\tools\TelemetryTests\Test-WinUIUsageTelemetry.ps1 `
    -RuntimeDirectory .\BuildOutput\bin\amd64chk\Test\UnpackagedApps\MUXControlsTestApp `
    -ExpectedXamlDllPath .\BuildOutput\bin\amd64chk\Product\Microsoft.UI.Xaml.dll
```

The test verifies loaded binary hashes, captures ETW before launch, and checks a DLL-load-only
negative control plus two independent app launches. Each app creates three windows, initializes
nested XAML managers on four additional UI threads across two rounds of dispatcher creation and
shutdown, and must emit exactly one startup event. It also checks startup payload/keyword/privacy
metadata, zero trace loss, and nonzero `RuntimeProfiler` events from both `Microsoft.UI.Xaml` and
`Microsoft.UI.Xaml.Controls`, including NavigationView and TabView constructor counts.

A separate workload activates all 31 source-instrumented core classes and the 57 active
controls/Tabular constructor IDs in `tools\TelemetryTests\ProfilerControls.csv`.
There are 90 scenarios for 88 distinct counters because Reveal brush variants and
SplitButton variants intentionally share profiler IDs. Each scenario runs twice.
The grouped/hierarchical adapter counters are exercised through `TableViewSource.GroupBy`
and `ParentBy`, not by calling the instrumentation directly.

The test checks every expected `[typeId|9999]:count`, combines duplicate ID entries, validates
the usage-event metadata and overflow flags, and verifies that a second flush without new
construction emits no duplicate controls/Tabular counters. `control-results.csv` records
the expected and observed count for every active constructor ID.
`MicaController`, `LinedFlowLayout`, and `MapControl` retain enum IDs but have no constructor
marker in the current source, so no event is expected for those reserved IDs.
Conditional member counters (for example TeachingTip F6 handling and composition-facade
animation properties) are separate from constructor coverage and are not exercised here.

The controls profiler's existing suspend export is called to flush counts deterministically;
this test does not wait for or verify the normal 20-minute timer. Raw ETL, decoded XML, process
IDs, and host output (including loaded module hashes) are retained in the reported evidence
directory. This verifies local emission, not ingestion into production telemetry services.