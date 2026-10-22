# Online XAML Profiler

A standalone live-snapshot tool. This is a new implementation: it does not use
`tools\XamlProfiler`, its TAP/helpers, or any ETW/ETL capture, parsing, or replay.

## Build

Install Visual Studio's .NET desktop and C++ desktop workloads, the Windows
10.0.26100 SDK, and .NET 9. From this directory:

```powershell
.\build.ps1 -Platform x64 -Configuration Debug -Test
```

The script restores public SDK package dependencies, builds the native injector and TAP first,
builds the unpackaged WinUI app, and optionally runs native TAP-walker tests and
focused protocol/tree tests. Running tests requires a host that can execute the
selected architecture.
Use `ARM64` or `x86` for those targets. Profiler, TAP, and target must have the same
architecture. Keep `OnlineXamlProfiler.Injector.exe` next to the TAP DLL. The output is under `artifacts\bin\OnlineXamlProfiler.App`; run
`OnlineXamlProfiler.App.exe` in its output directory, with its dependencies.

Core/transport tests can also be run independently:

```powershell
dotnet test .\OnlineXamlProfiler.Tests\OnlineXamlProfiler.Tests.csproj --filter "TestCategory!=Live"
```

## Required target runtime

The application being inspected **must load a runtime built from this change**.
The tool intentionally fails with an actionable diagnostic when the runtime lacks
the new `OnlineXamlProfilerWithTreeAccess` export. It does not silently substitute
ETW reconstruction or another tree source.

Build the repository's `Microsoft.UI.Xaml.dll` normally (initialize the repository
with `.\init.ps1 amd64chk`, then build through `build.cmd` or its documented
component-build workflow). Deploy the resulting runtime and its **matching**
dependencies using the target app's existing local-WinUI deployment mechanism.
For a self-contained test app this means its app-local WinUI runtime, not the
profiler's own WinUI files. For a framework-package app use a matching locally
built/deployed framework package. Do not replace files in an installed system
framework or mix binaries from incompatible Windows App SDK versions.

Restart the target after deploying; an already loaded DLL cannot be updated by
attaching. The connection status reports the full runtime path observed in the
target so you can verify the deployment. Injection needs access to the target
process: use the same account/integrity level, and a desktop WinUI 3 app with an
initialized UI. Protected processes, different users, System XAML/UWP, and
cross-architecture attach are not supported.

## Use

1. Enter an executable path and optional arguments, then **Launch executable**;
   or enter `PackageFamilyName!ApplicationId` and choose **Launch package**.
   Both launch paths attach to the returned process. If an app starts another
   process for its UI, attach to that UI process instead.
2. To inspect an already running application, enter its decimal PID and choose
   **Attach**. The default TAP DLL is next to the profiler; its path can be changed.
3. Choose **Take snapshot**. The three views show the captured XAML visual tree,
   internal WinUI HWCompNode tree, and composition IVisual tree. Select a node to
   inspect copied properties and identifiers. **Show XAML / HWCompNode / IVisual**
   follows the available cross-tree link in that same snapshot and UI thread.
   **Expand all trees** opens every descendant in all three views of the selected
   snapshot, without taking another capture.
4. Interact with the application and take another snapshot. The snapshot selector
   retains earlier captures in memory, so A and B can be inspected independently.
   **Disconnect** leaves the target running and retains captured data.

Snapshot transport, validation, and plain-data tree construction run off the UI
thread. Native TreeView nodes are published in cancellable UI-thread batches;
their complete parent graph allows links to select even previously collapsed
descendants. The native diagnostics-injection call runs in a short-lived, separate helper,
using the FrameworkUdk DLL already loaded by the target. This avoids mixing
different SDK diagnostic payload layouts or loading the target's SDK into the
profiler UI. Cancel stops the owned injector and aborts connection/capture; an
interrupted frame closes the session, requiring attach again. Closing the
profiler disconnects; it never terminates the target.

## Snapshot contract and limits

`OnlineXamlTreeAccess.h` is a versioned private diagnostic ABI, not a supported SDK
API. All three graph walks, node limits, properties, and snapshot assembly live
in the TAP. MUX only supplies existing roots and read-only, one-node-at-a-time
native access: immediate children, private HWCompNode parents, scalar XAML data,
ownership links, and existing backing visuals. It neither walks the graph nor
creates roots/visuals or forces layout/rendering. The TAP serializes the copied
snapshot on its worker and returns bounded, correlated frames on
a randomly named, current-user-only pipe. The profiler verifies the pipe client's
PID, process creation time, runtime path, ABI, request ID, schema, and tree topology.

