# Online XAML Profiler architecture

The Online XAML Profiler captures live, on-demand snapshots of three related
trees in a WinUI 3 application: the XAML visual tree, the internal HWCompNode
tree, and the Composition IVisual tree. A separate host-side sampler collects
process-wide memory and GPU statistics and associates nearby readings with tree
captures.

**MUX exposes read-only native access, including the private HWCompNode parent.
The injected TAP owns all three graph walks and builds the snapshot.** This is
a private diagnostic interface, not a new public SDK API.

This tool is independent of `tools\XamlProfiler` and its TAP/helpers. Tree
capture, startup texture observations and process statistics do not depend on
ETW. An optional host-owned real-time Windows GPU trace enriches snapshots with
allocation evidence; it does not enable the MUX profiler provider or reconstruct
trees. See the [README](README.md)
for build, deployment, and usage instructions.

## Component boundaries

| Component | Responsibility |
| --- | --- |
| Profiler application | Launches or attaches to a target, requests snapshots, validates received data, and displays snapshot history, properties, cross-tree links, and process statistics. |
| Injector helper | Performs native diagnostics injection in a separate, short-lived process, using the target's loaded FrameworkUdk. It does not walk trees. |
| TAP worker in the target | Receives pipe requests, dispatches capture onto registered target UI threads, handles completion/errors, and serializes copied snapshot records. |
| TAP `SnapshotWalker` on a target UI thread | Discovers graph roots, traverses all three trees, captures properties, builds ownership links, and enforces cancellation and node budgets. |
| MUX `TreeAccess` adapter | Supplies existing XAML roots and one-node-at-a-time access to private XAML/HWCompNode data. It does not build a snapshot or recursively walk the graphs. |
| MUX surface-observation cache | Retains bounded copied metadata from normal surface lifetime/drawing operations, independently of attachment or ETW. |
| Optional host GPU trace and ledger | Collects Windows resource/allocation evidence and performs bounded, process-aware temporal joins with frozen native draw observations. |
| Host process-statistics reader | Reads process memory and PID-scoped GPU counters through Windows APIs, independently of TAP and MUX tree capture. |
| Host statistics history and presentation | Retains a rolling sample window, freezes nearby readings with snapshots, and displays live graphs and capture markers. |

```text
Profiler process                         Target application process

Profiler application -- named pipe --> TAP worker
                                           |
                                           | DispatcherQueue.TryEnqueue
                                           v
                                       Target UI thread
                                       TAP CaptureSnapshot
                                           |
                                           | OnlineXamlProfilerWithTreeAccess
                                           v
                                       MUX creates TreeAccess
                                           |
                                           | synchronous callback into TAP
                                           v
                                       TAP SnapshotWalker
                                           |
                                           +-- ReadNode --> MUX one-node access
                                           |
                                           +-- Composition Visual APIs
                                           |
                                           v
                                       Copied snapshot records
                                           |
Profiler application <-- named pipe -- TAP worker serializes result
```

The walk executes inside the target process, on the target's UI thread. Neither
the profiler application's process nor the TAP worker dereferences live XAML or
composition objects to perform the walk.

## Attachment and UI-thread discovery

The profiler performs native diagnostics injection in an owned, short-lived
helper process, using the FrameworkUdk DLL already loaded by the target. This
avoids mixing different SDK diagnostic payload layouts or loading the target's
SDK into the profiler UI process.

In `Tap::SetSite`, TAP obtains `IXamlDiagnostics` and `IVisualTreeService`,
registers the current UI thread's `DispatcherQueue`, and reads the pipe name from
`IXamlDiagnostics::GetInitializationData`. Its worker connects to that pipe,
resolves the MUX tree-access export, and subscribes through
`IVisualTreeService::AdviseVisualTreeChange`. Each `OnVisualTreeChange` callback
only registers the executing UI thread and its dispatcher. TAP discards the
diagnostics parent/child payload; the captured hierarchy comes from
`SnapshotWalker`, not from those notifications. The worker unsubscribes during
session teardown.

**The read-only adapter contract is not a guarantee that attachment has no
side effects.** The diagnostics subscription performs an initial tree enumeration;
its child enumeration can call `GetPopupRoot`, which can lazily create a popup
root. The MUX `TreeAccess` adapter itself only returns existing roots and objects
and does not force layout or rendering.

## The private MUX access contract

`OnlineXamlTreeAccess.h` defines a versioned function-table ABI. The TAP resolves
the `OnlineXamlProfilerWithTreeAccess` export from the target's
`Microsoft.UI.Xaml.dll`. That export validates the ABI and current XAML core,
collects existing roots from the current core's content roots, and synchronously
invokes the TAP's callback with a `TreeAccess` table.

