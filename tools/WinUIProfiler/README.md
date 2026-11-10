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
in XAML. Executable and trace file browsing use
`Microsoft.Windows.Storage.Pickers.FileOpenPicker`, including when elevated;
they do not use the older, brokered `Windows.Storage.Pickers` API. Cancelling a
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
GPU memory and window focus events for it, to a session file. It can also read
them from a recorded trace (`.etl`) into a session file. The Launch tab shows
the launch's milestones, including an app's own (see
[Launch timeline](#launch-timeline)), the Frames tab its UI-thread ticks and
the regions of interest in them, and the Objects tab its UI elements and
composition visuals.
There is no heap capture or instrumented-product
dependency in the app yet. The profiler UI uses its pinned SDK runtime. It
replaces the target's WinUI runtime only when it launches the target with
private WinUI DLLs (see "Profile an app with private WinUI DLLs").

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
hash-verified and staged in `ConnectionPayloads`, and both private WinUI
launchers in `NativeLauncher`. The first build downloads Microsoft Detours for
the launcher with git (see `WinUIProfiler.Launcher\README.md`). The host's native metric DLL
is staged beside the executable. Distribute the complete output directory,
not the executable alone.

Run the event compatibility and snapshot resource regression checks from this directory:

```powershell
dotnet run --project .\WinUIProfiler.Tests\WinUIProfiler.Tests.csproj
```

These check object timelines across new TraceLogging and legacy manifest events,
and verify that migrated performance regions distinguish Start/Stop without
also matching legacy manifest events. Snapshot checks cover allocator deltas,
named graphics resources, untracked versus observed lifetimes, four-group
classification, missing captures and full-range 64-bit graphics comparisons.

Select a mode and target, optionally arguments/working directory and diagnostics
endpoint, check the trust box, then connect. Connection status shows the actual
target PID, creation time, runtime path and latest hello. Errors and timeouts
remain visible; connecting to a non-WinUI process cannot produce graph samples.
The selected target must expose WinUI diagnostics, use a compatible runtime and
have matching architecture and access rights. Access denial is reported, not
silently elevated. A target whose UI thread is too busy to take the TAP request
within about a second, as in a slow startup, makes the injector return
`0x80070490`; Connect then retries until its 45-second deadline. Select
**Recorded trace (.etl)** to read a recorded trace instead (see "Recorded, from
a trace file" below).

## Profile an app with private WinUI DLLs

`scripts\Profile-App.ps1` profiles a WinUI 3 app that loads private builds of
`Microsoft.UI.Xaml.dll` and `Microsoft.UI.Xaml.Controls.dll`, such as your own
build of this repo, instead of its own. Run it from this directory with
PowerShell 7.4 or later:

```powershell
.\scripts\Profile-App.ps1 -MuxPath D:\out\Microsoft.UI.Xaml.dll -MuxcPath D:\out\Microsoft.UI.Xaml.Controls.dll -ExePath C:\Apps\MyApp\MyApp.exe
.\scripts\Profile-App.ps1 -MuxPath D:\out\Microsoft.UI.Xaml.dll -MuxcPath D:\out\Microsoft.UI.Xaml.Controls.dll -Aumid 'Microsoft.WinUI3ControlsGallery_8wekyb3d8bbwe!App'
```

Use `-ExePath`, with optional `-Arguments`, for an unpackaged app, and `-Aumid`
for a full-trust packaged app (`Get-StartApps` lists AUMIDs). The script:

1. Builds the profiler with `Build-App.ps1` (Release, for this machine's
   architecture) when this checkout has no build. `-Build` rebuilds it;
   `-ProfilerPath` uses another `WinUIProfiler.App.exe`.
2. Asks for administrator rights with a UAC prompt, because reading ETW live
   and creating the links below need them. The app runs without them.
3. Checks that the DLLs are MUX and MUXC builds for the app's architecture,
   x64 or ARM64, that the app is not running and that it has no `.local`
   redirection.
4. Finds the WinUI runtime the app loads now: in its folder, its package or its
   package's framework dependencies. An unpackaged app that uses the Windows
   App SDK framework package needs `-MuxResourceDirectory` with that folder.
5. Stages a runtime folder,
   `%LOCALAPPDATA%\WinUIProfiler\PrivateRuntime\<run>\runtime`: copies of the
   two DLLs and symbolic links to the original runtime's other files and
   language folders. When the private MUX expects different MUI resource files
   than the original runtime has, the copy is associated with the original's
   with the Windows SDK's `muirct.exe`, and a signed DLL is refused.
6. Opens WinUI Profiler, which launches the app with `ProfilerLauncher.exe`
   (see `WinUIProfiler.Launcher\README.md`) so that it loads the private DLLs,
   and connects from process start. The connection fails if the app loaded
   another `Microsoft.UI.Xaml.dll`. Each Connect launches the app again with
   the same DLLs.
7. Waits for you to close the profiler. Then it closes the app if it is
   running, deletes the staged runtime (the links, not the files they point
   to), and prints each launch's process ID, the private DLLs it loaded and the
   session files saved. `<run>\launch\<n>` keeps each launch's `launch.json`
   and `redirect.jsonl`.

It exits with 0 when the app ran with the private DLLs and the staged runtime
was deleted, 4 when the UAC prompt was declined and 1 otherwise. A runtime
that cannot be deleted, for example because the app is still running, is kept
and reported.

The private DLLs must match the app's Windows App SDK version: the launcher
replaces the two DLLs, not the rest of the runtime. UWP (AppContainer) apps are
not supported.

The script starts the profiler with these options, which can also be used
directly:

| Option | Meaning |
| --- | --- |
| `--launch-exe <path>` or `--launch-aumid <AUMID>` | The app to launch: a full path, or a packaged app. |
| `--arguments <text>` | The unpackaged app's command line. |
| `--private-runtime <folder>` | The staged runtime the app loads MUX from. |
| `--runtime-source <folder>` | The original runtime folder, whose MUX requests are redirected. |
| `--redirect-controls` | Also load MUXC from the staged runtime. |
| `--launch-directory <folder>` | Where each launch writes its results, in `<n>` subfolders. |
| `--trust-local-binaries` | Checks the trust box. |
| `--connect` | Connects once the window opens. |

`--private-runtime`, `--runtime-source` and `--launch-directory` go together,
as full paths. Each launch runs the launcher from `<n>\native` in the launch
directory; see Limits in `WinUIProfiler.Launcher\README.md` for the characters
that path can have.

## Tree snapshots

Each capture also calls the target's private `IXamlDiagnosticsPrivate::GetResourceSnapShot`
once, before tree enumeration, and saves the result and its QPC interval in the
snapshot JSON (schema 4, `resourceSnapshot`). The **Resource allocations** view shows
four collapsible groups: **Allocation statistics**, **Composition objects**,
**D2D objects**, and **WinUI wrappers**. Each allocator metric or populated graphics
type occupies one row. Composition and raw D2D objects show only creation totals,
with a group-level note that destruction and live counts are not tracked. Native
WinUI wrappers show observed creation, destruction and live counts, not the lifetime
of underlying graphics objects or GPU allocations. Unknown slots appear separately
with their numeric labels and raw counters, without assigning an invented category.
The same groups in comparisons show **Before**, **After**, and
signed **Delta (After - Before)** values, in counts or bytes; resource-only changes
are included even when the trees are unchanged. Deltas preserve the full 64-bit
counter range, including decreases. All five allocator/GC metrics use signed 64-bit
values: `-1` means unavailable, while zero is a measured value. Unavailable metrics
and resources absent from either capture have no delta, not an assumed zero.

The private interface requires a target runtime containing resource diagnostics.
Its IID changed with the signed allocator-counter contract so older runtimes fail
the interface query safely instead of writing an incompatible snapshot layout.
Missing support or a failed call preserves the tree capture and reports unavailable
resources with the HRESULT. Schema-1 snapshots remain readable and report resources not
recorded. Existing schema-3 resource captures and allocation-only schema-2 memory
captures remain readable when their allocator values fit the signed 64-bit range;
schema-2 captures are normalized on read. Larger legacy unsigned allocator values
are rejected rather than wrapped. Graphics counters retain their unsigned 64-bit range;
old named graphics entries without fixed-slot identities are rejected, not assigned
invented identities. Allocator totals cover DBG accounting or enabled profiling,
not the process heap. The first resource snapshot ensures the provider is registered.
DBG builds track allocator statistics even without a profiler listener. Other builds
miss earlier allocations, and allocation/free pairs across profiling transitions can
be unbalanced. Disabled or unrepresentable allocator metrics are returned as `-1`;
retained internal totals are not presented as measured zeros when tracking stops.
`AllocatedMemory` is the independent managed-runtime GC heuristic and returns `-1`
when managed-memory tracking is disabled. Resource and tree intervals are not atomic.
Graphics counters require a target runtime containing rendering resource diagnostics
and an enabled profiler provider. Totals accumulate across enabled profiling periods
and exclude earlier creations; they are not scoped to the captured tree nodes.
All-zero slots are omitted: an empty graphics section can mean unsupported or disabled
tracking, or no observed creations, not that no graphics resources exist.

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
XAML and Composition node details hide known default-valued properties, including
the XAML identity render-transform matrix. Non-default values, unknown properties,
computed XAML geometry and property errors remain visible. Theme-dependent brush
colors are not assumed to be defaults. The complete saved snapshot JSON and
comparisons retain all captured values, including changes back to defaults.
History remains available across disconnect/reconnect for the current profiler
session, and the saved JSON files are retained after exit. The history list is
not reloaded on startup. Missing or invalid snapshot files produce visible errors.
No recording database, heap capture or MUX changes are required.

Double-clicking a node, in a snapshot's viewer or in a comparison, shows its
history in the main window's Objects tab: a XAML node's element, or a
Composition node's visual (see below).

XAML properties include margin, horizontal/vertical alignment, layout slot,
transformed bounds relative to the visual parent, render transform and origin,
and backgrounds on controls, panels and borders. Parentless bounds use the visual
root; bounds do not account for clipping or prove on-screen visibility.
TextBlocks also include text and foreground. Brushes include type/opacity and,
for solid brushes, `#AARRGGBB` color; other brush content is not captured.
Standard render transforms include their numeric properties; transform groups
include their combined `[m11, m12, m21, m22, offsetX, offsetY]` matrix.
Text over 4,096 UTF-16 code units is recorded as an explicit property error, not
silently truncated. This is a fixed SDK-property list, not arbitrary app getters,
all dependency properties, bindings or resources. New properties appear in both
the viewer and comparison; existing saved snapshots are not modified.

Select **Compare snapshots...** in history to choose **Before** and **After**
captures from the same process instance (PID and creation time). The comparison
highlights additions, removals, reparenting, sibling reordering, changed properties,
and affected ancestor subtrees in both trees. Select a node for before/after values
and parent locations; turn off **Show only changed subtrees** for unchanged context.
Selecting either tree also reveals its directly mapped counterparts in the other
tree. The match picker identifies mappings from Before, After, or both captures.
Unchanged matches and their ancestors remain visible while the changes-only filter
is enabled; link icons distinguish these correlations from actual changes.
The merged view uses the After hierarchy, retaining removed nodes at their old
parents. Matching is best-effort by UI thread, XamlRoot address, canonical COM
identity address and type, never capture-local node IDs. Address reuse cannot be
distinguished from a surviving object; cross-root moves appear as removals/additions.
All captured properties are compared, including default values, with semantic JSON
equality. Insertions/removals alone do not mark surviving siblings as reordered.
Visibility-property changes are shown, not inferred rendered visibility.

Root discovery uses public `IVisualTreeService` callbacks on the target's UI
threads; traversal uses `VisualTreeHelper`, `ElementCompositionPreview` and
Composition APIs. Captures are bounded to 20 seconds, 64 MiB and 131,072 records.
Roots have individual capture intervals, not a globally atomic timestamp.
Unrealized/logical objects, internal HWCompNodes and private compositor trees
are not exposed. `GetElementVisual` may materialize handout visuals. Property
errors and scope limitations are included in the JSON rather than fabricated values.

## XAML events

`WinUIProfiler.Core` reads all the events of the XAML providers for one process,
and the kernel's GPU memory and window focus events for it, live or from a
recorded `.etl` file. Each event becomes a `ProfilerEvent`
(`Events\ProfilerEvent.cs`).

| Provider | GUID | Events read |
| --- | --- | --- |
| Microsoft-Windows-XAML | `531a35ab-63ce-4bcf-aa98-f88c7a89e455` | Manifest events for frames, layout, parsing, resources, rendering, input and other runtime work. TraceLogging events for frames (`CoreServices_Frame`), objects entering the tree (`DependencyObject_Enter`), and layout, focus and other operations with element IDs |
| Microsoft-Windows-XAML-Diagnostics | `59e7a714-73a4-4147-b47e-0957048c75c4` | Elements created (with their XAML file), added to and removed from the tree, property changes, measure and arrange overrides, invalidation, bindings, resources, animations and input |
| Microsoft-Windows-XAML-Profiler | `a1b2c3d4-e5f6-4a5b-9c8d-7e6f5a4b3c2d` | Tree, composition and graphics events |
| Microsoft.UI.Xaml | `2dc72f6e-e4d1-5f58-3245-09a4243799dd` | The runtime's telemetry events, such as the first UI thread frame (`FirstUiThreadFrameEnd`), D3D device sharing and loss, XAML islands, GC durations and fail-fasts |
| Microsoft.UI.Xaml.Controls | `21e0ae07-56a7-55b5-12f9-011e6bc08cca` | The controls' telemetry events, such as NavigationView, ink, WebView2 and TableView usage and errors |
| Microsoft-Windows-DxgKrnl | `802ec45a-1e99-4b83-9920-87c98277ba9d` | GPU memory, keyword `0x4`: `VidMmProcessUsageChange` (367) and `VidMmProcessBudgetChange` (366), the process's usage and budget per adapter and memory segment group (`MemorySegmentGroup` 0 is local, 1 is non-local) |
| Microsoft-Windows-Win32k | `8c416c79-d49b-4f01-a467-e56d3aa8234c` | Window focus changes, keyword `0x2000` at level 4: event 2, the only event with that keyword, once per focus change. Its fields are the thread that lost focus and the thread that got it |

Layout, focus, virtualization, navigation, hit-testing and PasswordBox TraceLogging
events carry element identities in all builds. Tree-specific events and
Microsoft-Windows-XAML-Profiler instrumentation are also compiled in all builds;
event emission is controlled by runtime ETW enablement.
The GPU memory events come
from Windows, whatever the build. They carry the values
The providers of the marker configuration, such as an app's own, are read too,
for the same process only: see [Launch timeline](#launch-timeline).

Projection-layer activities, peer association and surface initialization use
runtime ETW enablement in all build configurations. Remaining element and
composition instrumentation still requires `XamlProfilerEnabled`. The GPU memory
events come from Windows, whatever the build. They carry the values
`IDXGIAdapter3::QueryVideoMemoryInfo` returns, which earlier builds also
reported in the profiler's `GpuMemorySnapshot` event. The kernel writes them
when the usage or budget changes, and writes the current values when the
process opens a GPU adapter and when a session requests capture state. The
System process writes some of them, such as those written when the process
exits, which bring the usage down to 0. So they are matched to the process by
their `ProcessId` field, which becomes the event's process ID.

The focus events also come from Windows, for every process with windows. They
are matched to the process by the event header, and only start the Frames
tab's regions of interest (see Frames tab below).

The XAML providers are enabled with every keyword at level 5 (Verbose). That
includes the telemetry events, which have keyword bits, and the TraceLogging
events the runtime writes only while the matching manifest event is enabled,
such as the start of each `FrameworkElement_ApplyTemplate`, which carries the
element's class and name. Not included are Microsoft-Windows-XAML's 16 events
at its own levels above Verbose, written for every API call and for every
object or property the parser sets, such as `ApiFunctionCallStart` and
`CreateInstanceBegin`.

TraceLogging events carry their own field names and types. Manifest events are
decoded with the manifest Windows registers for their provider. WinUI does not
register one: its Microsoft-Windows-XAML and Microsoft-Windows-XAML-Diagnostics
events share their IDs and fields with system XAML's (`Windows.UI.Xaml.dll`), so
they are decoded with system XAML's manifest. One event differs: WinUI's
`FrameInfo` (event 64, named `Frame`) has one field, `Value`, which is whether
the frame drew, so `Ingestion\XamlEventSubscription.cs` decodes it. An event
the registered manifest does not describe, such as one WinUI added that the
installed Windows does not have, is not delivered. The GPU memory events are
decoded with the kernel's manifest. `ProfilerEvent.TraceLogging` tells
TraceLogging events from manifest events: the same field name can hold
different objects in the two.

Profiler tree and composition events carry identities, relationships and state,
not debug labels, core allocation sizes or XAML source positions. Older traces'
label metadata remains readable.

While Microsoft-Windows-XAML-Diagnostics is enabled, the runtime keeps the XAML
source position of the objects it parses, which `ElementCreatedWithSource`
carries, and
keeps setters and resource references for XAML diagnostics. That costs the
target some memory and CPU time, as in any trace that enables the provider.
Only XAML parsed while the provider is enabled has positions: a launch has them
from the start, an attach only for XAML parsed after it. Compiled XAML without
line information, as in the Store's WinUI 3 Gallery, has `LineNumber` and
`ColumnNumber` 0.

Launching the WinUI 3 Gallery from the app and recording for about 11 seconds
wrote 116,000 to 138,000 events, 70 to 75 percent from Microsoft-Windows-XAML
and nearly all the rest from Microsoft-Windows-XAML-Diagnostics: a 6 to 7 MB
session file, with no events lost or dropped.

Reading ETW live, or recording a trace, needs an elevated process, or
membership in the local Performance Log Users group. Reading a trace file does
not.

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
  so its launch timing is unchanged. With private WinUI DLLs, the launcher
  creates it suspended to add its helper first, which adds to the launch time.
- **Attach:** the session filters on the process ID from the start and reads
  from the time of connection.

The GPU memory events are enabled without a process filter, because the System
process writes some of the target's; the events of other processes are skipped
as they are read. When reading starts, the session requests capture state, so
an attached target also gets its current values. If the GPU memory events
cannot be enabled, the session still reads the XAML events.

The panel says whether the counts start at process start or at connection. A
launch starts at connection when activation returns a packaged app that was
already running, when the app declares no executable, or when the launched
process runs a different executable than its manifest declares.

Events go through a queue of 131,072 events to one processing thread that runs
the event processors (`Processing\EventPipeline.cs`) in arrival order, numbering
the events 1, 2, 3... Today the processors count events
(`Processing\EventCounter.cs`) and save them (`Storage\SessionWriter.cs`).

The connection panel shows the event counts, events lost by ETW, and events
dropped because the queue was full. When the queue is full, new events are
dropped and counted rather than stopping the session. The session stops when
the connection ends; the last counts stay visible. If the session cannot start,
the panel shows why and the metrics keep working.

An ETW session outlives a profiler that crashes or is killed, and keeps the
providers enabled in the processes it filters on. Sessions are named
`WinUIProfiler-<profiler process ID>-<unique ID>`; starting a session stops any
whose profiler process no longer exists. To stop one by hand, find it with
`logman query -ets` and run `logman stop <name> -ets`.

### Session files

While connected, or while it reads a trace, the app saves every event it
receives to a SQLite file, one per connection or trace read, in
`%LOCALAPPDATA%\WinUIProfiler\Sessions`, named
`<date>-<time>-<process>-<pid>.sqlite`. A writer thread commits a batch every
2,000 events or 250 ms; if it falls behind, the processing thread waits for it,
so events are still only dropped at the queue.

| Table | Contents |
| --- | --- |
| `SessionInfo` | One row: the process, source (live or trace), coverage, time zero and QPC frequency, size limit, how and when the session ended, events lost by ETW and dropped by the profiler. For a live session, `StartedUtc` and `StartedQpc` are read together, which places the process's creation time on the event clock. For a trace, `ProcessStartQpc` is the process's creation, when the trace has the kernel's process events. `LauncherProcessId` and `LauncherName` are the process that launched it, when known: the profiler for a launch from the app, or for a trace, the parent the kernel's event for the process's creation names |
| `EventTypes` | One row per provider, event name, opcode, field layout and kind: the field names and their .NET types, such as `Int64,Int32,String,Boolean`, and `TraceLogging`, 1 for a TraceLogging event and 0 for a manifest event. `ProviderName` and `ProviderId` are the ETW provider's name and GUID; `Provider` is `Marker` for a provider of the marker configuration. `Launcher` is 1 for an event of the launcher (see [Launcher milestones](#launcher-milestones)), and 0 for the app's |
| `Events` | One row per event: `Seq` (arrival order), `Qpc`, thread, `TypeId` (its row in `EventTypes`), and the field values as a JSON array in field order |
| `Objects` | One row per UI element or composition visual the events mention: `Kind` (`Element` or `Visual`), address, type, and an element's `x:Name` or a visual's owner element label (see below) |
| `ObjectEvents` | One row per mention: `Seq`, `ObjectId`, and the index of the event's field that holds the object's address. An event can mention several objects, such as `ChildAdded`'s parent and child. `-1 - i` marks a visual whose parent, in field `i`, had its children cleared |
| `Frames` | One row per tick on a UI thread: thread, frame number, start and end (see below) |
| `FrameEvents` | One row per UI-thread event: `Seq`, the `FrameId` of its tick, and its bucket: `Xaml`, `Composition` or `Other` |

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

### Shared queries (views)

The rules that turn the tables into the Frames tab's data are SQL views in the
session file (`Storage\SessionSchema.cs`), not C# code. The app, the CLI and
any SQLite reader, such as an AI agent with only the file, read the same rows
from them. Each view's description is inside its definition, so
`SELECT sql FROM sqlite_master WHERE type = 'view'` shows it, and
`SELECT * FROM QueryCatalog` lists the views with an example query each.
Times are in ms from the start of the recording.

| View | Rows | Used by |
| --- | --- | --- |
| `EventDetails` | Every event, with its fields by name (JSON object), its tick and bucket, and the UI elements and visuals it mentions | The events of a region or row |
| `TickEvents` | The same columns for each tick's events, read through the tick's index | The events of a tick, `frames --frame` |
| `Ticks` | Every tick: times, `Drew`, `Slow` (over 16.7 ms), event counts by bucket | `frames` |
| `InterestingRows` | The regions of interest and the interesting rows (rules in Frames tab below), with `RegionId` and `TickId` | `interesting`, a region's rows |
| `InterestingRowEvents` | The events of each interesting row | Agents |
| `FrameList` | The Frames tab's list in order, with what each Show choice keeps (`InInteresting`, `InSlow`) | The Frames tab, `frames --list` |
| `LaunchSummary` | When the first 10 ticks that drew ended, from process start or recording start | The Frames tab |
| `QueryCatalog` | The views and an example query each | Agents |

```sql
SELECT * FROM FrameList WHERE InInteresting ORDER BY ListIndex;                 -- the tab with Show = Regions of interest
SELECT * FROM InterestingRows WHERE RegionId = :region ORDER BY StartSeq;       -- a region and the rows in it
SELECT Seq, Ms, Name, Bucket, Fields FROM TickEvents WHERE TickId = :tick ORDER BY Seq;   -- a tick's events
```

`InterestingRows` uses the `EventsByType` index. On a 148 MB file, it takes
about 0.15 s, and `FrameList` about 0.65 s. Files written before the views were
added are not read; record them again.

### UI elements

While saving events, the writer also indexes the UI elements they mention
(`Storage\ObjectIndex.cs`), in the same transaction, so trimming removes old
mentions with their events. The TraceLogging events of
Microsoft-Windows-XAML and Microsoft-Windows-XAML-Profiler are indexed, along
with older traces' `VirtualizationIsEnabledByLayout` manifest observation
when it has the version-1 element-data layout. New traces emit this observation
only as TraceLogging. None
creates or destroys an object, so an element's lifetime in the file is from
the first to the last event that mentions it. Other manifest events are not
indexed because they use the same field names for other objects too: `BeginStoryboard`'s
`ElementId` is the storyboard, and the `ParentId` of
Microsoft-Windows-XAML-Diagnostics' `PeerCreated` is a framework peer.

- An address is a UI element once it appears in a field that only ever holds a
  `CUIElement*` (listed in `ObjectIndex.RoleOf`).
- `DependencyObject_Enter`, `PeerAssociated` and `ContentChanged`'s content
  also carry other objects, so their mentions of an address are kept back until
  the address is known to be a UI element, then saved with it. The type and
  `x:Name` come from `DependencyObject_Enter`. When its type was not observed,
  existing template and render events can supply a fallback class name without
  adding metadata to the runtime's profiler events.
- The runtime reuses freed addresses. When `DependencyObject_Enter` reports a
  different type at a known address, a new element starts there. The new
  object's peer, parent and layout events can come first, so the old element's
  mentions since it (or its last child) left the tree move to the new one.
  Reuse by an object of the same type can't be told apart.
- `DependencyObject_Enter` reports the nearest built-in type: a `TabViewItem`
  shows as `ListViewItem`, and an app's own control as its built-in base class.
  Its type takes precedence over fallback class names; differences between a
  fallback class name and an enter type do not imply pointer reuse.
- Other objects (brushes, transforms, composition nodes, surfaces) are not
  indexed; they can still be found in `Events`.

### Visuals

The same index records the composition visuals (`IVisual`) the WUC visual and
visual surface events mention.

- `ParentVisualId`, `ChildVisualId` and `VisualId` only ever hold an `IVisual`
  pointer, so a visual is known from its first mention.
- Tree events carry visual/parent/child IDs and, for root changes, the target
  ID. `WucVisualChildInserted` and `WucVisualRootSet` also capture the concrete
  `VisualType`. They do not capture properties, insertion indices or owner
  comp-node IDs. ID-only traces remain readable but show `type not seen` for
  visuals unless a later event supplies their type.
- Older traces can also supply a visual's type and owner comp node. The
  owner's `CompPeerLinked` label (the element's `x:Name`, or its class name)
  becomes the visual's name. A comp node usually
  creates its visuals before it is linked, so the label is added when the link
  comes. Visuals inserted with owner 0, mostly leaf visuals, have no name.
- In older traces, an insertion with a different type or owner means a new
  visual reused the pointer, so a new visual starts there. Visuals get their
  children before they are inserted, so the old visual's mentions since it left
  the tree move to the new one. Reuse by a visual without type/owner metadata,
  or with the same type and owner, can't be told apart. A hand-in or hand-off
  visual that its element's re-created comp node inserts again also gets a new
  owner, so its history splits there.
- `WucVisualChildrenCleared` names only the parent, so each child it removes
  gets a mention through the parent's field (`child of ParentVisualId`).

### Frames

The writer also records which UI-thread tick each event belongs to
(`Storage\FrameIndex.cs`), in the same transaction. Each tick is a row of the
`Frames` table.

- `CoreServices_Frame` marks the start and end of every UI-thread tick with the
  core's frame counter, which only advances after a tick submits composition
  commands. A tick with nothing to submit keeps the number. So a frame number
  usually has one tick, and sometimes a few short ticks that drew nothing and
  then the one that drew. A tick drew if the thread's next tick has a higher
  number. For the thread's latest tick, this is not known yet.
- Events between a tick's start and its stop belong to that tick. Events
  between ticks belong to the thread's next tick, the one that renders them.
  So do a thread's events before its first tick. A stop without a start ends
  a tick that was in progress when the recording started. That tick has no
  `StartQpc`.
- Events on threads that never tick (the render and image decode threads) are
  not attributed. Nor are a UI thread's last events if no tick follows them.
- Buckets: `Xaml` for the tree, `DependencyObject_Enter`, templates, visual
  states and loading; `Composition` for composition node, WUC visual and
  surface events; `Other` for everything else (layout, rendering, scheduling).
  They go by the TraceLogging events' names, so manifest events about the same
  work, such as `ApplyTemplate/Start` or Microsoft-Windows-XAML-Diagnostics'
  `ElementAdded`, are `Other`. So are marker providers' events, whatever their
  names.

### Frames tab

The app's Frames tab (`Views\FramesView.xaml`) reads the session file of the
connection, or of the trace being read, once a second until the connection or
the reading ends: the `FrameList` view, when the file has new events. It keeps
the rows it read before, so the selection and the scroll position stay. When the
writer trims the oldest events, the tab reads the file again. **Open session
file...** shows a saved file instead, in this tab and the Objects tab, until the
next connection or trace read. A new connection or trace read clears both tabs
right away.

The list has one row for each tick, in time order. **Show** selects the rows:

- **All ticks**: every tick. A region of interest (see below) is a teal header
  row right above the tick it ends with, and that tick is tinted. An
  interesting row outside the regions, such as graphics device creation on a
  worker thread, is a gray row.
- **Regions of interest**: only the regions, their ticks and the gray rows.
  The `InterestingRows` view finds them with the rules below.
- **Slow ticks (over 16.7 ms)**: only the ticks that took more than 16.7 ms.

A tick shows its frame number, when it started, how long it took, a check mark
if it drew a frame, and its events per bucket. The bar is full at 50 ms, and a
warning icon marks more than 16.7 ms. A region header shows what started the
region, such as `Frame::Navigating`, and its span. The tints and accent bars
are in `App.xaml`. They give way to the list's hover and selection colors.

- Selecting a tick shows its events, 100 per page, all or one bucket. The
  filter keeps events whose name or a string field contains the text; a number
  (`42`, `0x1A2B`) matches numeric fields with exactly that value. When the
  selected event mentions UI elements or visuals, its details link to each
  one's history in the Objects tab. Each link has the same Element or Visual
  badge as the Objects tab.
- Selecting a region lists the interesting rows in it: its tick, `UpdateLayout`
  passes, operations, navigations and focus changes. Below them are its
  thread's events from the region's start to its end, with the same buckets,
  filter and pages. Selecting a gray row shows its thread's events from its
  start to its stop.
- For a launch recorded from process start, or a trace that has the process's
  creation, the tab gives when the first ten ticks that drew on the first UI
  thread ended, from the process's creation; otherwise from the start of the
  recording. A trimmed file no longer has them.
- `WinUIProfiler.Cli frames` prints the ticks, `frames --list` the tab's list
  (`--show interesting` or `slow` as the Show filter), and `interesting` the
  interesting rows (`--region <id>` for one region). `--json` prints the views'
  values.

The regions answer a different question from the tick time: which ticks
render a large change to the tree, not which ticks were slow. A slow tick with
no trigger before it is not interesting, and a fast one after a navigation is.
The rules:

- A region of interest starts on a UI thread at a navigation
  (`FrameNavigating`, `FrameNavigated`), at the start of a `PerfXamlEvent`
  (`WXM::InitializeForCurrentThread`, `DWXS::Initialize`, `MenuFlyout::ShowAt`,
  `FlyoutBase::ShowAt[WithOptions]`, `ContentDialog::ShowAsync[WithPlacement]`)
  or at a window focus change, and ends at the end of that thread's next tick.
  So a region holds one tick, and the time before it, such as building a page.
  A trigger during a tick moves the start back to that tick's start, and a
  trigger while a region is open is ignored. A focus change on a thread with
  no rows yet is held, and the next trigger's region starts at it.
- The rows are the region, and the ticks (`Frame`), `UpdateLayout` passes,
  `PerfXamlEvent` operations, navigations and focus changes that start in it.
  Operations, focus changes and graphics device creation
  (`CreateGraphicsDevice`, on a worker thread) are interesting anyway.
- A start while one of the same kind is open on the thread is ignored, as is a
  stop without a start, such as the end of a tick in progress when the
  recording started.

Limits:

- There is no CPU time per row: that needs the kernel's context switch events.
- WinAppSDK 1.4 and earlier trigger regions with `ApiFunctionCall` events,
  which are not read.

### Objects tab

The Objects tab (`Views\ObjectsView.xaml`) lists the UI elements and
composition visuals of the same session file, in the order they were first
seen, with their kind, type and name (a visual's is its owner element's label),
address, when an event first and last mentioned them, and how many events do.
The kind list shows all objects, only elements or only visuals; the search box
keeps the objects whose type, name or address contains the text. Selecting one
shows its history: the events that mention it, 100 per page, each with the
frame it belongs to. While connected or reading a trace, the tab reads the file
once a second while it is shown, keeping the list's scroll position and
selection.

Each row starts with an Element or Visual badge, and element rows have a light
blue background, visual rows a light amber one (darker shades in the dark
theme; the brushes are in `App.xaml`). The background gives way to the list's
own hover and selection colors, and high contrast uses the system colors, so
the badge is what always shows the kind. Opening an object from a tick's
event or a snapshot resets the kind list to all objects.

Snapshots don't record the runtime's element addresses, only each XAML
object's WinRT addresses (its `IUnknown` and its XAML diagnostics handle).
`PeerAssociated`, `ElementEnteredTree`, `ChildAdded` and `ChildInserted` carry
both an element's address and its WinRT object's, so
`SessionFile.FindSnapshotElement` maps a snapshot node to the element that was
at that address when the snapshot was taken (a marker provider's events of
those names aren't used). That needs the recording to have seen the element
being created or added to the tree: a session attached to a running app finds
only the elements added since.

A Composition node's address is its `IVisual` pointer, which the visual events
carry too, so `SessionFile.FindSnapshotVisual` finds the visual at that
address when the snapshot was taken. A visual inserted before the recording
started and not mentioned since isn't found.

Both lookups use the time the node's root finished being captured, not the end
of the whole snapshot: a root's objects can be released while other roots are
still being captured, and a new object can reuse the address.

### Recorded, from a trace file

To record a trace, use `logman`. Start it before the app: the kernel writes the
app's starting GPU memory values when the app opens a GPU adapter, and after
that only changes. `logman` enables the providers in every process, so other
XAML processes, such as explorer, write to the trace too; the larger buffers
(`-bs`, `-nb`) keep ETW from losing events. Then inspect the trace with
`WinUIProfiler.Cli`, which reads the file through `Ingestion\OfflineEtlSource.cs`.
The CLI reads session files the same way, and `record` writes a session file
from a trace. Run from this directory:

```powershell
'{531A35AB-63CE-4BCF-AA98-F88C7A89E455} 0xFFFFFFFFFFFFFFFF 5',
'{59E7A714-73A4-4147-B47E-0957048C75C4} 0xFFFFFFFFFFFFFFFF 5',
'{A1B2C3D4-E5F6-4A5B-9C8D-7E6F5A4B3C2D} 0xFFFFFFFFFFFFFFFF 5',
'{2DC72F6E-E4D1-5F58-3245-09A4243799DD} 0xFFFFFFFFFFFFFFFF 5',
'{21E0AE07-56A7-55B5-12F9-011E6BC08CCA} 0xFFFFFFFFFFFFFFFF 5',
'{802EC45A-1E99-4B83-9920-87C98277BA9D} 0x4 5',
'{8C416C79-D49B-4F01-A467-E56D3AA8234C} 0x2000 4' | Set-Content providers.txt
logman create trace XamlTrace -o xaml.etl -pf providers.txt -bs 1024 -nb 16 256 -ets
# ... use the target app ...
logman stop XamlTrace -ets

dotnet build .\WinUIProfiler.Cli\WinUIProfiler.Cli.csproj -c Release
$cli = '.\WinUIProfiler.Cli\bin\Release\net10.0\WinUIProfiler.Cli.exe'
& $cli processes xaml.etl                      # event counts of the processes with XAML events, to find the PID
& $cli summary xaml.etl --pid <pid>            # event counts for that process
& $cli dump xaml.etl --pid <pid> --name CoreServices_Frame --max 20   # the events themselves
& $cli record xaml.etl --pid <pid> --out session.sqlite [--limit-mb 500]  # a session file from the trace
& $cli summary session.sqlite                  # a session file holds one process; --pid is optional
& $cli objects session.sqlite --type Button    # UI elements and visuals (--kind element|visual), with first/last seen and event counts
& $cli timeline session.sqlite --object <id>   # the events that mention one object, with their frames (or --address 0x...)
& $cli frames session.sqlite                   # ticks per UI thread, with timing, whether each drew, and events per bucket
& $cli frames session.sqlite --frame <id> --bucket Xaml --filter Measure   # one tick's events, as in the Frames tab
& $cli interesting session.sqlite              # the regions of interest and the interesting rows in them, with what started each region
& $cli frames session.sqlite --list --show interesting   # the Frames tab's list, as Show = Regions of interest keeps it, with region IDs
& $cli interesting session.sqlite --region <id> --json   # one region and its rows, as JSON
& $cli launch xaml.etl --pid <pid> [--markers markers.json]   # the launch timeline, as in the Launch tab
& $cli launch session.sqlite                   # the same, from a session file
```

The app reads a trace too (`Ingestion\TraceReplay.cs`). Select **Recorded
trace (.etl)** as the connection mode, then type the trace's path or browse for
it. The app lists the processes with XAML events in the trace, most events
first, with their event counts and the trace times of their first and last
events, and selects the process if there is only one. A process is named only
when the trace has the kernel's process events, as a WPR trace has. **Open
trace** reads the selected process's events into a new session file, named
after the process, or after the trace when the process has no name. The events
go through the same pipeline and processors as a live session's, so the file
holds the same events, UI elements, visuals and ticks as one `record` writes
from the trace, and the Launch, Frames and Objects tabs follow it as it grows.

- A reader thread reads the file in its order and posts the process's events to
  the processing thread's queue, which holds as many events as a live
  session's. When the queue is full, the reader waits instead of dropping
  events, so no event of the process is dropped, and the connection panel's
  dropped count stays 0. Its lost count is the number of events ETW lost while
  the trace was recorded, of any process. As in a live session, the 500 MB
  limit can still delete the oldest events from the session file (see Session
  files above); the panel then says from when the file is complete.
- ETW delivers a trace's events in timestamp order, merging the processors'
  buffers, but events with the same timestamp on different processors can come
  in any order. The events keep the file's order; the connection panel
  counts the process's events whose timestamp is earlier than the event before
  them as out of order. In the traces measured, there were none.
- Timestamps in a trace count ticks of the recording machine's clock, whose
  frequency can differ from this machine's, such as 24 MHz on many Arm64 PCs
  and 10 MHz on many x64 PCs. Before reading the process's events, the app
  works out the trace's frequency from its first events and stores it in the
  session file, so their times are right from the first frame; `record` does
  the same. When it can't be worked out, as in a damaged or very short trace, it
  is derived from the process's events once they span a second.
- The connection panel shows how much of the trace has been read. **Stop**
  ends the reading, as does closing the app: the events already read are
  stored, so the session file holds the process's events from the start of the
  trace to about where the reading stopped.
- Most events of other providers, such as the kernel's in a WPR trace, are
  skipped without being decoded, in the CLI too. The kernel's process events,
  which name the processes in the list, and the graphics kernel's events, among
  them the GPU memory events, are still decoded. On the machine measured,
  listing the processes of a 17.9-second WPR trace of the WinUI 3 Gallery took
  about a second, and reading the app's 101,281 events less than two.
- A trace has no running process, so there are no live metrics or tree
  snapshots for it.

## Launch timeline

The Launch tab (`Views\LaunchView.xaml`) shows when the target reached each
milestone of its launch: the built-in ones, the process's creation and the
first frame drawn, and those of the marker configuration, such as the first
event an app writes when it is ready to use, or one its launcher writes as it
starts the app (see [Launcher milestones](#launcher-milestones)). It reads the
session file the Frames and Objects tabs show, once a second while it is
recorded, so it follows a launch from the app as it happens. The CLI's `launch`
command prints the same timeline for a trace or a session file.
`Launch\LaunchTimeline.cs` finds the milestones.

- Times are in milliseconds from the earliest launcher milestone before the
  process's creation, if there is one, or else from the creation, which the
  session knows for a launch from the app, an attach, and a trace with the
  kernel's process events, as a WPR trace has; the summary says which.
  Otherwise, as in a trace recorded with the `logman` commands above, they are
  from the start of the recording, the process's first event in the trace,
  and the tab says so.
- The milestones are in time order. Each shows its time, the time since the
  milestone before, a bar from the milestone before to it on a scale that ends
  at the last milestone, the event that marked it, "(launcher)" for a launcher
  milestone, its thread, and how many events marked it; its time is the first
  one's, or for a launcher milestone, the one nearest the process's creation.
  The milestones not reached are listed below them.
- The first frame drawn is when the first tick that drew on the first UI thread
  ended, the first of the ticks the Frames tab gives for a launch.
- Attached to a running process, the recording starts after the launch: the
  tab says when, from the process's creation, and the milestones before it,
  and the first frame drawn, are missing.
- When the session file has deleted its oldest events to stay within its size
  limit before the tab read them, as when **Reload** reads a long recording
  again, milestones among them may be missing or show a later event, and the
  tab says so. The first frame drawn is then not reached unless the tab found
  it before, as the Frames tab shows no launch frames for such a file.

### Marker configuration

The marker configuration is a JSON file, which can have comments and trailing
commas: the providers to record along with the XAML providers, and the
milestones. The profiler uses `%LOCALAPPDATA%\WinUIProfiler\markers.json` when
it exists, and otherwise the default, `Launch\markers.json`: Windows' launch
events (see [Launcher milestones](#launcher-milestones)), the process's
creation, the first UI thread frame (`Microsoft.UI.Xaml`'s
`FirstUiThreadFrameEnd`), the first frame drawn, Windows' measures of when the
app's UI responded and was complete, which some apps log, such as Paint, and
the Microsoft Store's launch scenarios. **Edit markers** in the Launch tab
creates the file from the default the first time and opens it; once it is
saved, **Reload** reads it again and shows its milestones. When the file is
not valid, the tab says why and uses the default, so a mistake in it never
stops a connection. The CLI does the same, with a warning; it reads another
file with `--markers`, and stops with exit code 3 when that file is not valid.

The configuration's providers are recorded for the same process as the XAML
providers, live at their level and keywords, and from a trace as the trace has
them; those a launcher milestone names are recorded for the launcher too.
Their events are saved to the session file. A provider added to the
configuration is recorded from the next connection or trace read; a milestone
added for a provider already recorded shows at once.

An app marks a milestone by writing an event, such as with an `EventSource` in
C#:

```csharp
private static readonly EventSource Markers = new("Contoso.App");

// When the app is ready to use:
Markers.Write("ReadyToInteract");
```

and a configuration that names it:

```json
{
  "version": 1,
  "providers": [
    { "name": "Contoso.App" }
  ],
  "milestones": [
    { "builtin": "ProcessCreated" },
    { "builtin": "FirstFrameDrawn" },
    { "provider": "Contoso.App", "event": "ReadyToInteract", "label": "Ready to interact" }
  ]
}
```

The file has `version` 1, `providers` (optional) and `milestones`. A provider
has:

| Property | Meaning |
| --- | --- |
| `name` | Required. Without `guid`, the provider's GUID is the one EventSource and TraceLogging derive from its name |
| `guid` | The GUID of a provider whose GUID is not derived from its name |
| `level` | From 1 (critical) to 5 (verbose, the default), or a provider's own higher level |
| `keywords` | A number, or a string such as `"0x400000000000"`; all by default |

The XAML providers, Microsoft-Windows-DxgKrnl and Microsoft-Windows-Win32k,
which the profiler always records, can't be in `providers`; milestones name
them without listing them. A milestone has:

| Property | Meaning |
| --- | --- |
| `builtin` | `ProcessCreated` or `FirstFrameDrawn`, labelled "Process created" and "First frame drawn" unless it has a `label` |
| `provider`, `event` | Instead of `builtin`: the first event of this name from this provider marks the milestone. The provider is one in `providers`, or one the profiler always records, such as `Microsoft.UI.Xaml` |
| `where` | Field values the event needs, such as `{ "Items": 200 }`. A number matches a numeric field with that value; a string, `true` or `false` matches the field's value as text, case-insensitively |
| `label` | The milestone's name |
| `labelFrom` | Instead of `label`: a field of the event. The first event with each value of the field is a milestone named by the value, as the Store's `PerfMetrics` events name each launch scenario in `scenario` |
| `process` | An executable name, with or without `.exe`: the milestone is only for that app, and for a session whose process name is not known |
| `from` | `app`, the default, or `launcher`: an event of the process that launched the app marks the milestone (see [Launcher milestones](#launcher-milestones)). Its provider is one in `providers`; a built-in milestone has no `from` |

Provider, event and field names match case-insensitively. Property names are
case-sensitive, and an unknown one is an error. An EventSource method named
`XxxStart` or `XxxStop` writes the event `Xxx/Start` or `Xxx/Stop`, and `Write`
the event it names.

A trace has an app's events only if its provider was recorded: add its GUID to
`providers.txt` (see [Recorded, from a trace file](#recorded-from-a-trace-file)).
PowerShell gives an EventSource's:
`[System.Diagnostics.Tracing.EventSource]::new('Contoso.App').Guid`.

### Launcher milestones

A milestone with `"from": "launcher"` is marked by an event of the target's
launcher, the process that launched it: the profiler for a launch from the
app, or in a trace, the process that created it, such as File Explorer or
PowerShell. The default configuration has three, from Windows' launch events:
**User click** (`AppLaunch_UserClick`), which the shell writes for a user's
click that launches an app, but not for a launch by UI Automation,
**Activation requested** (`TryActivateDesktopAppXApplication/Start`), when a
packaged desktop app is activated, and **ShellExecute**
(`ShellExecuteExW/Start`). The times are from the earliest of them before the
process's creation, so the timeline starts where the launch did; the summary
says which, and names the launcher.

- The session knows the launcher for a launch from the app, the profiler, and
  for a trace with the kernel's process events, from the event for the
  process's creation, which names its parent. An attach, or a trace without
  those events, has no launcher milestones, and doesn't list them as not
  reached.
- A launcher can launch other apps, so only its events within 10 seconds of
  the process's creation count, and the one nearest the creation marks the
  milestone. A start event after the creation started something else, such as
  the profiler's `ShellExecuteExW/Start` when **Edit markers** opens the
  configuration, and doesn't count.
- In a trace, only the launcher's events that are part of the launch count, so
  each app a script launches gets its own:
  - An event with an activity ID, as the start and stop events of
    `ShellExecuteExW` and `TryActivateDesktopAppXApplication` have, counts
    when the launcher created the process during the activity, whose stop
    event is at most 10 seconds after the creation, and the thread of the
    activity's start event created the process; or else no thread created it
    while running another of the launcher's activities, but one did create
    each other process the launcher created during the activity. So when a
    script's launches overlap, each app gets its own; but when the trace
    doesn't say which threads created them, an activity during which the
    launcher created more than one process counts for none of them. The
    thread that created a process is the parent of its first thread, from the
    kernel's thread events, when it is a thread of the process's parent. Such
    a call starts up to hundreds of milliseconds before the creation, so its
    start can be nearer the launcher's previous creation; it can create the
    process on another of its threads; and a call that created no process, as
    when the launch's process is created by another process, doesn't count
    for a process the launcher creates later.
  - An event without an activity ID counts as an event of the activity its
    thread was in: that of the thread's latest event with an activity ID,
    when the activity's start event is one of the launcher's events within 10
    seconds of the creation and the activity hadn't stopped before it.
    `ShellExecuteExW` writes `AppLaunch_UserClick` without one on a thread of
    its own, as for PowerShell's `Start-Process ms-settings:`, a call that
    creates no process itself, so that click doesn't count for a process
    PowerShell creates later.
  - A start event of no activity, or whose stop event is more than 10 seconds
    after the creation or not in the trace, counts when the process was the
    first the launcher created after it.
  - Any other event counts when it is nearer the creation than the launcher's
    creation of any other process.
  - The launcher is the process with its ID from its start until its end, so
    the events of another process with the same ID don't count.
- File Explorer opening an app's executable writes no `ShellExecuteExW/Start`,
  and writes `AppLaunch_UserClick` after it creates the process, 34 ms after in
  the trace measured, so the times of such a launch are from the creation, and
  User click comes after it.
- The launcher's events are saved to the session file, but the Frames and
  Objects tabs show only the target's.
- For a launch from the app, the profiler activates a packaged app itself, so
  the launch has **Activation requested** and **ShellExecute**, but creates an
  unpackaged app's process directly, which writes neither. With private WinUI
  DLLs, `ProfilerLauncher.exe` starts the app, so the launch has neither.
- A UWP app's process is created by a Windows service, svchost.exe, rather
  than by the process that launched it, so its launcher is that service.

A trace has the launcher milestones only if it has, in one file, the kernel's
process events and the configuration's providers, and tells overlapping
launches apart with the kernel's thread events. The `logman` commands of
[Recorded, from a trace file](#recorded-from-a-trace-file) don't record the
kernel's process and thread events. WPR can record them in one file with the
providers: use a profile with the kernel's process and thread events, the
providers in `providers.txt`, and the configuration's providers, such as an
app's own. Microsoft-Windows-DxgKrnl and Microsoft-Windows-Win32k write from
the kernel, so only to a session in nonpaged memory, which WPR uses only for a
provider with `NonPagedMemory="true"`: give them that, in a small collector of
their own, or their GPU memory and window focus events are lost. The default
configuration's providers are:

| Provider | GUID |
| --- | --- |
| Microsoft.Windows.AppLifeCycle.UI | `ee97cdc4-b095-5c70-6e37-a541eb74c2b5` |
| Microsoft.Windows.ApplicationModel.DesktopAppx | `5526aed1-f6e5-5896-cbf0-27d9f59b6be7` |
| Microsoft.Windows.ShellExecute | `382b5e24-181e-417f-a8d6-2155f749e724` |
| Microsoft.Windows.AppLifeCycle | `ef00584a-2655-462c-bc24-e7de630e7fbf` |
| Microsoft-Windows-Store | `9c2a37f3-e5fd-5cae-bcd1-43dafeee1ff0` |

Then, with the profile in `launch.wprp` and `$cli` as above:

```powershell
wpr -start launch.wprp -filemode               # from an elevated prompt, before the launch
# ... launch the app ...
wpr -stop launch.etl
& $cli processes launch.etl                    # to find the app's PID
& $cli launch launch.etl --pid <pid>
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
