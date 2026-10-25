# Profiler application foundation

`WinUIProfiler.App` is the C# WinUI application in this standalone solution.
It launches/connects to local WinUI targets and inspects real saved SQLite
sessions and their committed metric pages. It is not the complete P15
exploration/capture UI.

## Launch or connect to a target

The live panel supports **Unpackaged executable** (full `.exe` path, optional
arguments and working directory), **Packaged application** (installed current-user
`PackageFamilyName!ApplicationId` AUMID), and **Running process** (positive PID).
Use **Browse...** beside the target field to select an `.exe`, a current-user
installed packaged application's display name/AUMID, or a running process
name/PID. Packaged/process lists support search and **Refresh list**.
Selection fills the field only; it never launches or connects. Cancel preserves
the previous input, and manual entry remains available. Inaccessible entries
show their error and cannot be selected. Process selection also retains its
creation time: if that PID is reused before connection, attach fails with an
explicit request to refresh/reselect. Editing the field or changing modes
returns to manual selection without a retained creation-time expectation.
The native executable dialog is owned by the profiler window and supports
elevated as well as ordinary desktop execution.

Select **Trust local runtime, injector and TAP**, then **Launch and connect**.
Injection executes native code in the target: trust is an explicit prerequisite,
not a signature/OS-security bypass. Compatible retail WinUI runtimes support
the injected TAP's authenticated pipe and hello through standard `IXamlDiagnostics`.
The private profiler runtime bridge is not required for that transport. It is
required for private producer baseline/capture features; its absence is shown explicitly.
TAP-owned tree snapshots below do not require that private bridge.
ETW collection requires the account's existing ETW permissions; the app does not
elevate or change permissions.

The build stages SHA256-checked x64 and ARM64 injector/TAP payloads in
`ConnectionPayloads`. Target PID, creation time, user and architecture are
validated again by the matching helper. The diagnostics endpoint defaults to
1; multi-dispatcher apps may require another explicit endpoint number.
The controller waits for WinUI's diagnostics-ready event, authenticates the
target named pipe, reads capabilities, then sends literal `hello` and requires
the correlated native TAP `hello` reply before displaying **Connected**.
An initializer acknowledgement alone is not a connection.

ETW starts on its own listener thread before launch. A bounded queue transfers
owned data to a separate decoder and then the recording owner/SQLite writer;
no UI or database waits run in the ETW callback. The default profile enables
capabilities and layout/frame keywords (`0x11`). **Detailed events** enables
object/tree/composition/resource observations as well (`0x3f`), excluding
allocator and baseline-capture detail. Both profiles also enable the standard
`Microsoft-Windows-XAML` provider (`531a35ab-63ce-4bcf-aa98-f88c7a89e455`):
summary selects Core/Layout/Rendering (`0x1044`) at informational level;
detailed selects all keywords at verbose level. Standard events retain their original provider, name,
descriptor, activity/thread identity, QPC and complete bounded payload bytes as
`ObservedXaml` source evidence, not invented private object/frame observations.
They have partial semantic coverage and no fabricated source sequence/runtime epoch.
Their observation-stream GUID is trace-local. Detailed startup can exceed the finite
queues; this fails visibly with incomplete-tail diagnostics, not silent drops.
Connection recording saves supported XAML source observations and projects the
five process metrics below. It does not derive object/tree/resource state.
Complete pre-attach state, exact process-creation QPC, a raw `.etl` file and
baseline/rundown are not supplied by this connection feature.

The panel displays the actual PID, event/queue counts and saved SQLite path
under `%LOCALAPPDATA%\WinUIProfiler\Sessions`. **Disconnect** cancels an in-flight
connection or drains an established recording without killing its target.
Closing the profiler also awaits cleanup. Target exit ends and drains recording.
A hello probe every five seconds maintains the native pipe's 30-second idle
deadline. The native limit of 4096 distinct requests still applies (roughly
5.7 hours at this cadence); pipe loss, timeout and exhausted limits are explicit
failures, not automatic/reported-success reconnects.

### Live process graphs