The table supplies:

- The owning thread ID and existing XAML root IDs.
- A context valid for the current access scope.
- `ReadNode(context, tree, id, reader)` for XAML and HWCompNode nodes.
- `ReadMemory(context, memory)` for capability-qualified XAML heap counters and
  observation-cache coverage.
- `ReadSurface(context, compositionSurfaceId, surface)` for a copied live-owner
  surface record and its last texture observation.

The current private ABI and snapshot JSON schema are both version 2. The host
rejects mismatched versions rather than interpreting a truncated record.

`ReadNode` invokes `NodeReader.Node` with one node's metadata. If requested, it
then invokes `NodeReader.Child` for that node's immediate children, preserving
their indices. Enumerating a node's immediate children is the adapter's only
child iteration; choosing the next node and traversing the graph belong to TAP.

For XAML nodes, the metadata includes type, name, actual size, visibility, and
the existing composition-peer ID. For HWCompNodes, it includes type, private
parent ID, the owning XAML element when available, and existing backing/main-tree
visuals.

### How parent access works

There is no separate public `GetParent` export. In the HWCompNode branch of
MUX's `ReadNode`, `NativeNode.ParentId` is populated using:

```cpp
node.ParentId = Address(
    do_pointer_cast<HWCompNode>(
        comp->GetParentInternal(false /* publicOnly */)));
```

This exposes the internal HWCompNode parent relationship through the private
record. TAP uses it to climb from a discovered composition peer toward its
HWCompNode root before walking downward.

The other two trees do not require this private parent accessor:

- XAML snapshot parent IDs are recorded by TAP as it visits each node's children.
- Composition visuals already expose `Visual.Parent()`, which TAP calls directly.

MUX returns existing objects only. The adapter does not create roots, composition
peers, or visuals, and does not force layout or rendering. This applies to tree
access, not to the separate diagnostics attachment described above.

## The three TAP traversal passes

`SnapshotWalker::Capture` runs the passes in order. Each pass uses an explicit
pending-node stack and a visited set; parent walks also detect cycles and enforce
bounds. TAP owns the output records and sibling indices.

### 1. XAML: `CaptureXaml`

TAP starts with the XAML roots in `TreeAccess`. For each node it:

1. Calls `ReadNode` for scalar metadata and immediate children.
2. Copies the properties and records the parent and child index from its traversal.
3. Collects any existing composition-peer ID as a seed for the next pass.
4. Adds the children to its traversal stack.

The result is a XAML tree plus a set of entry points into the internal composition
graph. Not every XAML element has a composition peer.

### 2. HWCompNode: `CaptureComposition`

A composition peer discovered from XAML can be an interior node. For each seed,
TAP repeatedly reads `ParentId`, without requesting children, until it reaches
an HWCompNode with no HWCompNode parent. It deduplicates those roots, then walks
downward using the adapter's immediate-child enumeration.

During this pass, TAP records HWCompNode-to-XAML ownership and collects both
backing visuals and main-tree visuals as seeds for the visual pass. Root discovery,
deduplication, traversal order, and snapshot construction are all TAP code.

### 3. IVisual: `CaptureVisuals`

TAP climbs from each visual seed using `Visual.Parent()`, deduplicates the visual
roots, and walks downward through `ContainerVisual.Children()`. It reads visual
properties through Composition APIs, including size, offset, scale, rotation,
opacity, and visibility.

This pass does not call MUX's `ReadNode` for visual nodes. MUX only supplies the
existing visuals that bridge the internal HWCompNode graph to the Composition
visual graph.

## Hybrid memory and GPU observations

### Read-only native measurements

XAML node records include `CoreAllocationStatus` and `CoreAllocationBytes`.
Profiler-enabled builds use the runtime's allocation-size helper, including its
debug-allocation-header handling; this is not `sizeof(CUIElement)`. A compiled-out
or unavailable measurement reports an error status. These bytes exclude peer
objects, sparse/auxiliary storage and referenced resources; subtree and by-type
totals are totals of the measured core objects only.

`ReadMemory` copies process-wide XAML allocation counters and their availability
flag, heap identity/private-heap flag, cache capacity/count, dropped
registrations and fixed metadata bytes. Compiled-out counters are `E_NOTIMPL`,
not zero. Counter fields are independent atomic reads, not an atomic heap dump.
Multiple UI-thread records can refer to the same process heap and must not be
summed. The host's independent RAM/commit counters remain process totals.