```text
Profiler UI -- named pipe --> TAP worker in target
                                 |
                                 v target UI DispatcherQueue
                             TAP SnapshotWalker
                                 |
                                 v synchronous callback scope
                             MUX thin TreeAccess adapter
```

`OnlineXamlProfilerWithTreeAccess` invokes the TAP's callback synchronously on
the current UI thread. Its `TreeAccess` table supplies a `ReadNode` operation
for XAML and HWCompNode metadata plus immediate-child enumeration; the TAP walks
Composition IVisual objects itself. Native IDs, the adapter, and live COM references
must not escape this scope. Borrowed strings are copied during each node callback,
and visual references are released before returning from the access callback.
Callbacks must not pump messages, mutate objects, or throw across the ABI. Only
copied records remain for the worker. The previous full-walker runtime export is
not used or retained; redeploy both the updated TAP and adapter-enabled MUX.

- Each UI-thread capture is synchronous, but the collection of threads is **not a
  globally atomic snapshot or compositor-presented frame**. Capture QPC intervals
  and per-thread errors are included. A failed thread contributes no partial tree.
- The trees reflect existing visual content. Before a render produces composition
  peers, the composition trees can be empty. Nonvisual dependency objects,
  disconnected/cached elements, and other processes' compositor internals are not
  enumerated.
- Addresses are **capture-scoped identities**, not lifetime-stable IDs. Comparing
  equal addresses across captures does not prove object continuity or a leak.
- Direct links come from UIElement/HWCompNode/backing-visual relationships.
  A visual descendant without its own HWCompNode is associated with its nearest
  captured owning visual ancestor; its `CompLink` property says `NearestAncestor`,
  rather than claiming direct ownership. Some objects have no cross-tree link.
- Captured XAML properties include type/name, actual size, and visibility.
  Composition visuals additionally include size, offset, scale, rotation,
  opacity, and visibility. Property-read failures retain their HRESULT.
- Each capture is limited to 100,000 nodes across all threads and trees and
  64 MiB of JSON. Each target UI-thread dispatch has a 15-second timeout;
  the client bounds a request to one minute. Snapshot history is in-memory only.
- Only one tool should own a target's diagnostics session at a time; do not attach
  concurrently with Visual Studio Live Visual Tree or other TAPs.

## Opted-in live smoke test

Launch a test-owned target with the modified runtime, allow its first frame to
render, then run the test host with matching architecture. Use a target with
nested content in all three trees, such as `FolderTreeAppCpp`, rather than an
empty window:

```powershell
$env:ONLINE_XAML_TEST_PID = '<target PID>'
$env:ONLINE_XAML_TEST_TAP = '<absolute path to matching OnlineXamlProfilerTap.dll>'
dotnet test .\OnlineXamlProfiler.Tests\OnlineXamlProfiler.Tests.csproj --filter "TestCategory=Live"
```

This requests two snapshots over one connection, checks all three trees and
their nested topology and direct ownership links, and verifies the same process instance with distinct
request IDs. It never terminates or redeploys the selected target. Without those
variables it is inconclusive, not a reported successful live test.

For an interactive UI regression test, on an English desktop:

```powershell
$profiler = (Resolve-Path '.\artifacts\bin\OnlineXamlProfiler.App\x64\Debug\net9.0-windows10.0.26100.0\win-x64\OnlineXamlProfiler.App.exe').Path
.\OnlineXamlProfiler.Tests\Smoke-Ui.ps1 -ProfilerPath $profiler -TargetProcessId <PID>
# Or exercise the profiler's executable-launch button:
.\OnlineXamlProfiler.Tests\Smoke-Ui.ps1 -ProfilerPath $profiler -TargetExecutable '<absolute path to test app.exe>'
```

The UI test checks two snapshots, expanding all descendants in all three trees, a collapsed
HWCompNode-to-IVisual link, snapshot-history selection, and disconnect. It closes
only its own profiler window; it never terminates the selected target. In launch
mode it prints the newly launched PID so the caller can clean up a test-owned app.
