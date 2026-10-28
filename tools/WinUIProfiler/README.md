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
While connected, the app saves the target's XAML ETW events, and the kernel's
GPU memory events for it, to a session file.
There is no object or frame view, heap capture or instrumented-product
dependency in the app yet. The profiler UI uses its pinned SDK runtime; it does
not replace the target's loaded runtime.

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

## Tree snapshots

As soon as a connection succeeds after the genuine TAP hello, the app
automatically captures and saves an **Initial snapshot** as the first snapshot
of that connection. Manual capture is disabled until this initial attempt
finishes. A failed initial capture is reported without failing the connection;
**Take tree snapshot** remains available to retry or capture later states.
Each reconnect takes its own initial snapshot, retaining earlier history.

The TAP captures exactly two
trees: the realized XAML visual tree and its publicly exposed Composition visual
tree, with object-to-visual correlations and a bounded set of SDK properties.
The validated JSON is saved atomically under `%LOCALAPPDATA%\WinUIProfiler\Snapshots`;
the UI shows the node counts and saved path. Repeated captures are independent.
Each successful capture appears in **Snapshot history**, newest first. Select an
entry to open its viewer: expandable XAML and Composition trees, selected-node
properties, and direct object-to-visual mapping/highlighting. The same snapshot
reuses its open viewer; different snapshots can be compared in separate windows.
Tree labels include the XAML object's COM identity address or the Composition
visual's IVisual interface address. Each tree has Expand all and Collapse all
buttons; expanding includes descendants that have not yet been realized.
Composition node details hide known default-valued properties. Non-default
values, unknown properties and property errors remain visible; XAML details and
the complete saved snapshot JSON are unchanged.
History remains available across disconnect/reconnect for the current profiler
session, and the saved JSON files are retained after exit. The history list is
not reloaded on startup. Missing or invalid snapshot files produce visible errors.
No recording database, heap capture or MUX changes are required.

Root discovery uses public `IVisualTreeService` callbacks on the target's UI
threads; traversal uses `VisualTreeHelper`, `ElementCompositionPreview` and
Composition APIs. Captures are bounded to 20 seconds, 64 MiB and 131,072 records.
Roots have individual capture intervals, not a globally atomic timestamp.
Unrealized/logical objects, internal HWCompNodes and private compositor trees
are not exposed. `GetElementVisual` may materialize handout visuals. Property
errors and scope limitations are included in the JSON rather than fabricated values.

## XAML events

`WinUIProfiler.Core` reads the XAML TraceLogging events of one process, and the
kernel's GPU memory events for it, live or from a recorded `.etl` file. Each
event becomes a `ProfilerEvent` (`Events\ProfilerEvent.cs`). TraceLogging events
carry their own field names and types, so no manifest is needed. Manifest events
on the XAML providers are skipped. The GPU memory events are manifest events,
decoded with the manifest installed with Windows.

| Provider | GUID | Events read |
| --- | --- | --- |
| Microsoft-Windows-XAML | `531a35ab-63ce-4bcf-aa98-f88c7a89e455` | Frames (`CoreServices_Frame`), objects entering the tree (`DependencyObject_Enter`), and layout, focus and other operations with element IDs |
| Microsoft-Windows-XAML-Profiler | `a1b2c3d4-e5f6-4a5b-9c8d-7e6f5a4b3c2d` | Tree, composition and graphics events |
| Microsoft-Windows-DxgKrnl | `802ec45a-1e99-4b83-9920-87c98277ba9d` | GPU memory, keyword `0x4`: `VidMmProcessUsageChange` (367) and `VidMmProcessBudgetChange` (366), the process's usage and budget per adapter and memory segment group (`MemorySegmentGroup` 0 is local, 1 is non-local) |

Events with element IDs and all Microsoft-Windows-XAML-Profiler events are only
in builds with `XamlProfilerEnabled`. The GPU memory events come from Windows,
whatever the build. They carry the values `IDXGIAdapter3::QueryVideoMemoryInfo`
returns, which earlier builds also reported in the profiler's `GpuMemorySnapshot`
event. The kernel writes them when the usage or budget changes, and
writes the current values when the process opens a GPU adapter and when a
session requests capture state. The System process writes some of them, such as
those written when the process exits, which bring the usage down to 0. So they
are matched to the process by their `ProcessId` field, which becomes the
event's process ID.

Microsoft-Windows-XAML is enabled with keyword `0x100000`. No manifest event
uses that keyword, so ETW drops them at the source and only TraceLogging events
are delivered.

Reading ETW needs an elevated process, or membership in the local Performance
Log Users group.

### Live, from the app

The app reads ETW with one real-time session per connection
(`Ingestion\LiveEtwSource.cs`):

- **Launch:** the session starts before the process is created, so the target's
  first events are included. The process has no ID yet, so ETW filters on its
  executable name: the file name of the selected executable, or for a packaged
  app the `Executable` of the matching `<Application>` in the installed
  package's `AppxManifest.xml`. As soon as the launch returns the process ID,
  the same session switches to a process ID filter. Events stay in the
  session's buffers until then, and events from other processes with the same
  executable name are skipped. The target is launched normally, not suspended,
  so its launch timing is unchanged.
- **Attach:** the session filters on the process ID from the start and reads
  from the time of connection.