On the TAP worker, DXGI adapter enumeration and `IDXGIAdapter3::QueryVideoMemoryInfo`
read target-process usage, budget, reservation and available-for-reservation for
local/nonlocal segments. No D3D device is created. Factory/query errors remain
explicit, and this query has its own QPC interval after the UI-thread tree
captures.

### Startup-retained surface cache

`SurfaceObservationCache` has 8,192 fixed open-addressed slots, an SRW lock and
no heap allocations, COM references or static-destruction dependency. It is
compiled into profiler-enabled MUX and begins recording with ordinary surface
creation, not with profiler attachment. Reading a record never consumes it.
Overflow drops new registrations rather than evicting known live surfaces;
cumulative drops and the resulting incomplete coverage are exposed to the host.

`DCompSurface` registers copied metadata after its WinRT wrapper is initialized.
The key is the `ICompositionSurface` ABI identity; the value includes the native
owner identity, a monotonically increasing generation, dimensions, actual
creation format, virtual flag and logical size estimate. Reinitialization and
resize start a new generation and clear texture evidence. Release/device-loss
handling and destruction remove the owner-checked record; stale owners cannot
overwrite or remove a reused identity.

During normal draw/upload operations, the existing texture-observation hook
copies the `ID3D11Resource` identity, texture description, draw offset and QPC
even when MUX ETW is disabled. Temporary QI references are released immediately;
neither the cache nor a snapshot keeps a texture alive. Failed observation
queries retain their HRESULT. Snapshotting does not call BeginDraw, force
rendering/layout, or recover an unobserved historical texture by guessing.

**Coverage is deliberately live-owner-only.** Composition can retain a surface
wrapper after its `DCompSurface` owner is destroyed. Those surfaces, app/custom
surfaces, never-drawn textures and dropped registrations are unavailable rather
than attributed through a potentially recycled pointer. A retained observation
describes a previous draw, not proof of current physical backing or residency.
Logical estimates include virtual extent, exclude allocation overhead and must
not be added as independent atlas VRAM allocations.

### Current brush ownership and frozen records

The TAP reads each current SpriteVisual brush using public Composition APIs.
It follows surface, mask source/mask and nine-grid source brushes with a
64-brush-per-visual bound, then queries MUX by the current surface ABI identity.
Solid-color brushes have no surface; unsupported brush graphs and getter
failures have a per-node `SurfaceTraversalStatus`. Surface references are bounded
to 100,000 across the document; a failed UI thread contributes neither a partial
tree nor partial surface list.

Only copied metadata crosses the pipe. Integer byte counts, generations and
QPC values are serialized as decimal strings to avoid JSON-double precision
loss; pointer identities remain canonical uppercase 16-digit hexadecimal.
The host validates required schema fields, statuses, identities, visual
references and observation times before building frozen summaries. Shared logical
surfaces are deduplicated by owner/generation, and allocation evidence uses its
own identities rather than summing a texture per visual.

The statistics/memory scroller, trees and property inspector share flexible
height, with caps on the scroller and inspector. Expanding memory readings must
not collapse the tree viewports at higher display scaling.

### Optional Windows trace and temporal joins

`GpuObservationSession` owns a uniquely named real-time TraceEvent session.
Tracing is off by default, starts before executable/package activation when
requested, and is disposed on failed connection, disconnect, exit or close.
The MUX profiler provider is never enabled. The installed OS event contracts
used are Direct3D11 Texture2D creation (Resources, event 12), DxgKrnl
AdapterAllocation/DeviceAllocation (Resource, events 33/36), and DXGI Present
calls (Events, event 42). Payload fields are checked rather than presumed
present for an unsupported schema.

Process-specific resource/Present records are filtered to the target once its
PID is known. Before launch returns a PID, a bounded buffer retains candidate
records. DxgKrnl allocation records are collected trace-wide because a reachable
allocation can belong to another process; the host follows only chains reached
from the selected target. It preserves owner PIDs without guessing that every
non-app allocation belongs to DWM.

The 32,768-record ledger joins:

```text
native draw: ID3D11Resource + observation QPC
    -> latest preceding D3D11 resource creation for that PID
    -> (PID, hKMResource) / (PID, hThunkResource)
    -> hVidMmGlobalAlloc
    -> preceding adapter allocation size and owner PID
```

Resource-address versions and event timestamps prevent a later creation from
retrofitting an older draw. Conflicting allocation histories for a reused kernel
handle remain ambiguous; no exact-byte attribution is claimed for them. Matched
bytes are deduplicated by allocation identity and allocation-observation time.
Start/Stop opcodes are not interpreted as destruction, and the ledger is not a
live allocation/residency inventory.