After the authenticated TAP hello, the controller starts independent native
sampling with a one-second delay between acquisitions. **Live metrics** shows
CPU and busiest-engine GPU utilization in percent, and total working set,
private working set and private commit separately in MiB. CPU/memory use
`NtQuerySystemInformation(SystemProcessInformation)` process snapshots; CPU
also uses processor-group `NtQuerySystemInformationEx` counters. GPU uses
English PDH GPU Engine counters. CPU is processor-time utilization, not
frequency-adjusted utility or a claim of exact Task Manager numeric parity.
See [metric definitions and limitations](metrics.md#connected-process-graphs).

Samples enter the same collector ETW, bounded processing and SQLite pipeline.
Graph refresh reads committed data off the UI thread; it does not poll native
counters. Each graph retains at most 121 points over two minutes. Warm-up,
unavailable measurements and acquisition gaps break the line instead of
inventing zero. Each chart displays its latest committed value, units,
source and quality, with an accessible summary. GPU-memory byte queries are
not enabled by these five graphs.

Connection settings collapse while connected. **Saved session** hides the live
connection panel and retains the read-only paged inspector; `--session` selects
that tab automatically. Switching tabs does not stop acquisition.
Disconnect stops and joins native polling before ETW drain, preserving the
final committed graph window without terminating the target. A new connection
resets graph scope/history; all original samples remain in the SQLite file.
The panel reports XAML event counts separately from collector-inclusive
ETW counts, so metric activity cannot masquerade as continued target XAML.

### Take a tree snapshot

While connected, select **Take snapshot**. The authenticated `TakeSnapshot`
command executes traversal and serialization inside the injected TAP, not the
product DLL or profiler UI. It collects exposed, realized XAML trees and
composition visual trees and returns explicit XAML-object-to-visual links.
The panel shows root/node/link counts and the saved `.trees.json` path beside
the recording. Repeated captures have separate capture IDs/files.

Node IDs are snapshot-local. `objectAddress`, `diagnosticsHandle`, and
`visualAddress` retain actual object/diagnostics identities as hexadecimal
metadata; `correlations` references the matching XAML and composition node IDs.
These are not private core-object, HWCompNode, ETW lifetime or producer-epoch
identities. Uninstantiated templates, unrealized virtualized objects, private
composition nodes and other unexposed state cannot be represented as a full
private baseline. Each root records its own UI-thread capture interval;
different roots are not one globally atomic observation. Public
`GetElementVisual` can materialize handout visuals, so capture may perturb
composition state. The document records coverage/perturbation limitations.

Transfers are bounded, chunked and validated before atomic file publication.
Cancellation, invalid graphs and incomplete transfers never publish a partial
snapshot. Unsupported capture or traversal failures show their actual error.
Heartbeat commands wait outside their normal timeout while a snapshot owns
the TAP command channel. The independent ETW listener/decoder/recording queue
continues running. Disconnect/close cancels an in-flight snapshot.

### Snapshot history and viewer

Each successful capture adds a newest-first entry to **Snapshots**, showing
its local save time, PID, capture ID and XAML/visual/link counts. History stays
available after disconnect and reconnect during this profiler run. The JSON
files persist across app restarts; automatic discovery of previous runs is not
part of this list.

Select an entry to open its read-only viewer. Reopening the same entry activates
its existing window; different captures can have separate viewers. Saved files
load and validate off the UI thread. Missing, malformed or mismatched files
display an error rather than an empty successful tree. Closing a viewer does
not disconnect the target. Closing the main profiler closes its viewers and
cancels outstanding loads.

XAML and composition trees appear side by side with lazy branch expansion.
Selecting an object reveals and selects its direct counterpart in the other
tree. For multiple direct counterparts, the first is selected, all are
highlighted, and **Direct mapped nodes** lets you reveal another match. Native
TreeView multi-selection cascades to descendants, so it is deliberately not
used to represent exact object mappings. Unmapped selections clear the other
tree; ancestor or geometry guesses are never substituted for captured links.
Selected-object details show full types, object/interface addresses, diagnostics
handles when available and saved properties.

Metrics show distinct XAML nodes, composition visuals, XamlRoots, correlation
links, both maximum tree depths and composition visual counts by full type.
Depth counts actual parent edges as levels: a parentless node is level 1.
Synthetic root display groups do not change counts or depths. Metrics describe
the captured public-tree scope, not all process allocations or private visuals.

Real launch/attach and UI qualification commands, from `tools\WinUIProfiler`:

```powershell
dotnet test WinUIProfiler.IntegrationTests\WinUIProfiler.IntegrationTests.csproj -c Release --no-restore --filter "FullyQualifiedName~TargetConnectionTests&Category!=PackagedConnection"
powershell.exe -NoProfile -File scripts\Tests\Test-PackagedConnection.ps1
# Also discover/select the owned packaged fixture and exercise all three pickers:
powershell.exe -NoProfile -File scripts\Tests\Test-PackagedConnection.ps1 -VerifyTargetSelection
# Exercise an existing installed app without registering or replacing its package/runtime:
powershell.exe -NoProfile -File scripts\Tests\Test-InstalledPackagedConnection.ps1 `
    -Executable "$output\WinUIProfiler.App.exe" -ApplicationAumid "PackageFamilyName!ApplicationId"
# Also verify repeated TAP tree capture, durable files, subsequent hello and ETW:
powershell.exe -NoProfile -File scripts\Tests\Test-InstalledPackagedConnection.ps1 `
    -Executable "$output\WinUIProfiler.App.exe" -ApplicationAumid "PackageFamilyName!ApplicationId" -VerifySnapshots
# Also verify history, both selection directions, exact metrics, window reuse,
# missing-file errors and reopening saved entries after disconnect:
powershell.exe -NoProfile -File scripts\Tests\Test-InstalledPackagedConnection.ps1 `
    -Executable "$output\WinUIProfiler.App.exe" -ApplicationAumid "PackageFamilyName!ApplicationId" -VerifyViewer
# Verify all five live charts, progression, independent memory readings and
# final stopped retention; -LeaveConnected additionally exercises reconnect:
powershell.exe -NoProfile -File scripts\Tests\Test-InstalledPackagedConnection.ps1 `
    -Executable "$output\WinUIProfiler.App.exe" -ApplicationAumid "PackageFamilyName!ApplicationId" -VerifyMetrics
powershell.exe -NoProfile -File scripts\Tests\Test-TargetSelectionUi.ps1 `
    -Executable "$output\WinUIProfiler.App.exe"
powershell.exe -NoProfile -File scripts\Tests\Test-LiveConnectionUi.ps1 `
    -Executable "$output\WinUIProfiler.App.exe" -TargetExecutable "$output\WinUIProfiler.App.exe"
# Exercise Disconnect followed immediately by repeated window-close requests:
powershell.exe -NoProfile -File scripts\Tests\Test-LiveConnectionUi.ps1 `
    -Executable "$output\WinUIProfiler.App.exe" -TargetExecutable "$output\WinUIProfiler.App.exe" -CloseDuringDisconnect
```

The packaged wrapper registers only its owned current-user fixture, derives
WinRT registrations from the built executable manifest and installs the
matching `resources.pri` layout. It refuses existing identity/files and removes
its registration/files in `finally`; it does not package or alter the selected
user app. Qualification covers x64 execution. ARM64 payload builds are not
ARM64 execution or cross-architecture module-discovery qualification.
The live UI tests additionally reopen the recording through the read-only CLI,
requiring a nonempty durable watermark and terminal Stopped status.
The target-selection UI test also attaches to the selected process, requires its
real TAP hello and XAML ETW events, verifies Browse is disabled while connected,
then disconnects and checks durable recording without terminating the target.

## Build and local runtime prerequisites

Use Windows 11 and full Visual Studio MSBuild with the WinUI XAML compiler.
The tool's `global.json` pins its .NET SDK independently of the framework build.
`Build-App.ps1` evaluates framework properties in a child process so Product
initialization cannot change the app's SDK selection. It reads structured
MSBuild properties and validates the native project identity; diagnostic file
paths are not interpreted as possible Product directories.

Build the matching local WinUI Product first. Localized controls also require
both framework satellite targets. For Release/x64, these commands run from the
repository root after flavor initialization:

```powershell
.\initrun.ps1 -Flavor amd64fre msb /q dxaml\xcp\dxaml\dllsrv\winrt\native\Microsoft.ui.xaml.vcxproj /p:PGOBuildMode=None
.\initrun.ps1 -Flavor amd64fre msb /q dxaml\xcp\dxaml\dllsrv\winrt\res\common\Microsoft.ui.xaml.resources.common.vcxproj /p:PGOBuildMode=None
.\initrun.ps1 -Flavor amd64fre msb /q dxaml\xcp\dxaml\dllsrv\winrt\res\19h1\Microsoft.ui.xaml.resources.19h1.vcxproj /p:PGOBuildMode=None
```

`PGOBuildMode=None` selects an optimized local development build without a PGO
training package. It is not shipping PGO or instrumentation-overhead evidence.

From `tools\WinUIProfiler`:

```powershell
.\scripts\Build-App.ps1 -Restore -Configuration Release -Architecture x64
# Include the native collector DLL and test projects in the mixed solution build:
.\scripts\Build-App.ps1 -Restore -Solution -Configuration Release -Architecture x64
```

Supported configurations are Debug/Release and x64/ARM64. Debug selects the
checked Product; Release selects fre. An ARM64 build needs the corresponding
complete ARM64 Product and is not native ARM64 execution on an x64 machine.
Restore includes both `win-x64` and `win-arm64`, so switching architecture does
not require replacing the shared dependency lock with a different RID graph.

The app uses `WindowsPackageType=None`, `WindowsAppSDKSelfContained=true`,
and `SelfContained=true`; it does not bootstrap an installed shared runtime.
The shared deployment targets replace the SDK's WinUI component with the
complete local component while retaining the other SDK components. Required
files include the core runtime, common and 19H1 satellite DLLs, en-US MUI and
controls PRI. A missing common satellite causes native `TextBox` template
initialization to fail even when the core DLL loads successfully.

The build script verifies the evaluated architecture/self-contained settings,
the core PE machine and the source/destination SHA-256 hashes. It writes
`ProfilerAppDeployment.json` beside the executable and an evaluated output
record under `WinUIProfiler.App\obj`; no machine-specific Product path is
hard-coded into the app.

## Open a saved session

```powershell
$output = (Get-Content WinUIProfiler.App\obj\ProfilerAppOutput.x64.Release.txt -Raw).Trim()
& "$output\WinUIProfiler.App.exe" --session C:\captures\session.sqlite
```

Alternatively, launch without arguments and use the session-path field and
**Open** button. Invalid files and query failures are shown in the error bar;
the previously loaded document is preserved.

Reads run off the UI thread and open SQLite read-only. Pages are bounded to
256 rows and remain pinned to one committed watermark until **Refresh**.
Paging/refresh reject replacement of the selected session identity. The view
shows the recorded process instance, raw QPC/FILETIME values, status, metric
source/version, quality, coverage reason and original native observation when
present. Missing observations are unavailable, not measured zero.

## Verification

The engine tests exercise real read-only session queries, paging, refresh,
cancellation, missing measurements and rejection of incomplete local
deployments by the actual shared MSBuild target:

```powershell
dotnet test WinUIProfiler.Engine.Tests\WinUIProfiler.Engine.Tests.csproj -c Release --filter "FullyQualifiedName~SavedSessionAppTests|FullyQualifiedName~SavedSessionViewModelTests"
```

Use Windows PowerShell for the actual UI Automation smoke test. Supply values
known to exist in the selected fixture; synthetic fixtures must remain clearly
identified as synthetic, not presented as OS measurements.

```powershell
powershell.exe -NoProfile -File scripts\Test-App.ps1 `
    -Deployment "$output\ProfilerAppDeployment.json" `
    -SessionPath C:\captures\session.sqlite `
    -ExpectedSource "known source / version" -ExpectedValue "4096"
```

The smoke test checks rendered source/value/quality, the final-page navigation
state, actual loaded runtime paths/hashes, unchanged database hash and clean
process exit. For a fixture spanning exactly two pages (257-512 samples),
`-ExpectedNextValue` checks Next, final-page state, Previous and the restored
first-page state. `-VerifyReadError` opens a unique nonexistent path, requires
the visible error without losing the existing document or creating a database,
then reopens the valid session and requires recovery. For example, a clearly
identified synthetic 300-sample fixture whose values are `index * 4096` uses:

```powershell
powershell.exe -NoProfile -File scripts\Test-App.ps1 `
    -Deployment "$output\ProfilerAppDeployment.json" `
    -SessionPath C:\captures\synthetic-300-samples.sqlite `
    -ExpectedSource "synthetic UI validation fixture / 1" -ExpectedValue "4096" `
    -ExpectedNextValue "1052672" -VerifyReadError
```

Run the same checks against Debug/checked and Release/fre deployments. The test writes
module evidence and stdout/stderr beside the executable. A startup or shutdown
failure is a failing test, not a suppressed exception.

`WINUI_PROFILER_TRACE_XAML=1` additionally enables resource-reference and binding
diagnostics on stderr. Unhandled exceptions are logged and remain unhandled.

## Intentionally absent

Synchronized graphs, startup/frame navigation,
object/XAML/composition/resource views, snapshots and heap analysis are not
implemented by this foundation. The UI does not manufacture those projections
or expose nonfunctional capture commands. Their engine/runtime dependencies
and full P15 validation remain separate unfinished deliverables.