The GPU memory events are enabled for every process: a process filter does not
narrow a kernel provider's events, and the System process writes some of the
target's. Keyword `0x4` also covers the kernel's other video memory events of
every process, such as residency changes, which are far more frequent, so the
session enables only events 366 and 367 (an event ID filter); the events of
other processes are skipped as they are read. When reading starts, the session
requests capture state, so an attached target also gets its current values.
Capture state enables the provider again without the filter, so the session
applies the filter again right after. If the GPU memory events cannot be
enabled, the session still reads the XAML events, and the tooltip of the
panel's ETW line says why.

The panel says whether the counts start at process start or at connection. A
launch starts at connection when activation returns a packaged app that was
already running, when the app declares no executable, or when the launched
process runs a different executable than its manifest declares.

Events go through a queue of 65,536 events to one processing thread that runs
the event processors (`Processing\EventPipeline.cs`) in arrival order, numbering
the events 1, 2, 3... Today the processors count events
(`Processing\EventCounter.cs`) and save them (`Storage\SessionWriter.cs`).

The connection panel shows the count per provider, events lost by ETW, and
events dropped because the queue was full. When the queue is full, new events
are dropped and counted rather than stopping the session. The session stops
when the connection ends; the last counts stay visible. If the session cannot
start, the panel shows why and the metrics keep working.

An ETW session outlives a profiler that crashes or is killed, and keeps the
providers enabled in the processes it filters on, and the kernel's GPU memory
events for every process. Sessions are named
`WinUIProfiler-<profiler process ID>-<unique ID>`; starting a session stops any
whose profiler process no longer exists. To stop one by hand, find it with
`logman query -ets` and run `logman stop <name> -ets`.

### Session files

While connected, the app saves every event it receives to a SQLite file, one
per connection, in `%LOCALAPPDATA%\WinUIProfiler\Sessions`, named
`<date>-<time>-<process>-<pid>.sqlite`. A writer thread commits a batch every
2,000 events or 250 ms; if it falls behind, the processing thread waits for it,
so events are still only dropped at the queue.

| Table | Contents |
| --- | --- |
| `SessionInfo` | One row: the process, source (live or trace), coverage, time zero and QPC frequency, size limit, how and when the session ended, events lost by ETW and dropped by the profiler |
| `EventTypes` | One row per provider, event name, opcode and field layout: the field names and their .NET types, such as `Int64,Int32,String,Boolean` |
| `Events` | One row per event: `Seq` (arrival order), `Qpc`, thread, `TypeId` (its row in `EventTypes`), and the field values as a JSON array in field order |

JSON has no Infinity or NaN, so those values are stored as the strings
`"Infinity"`, `"-Infinity"` and `"NaN"`; the field type restores the number.

A session file is limited to 500 MB. Before a batch would take it past the
limit, the oldest events are deleted, `SessionInfo.CompleteFromSeq` records the
first event still stored, and the connection panel says from when the file is
complete. While recording, the file is in WAL mode, so it can be read at the
same time, for example with the CLI; when the session ends, it is left as a
single `.sqlite` file. A write error stops the recording, keeps what was saved,
and the panel shows why. Session files are not deleted by the profiler.

ETW timestamps an event separately in each session that receives it, so the
same event recorded by `logman` and by the app differs by a few microseconds.

`Seq` numbers the events 1, 2, 3... in arrival order, without gaps (after
trimming it starts later). In the sessions measured, arrival order matched
timestamp order, but ETW does not guarantee that across processors, and two
events can have the same timestamp: to order events by time, sort by `Qpc`,
then `Seq`.

### Recorded, from a trace file

To record a trace, use `logman`. Start it before the app: the kernel writes the
app's starting GPU memory values when the app opens a GPU adapter, and after
that only changes. `logman` cannot filter by event ID, so the trace also holds
the kernel's other keyword `0x4` events, of every process; the CLI skips them.
Then inspect the trace with `WinUIProfiler.Cli`, which reads the file through
`Ingestion\OfflineEtlSource.cs`. The CLI reads session files the same way, and
`record` writes a session file from a trace. Run from this directory:

```powershell
'{531A35AB-63CE-4BCF-AA98-F88C7A89E455} 0x100000 5',
'{A1B2C3D4-E5F6-4A5B-9C8D-7E6F5A4B3C2D} 0xFFFFFFFFFFFFFFFF 5',
'{802EC45A-1E99-4B83-9920-87C98277BA9D} 0x4 5' | Set-Content providers.txt
logman create trace XamlTrace -o xaml.etl -pf providers.txt -ets
# ... use the target app ...
logman stop XamlTrace -ets

dotnet build .\WinUIProfiler.Cli\WinUIProfiler.Cli.csproj -c Release
$cli = '.\WinUIProfiler.Cli\bin\Release\net10.0\WinUIProfiler.Cli.exe'
& $cli processes xaml.etl                      # event counts of the processes with XAML events, to find the PID
& $cli summary xaml.etl --pid <pid>            # event counts for that process
& $cli dump xaml.etl --pid <pid> --name CoreServices_Frame --max 20   # the events themselves
& $cli record xaml.etl --pid <pid> --out session.sqlite [--limit-mb 500]  # a session file from the trace
& $cli summary session.sqlite                  # a session file holds one process; --pid is optional
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