Every frozen result retains startup-versus-late-attach coverage, trace state,
errors, OS event loss, bounded-history drops, invalid payload counts and timing.
Flush requests buffer delivery but is **not** a consumer-drain or cross-provider
atomicity guarantee. Late delivery or missing startup events can leave a chain
unmatched. A matched allocation size is historical evidence, not current VRAM.
Present calls are API-call observations, not GPU execution time; the existing
counter-based busiest-engine graph remains the activity view.

## Threading and object lifetime

Each target UI-thread capture has one synchronous access scope:

1. The TAP worker enqueues `CaptureSnapshot` on that thread's `DispatcherQueue`.
2. `CaptureSnapshot` calls `OnlineXamlProfilerWithTreeAccess`.
3. MUX calls TAP's `Walk` callback with its temporary access table.
4. TAP completes all three passes before returning from that callback.
5. The UI-thread callback signals completion; the worker serializes the copied data.

The access table and live native objects must not be used outside this scope.
Strings and COM pointers delivered to `NodeReader.Node` are borrowed during that
node callback. TAP copies strings immediately and acquires its own references to
visuals needed later in the same access scope. Those visual references are released
before the access callback returns.

Only copied records remain for the worker. Address-derived IDs can be retained
as snapshot identity values, not as handles that another thread or the profiler
may dereference. Equal addresses across separate snapshots do not establish
object continuity.

Access callbacks must not pump messages or mutate live objects. Exceptions must
not cross the private ABI; TAP callback boundaries translate failures to HRESULTs.

## Cross-tree links

The three trees remain separate graphs. TAP records links between them using:

- The XAML element's existing composition peer.
- The HWCompNode's XAML owner.
- The HWCompNode's backing/main-tree visuals.

A visual without its own direct HWCompNode association can inherit the nearest
captured ancestor's ownership. Its `CompLink` property distinguishes `Direct`,
`NearestAncestor`, and `None`; an inherited link is not presented as direct
ownership. Some nodes legitimately have no corresponding node in another tree.

## Transport, consistency, and limits

The TAP serializes copied records on its worker and returns correlated frames
over a randomly named, current-user-only pipe. The profiler verifies the pipe
client's PID, process creation time, runtime path, ABI, request ID, schema, and
tree topology. Snapshot transport, validation, and plain-data tree construction
run off the profiler UI thread. Native TreeView nodes are published in cancellable
UI-thread batches; their complete parent graph lets cross-tree links select
previously collapsed descendants.

Capture is synchronous per target UI thread, but captures across threads are not
globally atomic and do not represent a compositor-presented frame. Per-thread
QPC intervals and status describe the capture. A failed thread contributes an
error rather than a partial tree. Property-read failures retain their HRESULT.

The tool observes existing visual content. Before rendering creates composition
peers, the composition trees may be empty. It does not enumerate nonvisual
dependency objects, disconnected/cached elements, or another process's compositor
internals. Each capture is limited to 100,000 nodes across all threads and trees
and 64 MiB of JSON. Each target UI-thread dispatch has a 15-second timeout; the
client bounds a request to one minute. Snapshot history is in-memory only.

Cancel stops the owned injector and aborts connection/capture. An interrupted
frame closes the session rather than attempting to resume a partially transferred
message, so another attachment is required. Disconnect and profiler shutdown
close the connection without terminating the target; captured snapshots remain
available in the profiler until it closes.

The target must load an adapter-enabled MUX runtime. A missing export or incompatible
ABI produces an explicit error; the TAP does not fall back to ETW reconstruction
or the previous full-walker runtime export.

## Process statistics

Process statistics is a separate data path in the profiler process. It uses no
ETW session or providers and adds no TAP/runtime instrumentation:

```text
Profiler host background task
    |
    +-- ProcessStatisticsReader -- process handle --> Windows memory APIs
    |
    +-- ProcessStatisticsReader -- PID-scoped PDH --> GPU counters
    |
    v UI DispatcherQueue
ProcessStatisticsHistory --> live ProcessStatisticsPanel graphs
    |
    +-- nearest sample to capture midpoint --> CapturedSnapshot frozen readings
```

### Collection and counter semantics

After attachment, `MainViewModel` starts a background task with a
`ProcessStatisticsReader` and a one-second `PeriodicTimer`. Each copied sample
includes UTC time, a monotonic `Stopwatch` timestamp, process identity, values,
and per-counter diagnostics. The UI dispatcher adds samples to history and
refreshes the panel; queued updates are ignored after cancellation or when their
session is no longer current.

