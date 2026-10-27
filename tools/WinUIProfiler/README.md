# WinUI Profiler executable baseline

A standalone, unpackaged, Windows App SDK and .NET self-contained WinUI 3
application. Launch a packaged application by AUMID, launch an unpackaged
executable, or attach to a running process. Browse buttons supplement manual
entry. Connecting requires explicit trust of the local injection binaries.

The architecture-matched native injector loads a public `IXamlDiagnostics` TAP
into the selected target. A local, same-user named pipe verifies the process
creation identity, session, nonce and correlated `hello` reply. An injector
acceptance result alone never establishes a connection. Periodic hello requests
keep the connection alive.

Only after the genuine TAP hello, an independent polling worker starts these
two live graphs:

| Graph | Series | Source and meaning |
| --- | --- | --- |
| CPU and GPU | CPU %, GPU % | NT process/system CPU snapshots with processor-group normalization; English PDH GPU Engine counters, using the busiest process engine, not their sum. |
| Process memory | Working set, private working set, private commit (MiB) | `NtQuerySystemInformation(SystemProcessInformation)` process snapshots. Commit is not resident memory. |

Metrics are acquired every 250 ms on a fixed cadence; slow acquisitions skip
missed slots without overlapping captures. Metric polling and UI updates are
independent. Connection-state, heartbeat and
new-sample notifications schedule coalesced updates on the UI dispatcher; there
is no periodic UI refresh timer. State-only notifications do not redraw unchanged
metric histories. Each series retains at most 481 points and two minutes.
Charts start at the left and fill toward the right, then scroll over the latest
two minutes. Time labels show seconds since the first acquisition. The combined
CPU/GPU axis is `min(100, 10 * highest available utilization in the visible data)`;
an empty or all-zero window uses a 1% scale. Memory retains its independent MiB
scale. Disconnect freezes the final history and does not terminate the
target. Reconnecting starts a new metric scope. Closing the profiler cancels
connection work, joins polling and disconnects the TAP without killing the target.
Unavailable and warm-up measurements remain labelled gaps, never invented zeros.

The chart visuals, legends and searchable target-selection dialog are declared
in XAML. Executable browsing uses
`Microsoft.Windows.Storage.Pickers.FileOpenPicker`, including when elevated;
it does not use the older, brokered `Windows.Storage.Pickers` API. Cancelling a
picker preserves the existing target input.

`WinUIProfiler.Core\Ingestion\NativeMetricAbi.cs` mirrors the fixed-width C ABI
in `WinUIProfiler.Native\Exports.h`: explicit pack-8 structs, named statuses,
sources, clocks and units, and inline CPU-group/GPU-engine arrays. Native calls
use typed pointers rather than byte buffers and numeric decoding offsets.
Unsafe code is confined to synchronous native calls with bounded caller-owned
storage. Size/version, process identity, counts, reserved fields, intervals,
units, missing-value sentinels and busiest-engine validation remain enforced.
Native compile-time size assertions protect the x64 and ARM64 layouts.

This is the executable/connection/metrics baseline, not the full profiler.
There is no ETW listener, recording database, frame processing, tree snapshot,
heap capture or instrumented-product dependency in the app yet. Reading XAML
events from a recorded trace is described below. The profiler UI uses its pinned
SDK runtime; it does not replace the target's loaded runtime.

## Build and run

Requires Windows 11, .NET SDK 10.0.403, Windows SDK 10.0.26100.0 and Visual Studio
with MSBuild plus the x64 and ARM64 C++ toolchains. This tool has a standalone
build boundary and does not require a WinUI product build or product initialization.

Run from this directory:

```powershell
.\scripts\Build-App.ps1 -Configuration Release -Platform x64
$output = (Get-Content .\WinUIProfiler.App\obj\ProfilerAppOutput.x64.Release.txt -Raw).Trim()
& (Join-Path $output 'WinUIProfiler.App.exe')
```

Use `-Platform ARM64` to cross-build an ARM64 application; `-Architecture` is an
alias for `-Platform`. Both target-architecture TAP/injector pairs are built,
hash-verified and staged in `ConnectionPayloads`. The host's native metric DLL
is staged beside the executable. Distribute the complete output directory,
not the executable alone.

Select a mode and target, optionally arguments/working directory and diagnostics
endpoint, check the trust box, then connect. Connection status shows the actual
target PID, creation time, runtime path and latest hello. Errors and timeouts
remain visible; connecting to a non-WinUI process cannot produce graph samples.
The selected target must expose WinUI diagnostics, use a compatible runtime and
have matching architecture and access rights. Access denial is reported, not
silently elevated.

## Reading XAML events from a trace

`WinUIProfiler.Core` reads the XAML TraceLogging events of one process from a
recorded `.etl` file (`Ingestion\OfflineEtlSource.cs`). Each event becomes a
`ProfilerEvent` (`Events\ProfilerEvent.cs`). TraceLogging events carry their own
field names and types, so no manifest is needed. Manifest events on the same
providers are skipped.

| Provider | GUID | TraceLogging events |
| --- | --- | --- |
| Microsoft-Windows-XAML | `531a35ab-63ce-4bcf-aa98-f88c7a89e455` | Frames (`CoreServices_Frame`), objects entering the tree (`DependencyObject_Enter`), and layout, focus and other operations with element IDs |
| Microsoft-Windows-XAML-Profiler | `a1b2c3d4-e5f6-4a5b-9c8d-7e6f5a4b3c2d` | Tree, composition and graphics events |

Events with element IDs and all Microsoft-Windows-XAML-Profiler events are only
in builds with `XamlProfilerEnabled`.

To record a trace, use `logman` from an elevated prompt, or from a normal prompt
if your account is in the local Performance Log Users group. Then inspect it with
`WinUIProfiler.Cli`. Run from this directory:

```powershell
'{531A35AB-63CE-4BCF-AA98-F88C7A89E455} 0xFFFFF 5',
'{A1B2C3D4-E5F6-4A5B-9C8D-7E6F5A4B3C2D} 0xFFFFFFFFFFFFFFFF 5' | Set-Content providers.txt
logman create trace XamlTrace -o xaml.etl -pf providers.txt -ets
# ... use the target app ...
logman stop XamlTrace -ets

dotnet build .\WinUIProfiler.Cli\WinUIProfiler.Cli.csproj -c Release
$cli = '.\WinUIProfiler.Cli\bin\Release\net10.0\WinUIProfiler.Cli.exe'
& $cli processes xaml.etl                      # event count per process, to find the PID
& $cli summary xaml.etl --pid <pid>            # event counts for that process
& $cli dump xaml.etl --pid <pid> --name CoreServices_Frame --max 20   # the events themselves
```

## Limitations

CPU uses processor-time utilization, not Task Manager's frequency-adjusted
utility. Aligned hardware parity with Task Manager and native ARM64 execution
are separate qualification gates, not implied by a successful build or matching
collection APIs. NT process layouts are validated at the compatibility boundary;
unsupported layouts and missing GPU counters produce explicit diagnostics.
The TAP's bounded replay guard rejects duplicates without eviction; its Bloom
filter can conservatively reject a fresh request after very long sessions.
Such a rejection stops the connection visibly rather than silently reconnecting.
