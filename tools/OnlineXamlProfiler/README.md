# Online XAML Profiler

A standalone tool for taking live snapshots of a desktop WinUI 3 application's
XAML visual, internal HWCompNode, and Composition IVisual trees.

This README covers building, deploying, and using the profiler. See
[Architecture](ARCHITECTURE.md) for the implementation and design.

## Build

Install Visual Studio's .NET desktop and C++ desktop workloads, the Windows
10.0.26100 SDK, and .NET 9. From this directory:

```powershell
.\build.ps1 -Platform x64 -Configuration Debug -Test
```

The script restores public SDK package dependencies and builds the native
components and unpackaged WinUI app. `-Test` also runs native TAP-walker tests and
focused managed tests. Running tests requires a host that can execute the selected
architecture.
Use `ARM64` or `x86` for those targets. Profiler, TAP, and target must have the same
architecture. Keep `OnlineXamlProfiler.Injector.exe` next to the TAP DLL. The output
is under `artifacts\bin\OnlineXamlProfiler.App`; run `OnlineXamlProfiler.App.exe`
in its output directory, with its dependencies.

Core/transport tests can also be run independently:

```powershell
dotnet test .\OnlineXamlProfiler.Tests\OnlineXamlProfiler.Tests.csproj --filter "TestCategory!=Live"
```

## Required target runtime

The application being inspected **must load a runtime built from this change**.
The tool intentionally fails with an actionable diagnostic when the runtime lacks
the new `OnlineXamlProfilerWithTreeAccess` export. It does not silently substitute
ETW reconstruction or another tree source.

Build the repository's `Microsoft.UI.Xaml.dll` normally: from the repository root,
initialize with `.\init.ps1 amd64chk`, then build through `build.cmd` or its
documented component-build workflow. Deploy the resulting runtime and its **matching**
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

**Cancel** aborts an in-progress connection or capture. If the connection is
interrupted, attach again before taking another snapshot. Closing the profiler
disconnects; it never terminates the target.

### Live process statistics

Expand **Process statistics** to see working set/private working set, private
commit, dedicated/shared GPU memory, and busiest GPU engine usage. The graphs
update approximately once a second and show the last 60 seconds. The first memory
series is solid and the second is dashed. Values are process totals, not
per-XAML-element allocation or GPU-resource attribution.

Working-set and commit peaks are OS-reported process-lifetime values. The GPU
memory peak is the highest sampled dedicated-plus-shared total **since attach**,
not a lifetime peak; polling can miss short spikes. Utilization is the busiest
engine, not the sum of engine percentages. Failed or missing samples show
**Unavailable** with diagnostics and leave graph gaps, rather than displaying
zero. Private working set can be unavailable on older Windows versions while
working set and commit remain available.

Dashed graph markers identify tree captures. Selecting an earlier snapshot shows
its frozen process readings, sample time, and time offset while the live graphs
continue. Tree and process readings are **not atomic**; a snapshot can have no
nearby process sample. Disconnect, target exit, and window close stop sampling.
Reattaching starts a new live history and observed GPU peak while preserving
earlier snapshots' readings in memory.

## Usage notes and limitations

- Each UI-thread capture is synchronous, but the collection of threads is **not a
  globally atomic snapshot or compositor-presented frame**. Capture QPC intervals
  and per-thread errors are included. A failed thread contributes no partial tree.
- Attaching can lazily create a popup root through WinUI diagnostics. The capture
  adapter itself does not create roots or force layout/rendering; see the
  [attachment caveat](ARCHITECTURE.md#attachment-and-ui-thread-discovery).
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
- Snapshot history is in-memory only. Capture size and duration are bounded; see
  [transport, consistency, and limits](ARCHITECTURE.md#transport-consistency-and-limits)
  for the limits and failure behavior.
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
HWCompNode-to-IVisual link, live statistics updates, frozen snapshot readings,
snapshot-history selection, stopping samples on disconnect, and reattach. Add
`-ScreenshotPath '<absolute path to image.png>'` to retain a profiler-window image.
It closes only its own profiler window; it never terminates the selected target. In launch
mode it prints the newly launched PID so the caller can clean up a test-owned app.