The reader opens and retains a process handle and verifies its creation time
against the connected session before collecting. It checks that handle for exit
both before and after collection, discarding values if the target exits during
the read. This prevents PID-scoped GPU counters from being attributed to a reused
PID.

`GetProcessMemoryInfo` supplies working set, private working set, private commit,
and OS-reported process-lifetime working-set/commit peaks. Where the extended
private-working-set field is unsupported, the reader uses the older structure
size for the other memory values and records private working set as unavailable.

GPU collection uses PDH English counter paths scoped to `pid_<PID>_*` instances:

- `GPU Process Memory` dedicated/shared usage is summed across matching adapter
  instances. Dedicated and shared readings must describe the same instance set.
- `GPU Engine` utilization is the highest valid engine percentage, capped at
  100%, not the sum of engine percentages. The engine instance is retained.
- The observed GPU-memory peak is the highest valid sampled dedicated-plus-shared
  total since attachment, not an OS process-lifetime peak.

The reader validates counter status, instance identity, numeric ranges, and
availability. Missing counters, initial engine-rate warm-up, failed reads, and
invalid values retain explicit diagnostics rather than becoming zeroes. Memory
and GPU results can be unavailable independently. These are process totals, not
per-element allocation or resource attribution, and polling can miss short spikes.

### History, snapshot association, and lifetime

`ProcessStatisticsHistory` accepts only the attached PID/creation-time pair and
strictly increasing monotonic timestamps. It retains the last 60 seconds of
samples; its observed GPU peak survives eviction from that rolling window.

`MainViewModel` measures each tree request's start and completion and assigns it
their monotonic midpoint. Once the capture completes, it selects the nearest
available sample from the matching process instance within two seconds of that
midpoint. The `CapturedSnapshot` retains that immutable sample, its diagnostics,
and the observed GPU peak at association time. If no sample qualifies, the
snapshot explicitly has no process reading; it is not filled later.

`ProcessStatisticsPanel` continues drawing live history while displaying the
selected snapshot's frozen readings, sample time, and offset from its capture
midpoint. Capture markers are drawn only for the current process instance within
the visible history. The first memory series is solid and the second is dashed.
Unavailable values and sample intervals longer than 2.5 seconds break graph
lines instead of bridging gaps. Tree and process measurements are not atomic.

Disconnect, target exit, and window close stop sampling and release the reader's
process/PDH handles. Reattachment creates a new history and observed GPU peak.
Earlier snapshots retain their readings independently of the live history.

## Source map

Paths are relative to the repository root.

| File | Architecture responsibility |
| --- | --- |
| `dxaml\xcp\components\base\inc\OnlineXamlTreeAccess.h` | Private ABI, node records, reader callbacks, and lifetime contract. |
| `dxaml\xcp\dxaml\lib\OnlineXamlTreeAccess.cpp` | MUX export, existing-root collection, and one-node native adapter including private parent access. |
| `tools\OnlineXamlProfiler\OnlineXamlProfilerTap\SnapshotWalker.h` | Copied snapshot record types and capture entry point. |
| `tools\OnlineXamlProfiler\OnlineXamlProfilerTap\SnapshotWalker.cpp` | All three graph walks, properties, ownership links, cancellation, and node budgets. |
| `tools\OnlineXamlProfiler\OnlineXamlProfilerTap\Tap.cpp` | Diagnostics bootstrap and UI-thread discovery, export resolution, capture dispatch, worker serialization, and pipe request handling. |
| `tools\OnlineXamlProfiler\OnlineXamlProfiler.Windows\ProfilerSession.cs` | Profiler-side connection, request handling, and snapshot receipt. |
| `tools\OnlineXamlProfiler\OnlineXamlProfiler.Windows\ProcessStatisticsReader.cs` | Pinned process identity, OS memory reads, PDH GPU counters, and explicit counter diagnostics. |
| `tools\OnlineXamlProfiler\OnlineXamlProfiler.Core\ProcessStatistics.cs` | Immutable process-statistics records, rolling history, observed GPU peak, and nearest-sample selection. |
| `tools\OnlineXamlProfiler\OnlineXamlProfiler.App\MainViewModel.cs` | Connection/sampling lifetime, snapshot history, capture midpoint, and frozen statistics association. |
| `tools\OnlineXamlProfiler\OnlineXamlProfiler.App\ProcessStatisticsPanel.xaml` and `.xaml.cs` | Statistics presentation, live graphs, capture markers, frozen readings, and unavailable-value diagnostics. |
