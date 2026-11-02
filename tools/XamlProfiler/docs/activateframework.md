# Framework-Side Profiler Activation

The XamlProfiler producer-side instrumentation is compiled into
`Microsoft.UI.Xaml.dll` in all configurations, including Debug/chk and
Release/fre. No build-time switch or source edit is required.

> For *what* the instrumentation does, see `frameworksidechanges.md`.
> For *how the consumer* (profiler app + tap) uses it, see `profilerworking.md`.

## Runtime activation

Event emission is controlled by ETW provider enablement:

| Provider | GUID | Purpose |
|---|---|---|
| Microsoft-Windows-XAML-Profiler | `A1B2C3D4-E5F6-4A5B-9C8D-7E6F5A4B3C2D` | Tree, composition, surface, and memory events |
| Microsoft-Windows-XAML | `531A35AB-63CE-4BCF-AA98-F88C7A89E455` | Normal runtime events, including identity-bearing operations and composition removals |

Start a capture with the profiler app or `XamlProfiler.wprp` to enable the
providers. The supplied profile captures both at Verbose level. Stop the
capture to disable event emission when no other session has enabled them.

Existing runtime checks remain in place so expensive event-payload extraction
is skipped when the corresponding provider is not enabled. Instrumentation is
always present in the binary; stopping a capture does not remove that code.

## Build and verification

Build the runtime normally:

```powershell
.\initrun.ps1 .\build.cmd /q mux
```

Attach the profiler to an application using the built runtime and verify that
tree and composition events arrive while its capture session is active.
The same instrumentation is available in both Debug and Release builds.

Independent diagnostic conditions still apply. Allocation counters retain
their existing Debug/allocation-tracing conditions. Source locations require
source-info collection and available XAML line information.
