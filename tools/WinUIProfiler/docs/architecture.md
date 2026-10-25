# WinUI Profiler foundation and in-progress runtime/metric integration

## Scope and build

This is a new, independent profiler. P01 contains portable `net10.0` contracts,
bounded event/protocol codecs, validation, and deterministic synthetic tests.
P02 adds transactional SQLite persistence and bounded, read-only typed queries.
P03 adds real live/offline ETW ingestion, typed native decoding, bounded
dedicated processing/writer threads and durable diagnostic/stop control.
P04 supplies the native typed writers/private protocol foundation.
P05 adds selected framework observation hooks and actual local-runtime tests;
P07 adds native metric sources, managed CPU/memory/GPU interop and independent
CPU/memory/GPU polling, typed collector Process/Metric/Loss emission/decoding,
an owned live metric recording coordinator and persisted live/offline queries.
P06 has guarded allocator foundations and actual heap-boundary hooks. Matched
x64 checked/fre hosts qualify actual allocator ETW transitions and a full
disable/re-enable gap. Graphics resource coverage, loss accounting acceptance,
remaining allocator runtime scenarios and overhead remain unfinished.
These milestones remain incomplete. `WinUIProfiler.App` supplies a real
saved-session/metric inspection UI, not the full exploration or capture UI.
P08 now includes architecture-matched native injection, a COM TAP, authenticated
named-pipe transport and managed client. Runtime capture/rundown is still
unsupported; this is not a complete attach/baseline implementation. P09-P11
projections now form the default five-processor ordinary pipeline: ObjectState,
Frame (composing metrics), XamlTree, CompositionTree and ResourceUsage.
An explicitly supplied empty processor list retains input-only recording.
The bounded `BaselineCoordinator` validates capture assembly and domain receipts;
delta reconciliation and atomic owner publication are not yet integrated.
`SessionWriter.CommitBaseline` supplies bounded, paged single-transaction
storage beyond the ordinary batch limit, preserving original End provenance and
clock metadata transactionally. Owner-affine immutable aggregate page production
is also available; reconciliation, all-five End routing and recording integration
remain separate gates.
`WinUIProfiler.Cli` supplies the bounded P16 read-only saved-session query
foundation, with typed routing, bounded JSON and focused subprocess tests;
see [CLI contract and limitations](cli.md). P12/P13/P14 and end-to-end data
producers remain open; this does not complete P16 or the overall plan.
Automatic runtime capability
coverage and capture traversal remain unfinished. See [TAP and injection](tap.md), [ingestion](ingestion.md),
[event coverage](event-coverage.md) and [metrics](metrics.md).
No other profiler is a source, project, package, binary or design dependency.

The complete suite includes Windows ETW integration tests, which require
permission to create/enable the uniquely owned sessions and an x64 Release
native fixture harness. Build that prerequisite from the repository root
using the command in [ingestion verification](ingestion.md#verification-and-intentionally-absent-work);
permission or missing-harness failures are not silently skipped.

Then, from `tools\WinUIProfiler`:

```powershell
dotnet restore WinUIProfiler.Engine.Tests\WinUIProfiler.Engine.Tests.csproj --locked-mode
dotnet test WinUIProfiler.Engine.Tests\WinUIProfiler.Engine.Tests.csproj -c Release --no-restore
```

The solution includes the C# WinUI app and a real C++ DLL and requires **Visual Studio MSBuild**.
`dotnet build` or `dotnet restore` of the mixed solution cannot evaluate the
Visual C++ defaults; building only managed projects does not validate native
integration. The build entry below evaluates the matching local WinUI Product
through the repository's build environment, selects an installed native toolset,
and builds the mixed solution using full Visual Studio MSBuild:

```powershell
.\scripts\Build.ps1 -Configuration Release -Architecture x64
```

`Build.ps1` always selects the full mixed solution, including the native metric,
TAP and injector projects. It reuses `Build-App.ps1` for evaluated Product
discovery, native toolset selection, self-contained staging and deployment
verification rather than maintaining a second build path. Add `-Restore` only
when dependency manifests changed or a build reports missing restored assets.
Both entry points propagate build/deployment failures. The wrapper's parameter,
solution-selection and failure behavior can be checked without rebuilding:

```powershell
.\scripts\Tests\Test-BuildEntryPoint.ps1
```

The app is unpackaged, Windows App SDK self-contained and .NET self-contained.
Its deployment manifest records the verified matched local runtime payloads;
an incomplete runtime/resource Product is a build error, not an SDK fallback.
See [application build and verification](app.md) for the satellite prerequisites,
launch command, actual UI smoke test and intentionally absent functionality.

For actual native interop tests, build both architecture DLLs: the wrong-
architecture rejection test deliberately needs the other artifact and does not
silently skip it. The native scripts discover the installed compiler toolset.
From `tools\WinUIProfiler`:

```powershell
.\WinUIProfiler.Native\Build-NativeDll.ps1 -Architecture x64
.\WinUIProfiler.Native\Build-NativeDll.ps1 -Architecture ARM64
dotnet restore WinUIProfiler.IntegrationTests\WinUIProfiler.IntegrationTests.csproj --locked-mode
dotnet test WinUIProfiler.IntegrationTests\WinUIProfiler.IntegrationTests.csproj -c Release --no-restore
```

Interop tests choose the DLL matching the actual .NET process architecture,
load it by an explicit local absolute path with constrained dependency search,
and invoke real exports. ARM64 cross-compilation on x64 is not ARM64 execution.
The native sampler tests can run independently using
`WinUIProfiler.Native.Tests\Run-NativeTests.ps1` and
`WinUIProfiler.Native.Tests\Run-NativeDllTests.ps1`; these do not require product
ETW sessions. The complete Engine suite does use ETW: run it separately from
framework lifecycle/instrumentation sessions to preserve enablement isolation.
The IntegrationTests suite also creates real collector ETW sessions and needs
the same session permissions and isolation.

`NativeMetricSession` retains a selected-target handle with query-limited and
synchronize rights and polls CPU/memory/GPU on `MetricPollingWorker`, not a UI or
ETW-decoding thread. Each batch retains native boot-clock acquisition/sample
intervals, GPU's original PDH FILETIME timestamps, and a separate actual QPC
bracket around the three sequential calls;
the bracket is not an atomic snapshot or an invented boot-to-QPC conversion.
The configured delay is between acquisitions, so consumers must use actual
recorded intervals rather than assuming an exact timer period.

Publication uses a nonblocking admission callback intended for bounded ingress.
A rejected sample faults polling instead of silently dropping it. A missing
target or changed identity publishes its final missing batch and then stops.
Callers must observe `Completion`; acquisition, publication and native ABI
errors propagate through it. Stop/disposal waits for any in-flight call before
closing native modules and the target handle. No new capture starts after
stopping. Its typed terminal status is set only after the final batch is accepted;
identity mismatch and authoritative target exit remain distinct.
A GPU capture with no
selected-process engine rows remains unavailable, never measured zero. The GPU
normalizer reports the busiest measured engine, not the sum of engine percentages.
GPU-engine source version 1 leaves its process-wide committed/resident memory
unavailable. The optional independent DXCore source supplies configured physical
adapter gauges without inventing an aggregate.
The current host exposes only Basic Render Driver adapters: interop and missing
data behavior are tested, but hardware GPU qualification is not claimed.

`NativeMetricPayloads` maps an acquired batch into distinct selected-process CPU,
system/interrupt/other-or-unattributed CPU aggregates, explicitly unavailable CPU
utility, three separate memory gauges, busiest-engine and per-engine GPU
utilization, and four independently unavailable process-wide GPU memory measures.
An immutable explicit physical-adapter selection, or a fresh graphics-visible
DXGI inventory when selection is null, adds four independent DXCore
committed/resident dedicated/shared gauges per scope, including honest missing
observations. These queries share polling ownership and target termination checks;
they do not derive adapter coverage from active engine rows. New CPU
metric identifiers are appended, preserving existing numeric identifiers.
The GPU winning physical adapter/engine is retained; engine class stays unknown
unless the source supplies authoritative classification.
Inventory partial/error/software/selection coverage and its compute/NPU limits
are retained in the collector Attach quality reason; individual known-scope
gauges do not claim complete global inventory or process-wide byte aggregation.

Metric event/query records now have an **optional additive**
`SourceObservation`. Missing metadata in older/synthetic records stays null,
not a guessed clock mapping. It retains the original boot or UTC FILETIME
clock, explicit 10 MHz units, native interval/status, separate monotonic
boot-clock acquisition bracket and native error domains/PDH error. A reversed
FILETIME interval is allowed only for an explicitly recorded counter reset.
Nonmeasured source statuses cannot accompany numeric values. `MetricPayload.Interval`
is the independently recorded session-QPC acquisition bracket, not a conversion
of that native observation interval.

The stateless `MetricProcessor` runs on the common bounded owner and persists
these records through the existing single SQLite writer and typed query path.
The optional field survives envelope serialization, committed projection JSON
and read-only reopen; this requires no reinterpretation of fixed runtime bytes
or database-column migration. Actual native polling through typed ingress,
processing, SQLite and reopen is tested independently of the transport.

`CollectorMetricEmitter` additionally writes the fixed collector provider's
schema-1 Metric event as forty flat typed fields, not JSON. Its explicit static
initialization registers the provider before constructing an emitter. Owned
live sessions and the ETL conformance capture explicitly set a bounded 10-second
synchronous provider-enable timeout: TraceEvent 3.2.8's observed default is zero
(asynchronous), despite its XML documentation describing a 10-second default.
Provider enablement must complete before the first metric is published.
The decoder routes by payload session and canonical target identity, not the
collector's header PID, and preserves producer/header provenance separately.
See [the wire contract](event-contract.md#collector-metric-wire-schema-1).

Real ETL replay and simultaneous live-session tests verify isolation, clock
preservation and committed queries. An additional integration test acquires
actual CPU/memory/GPU samples through the native DLL, emits collector ETW and
persists/reopens the results through the common owner and SQLite writer.
Only the exact known Windows PartitionInfoExtensionV2 transport descriptor is
ignored; neighboring unknown descriptors remain diagnostic observations.
Collector Process and Loss now have separate strict typed schema-1 writers and
decoders using the same session/target/epoch and serialized sequence. Real ETL
conformance verifies all three collector events, loss-domain reconstruction
and durable ProducerLoss health after reopening.
These tests retain MissingMetadata health rather than claiming a complete
runtime baseline.

`LiveMetricRecording` composes these components under one acquisition lease.
`Start` is one-shot; `Started` completes only after synchronous provider
enablement, Attach publication and the first accepted native metric batch.
Publication is not a delivery acknowledgment or a durable commit barrier.
`Completion` returns the terminal processing result. User stop, caller
cancellation and worker stop all quiesce polling and its in-flight call before
stopping/draining ETW. Only then does the coordinator seal ingress and release
the lease, allowing terminal health persistence. The internal component mode
does not acquire a second lease, complete the worker or stop ETW directly on
worker cancellation. Statistics-query failure signals the coordinator rather
than stopping still-usable transport; native calls finish and Loss can be
published before ETW closes. Standalone `LiveEtwSource` behavior is unchanged.
An ingress limit can seal the bounded queue earlier; the acquisition lease
still prevents premature health finalization while components quiesce.

Native/transport failures become durable SourceFailure health and Failed state.
If transport is still available, acquisition failure also emits an explicit
Metric-domain Loss with unknown lost-event count. Accepted native TargetMissing
is a normal authoritative exit: the final missing metrics and Process Exit are
drained without fabricating failure. IdentityChanged remains failure, never Exit.
No process-creation QPC is inferred from FILETIME. Stop/disposal before Start and
precanceled Start open neither native modules nor transport. Eight actual
integration cases initially covered normal drain/reopen, startup failure, identity mismatch,
owned child exit, stop-before-start, precancellation, live cancellation and
processing failure. Three additional cases verify a statistics-query failure
with native publication held in flight, explicit ingress-byte-limit shutdown,
and concurrent stop/disposal. The control-failure ordering correction has
narrow independent review approval. Complete producer capability/baseline reconciliation,
runtime control/injection and hardware/parity qualification remain unfinished.

The local `global.json` pins installed .NET LTS SDK **10.0.402**. Build
props/targets terminate discovery without importing product build machinery.
Always run commands from this directory: an SDK selection made from the
repository root is outside this boundary. Local NuGet configuration clears
inherited sources/mappings and uses Microsoft's **public** `dotnet-public`
NuGet mirror; packages are isolated in `.packages`, not product caches.
`RestorePackagesPath` explicitly overrides an inherited `NUGET_PACKAGES`
environment variable as well as the config's global-packages folder.
The mirror was chosen because direct `api.nuget.org` TLS negotiation failed
on the development host, not because any private packages are needed.

Direct test dependencies are exact central pins: `Microsoft.NET.Test.Sdk`
**17.14.1**, `xunit` **2.9.3**, and `xunit.runner.visualstudio` **3.1.4**.
Public metadata:
[test SDK](https://www.nuget.org/packages/Microsoft.NET.Test.Sdk/17.14.1),
[xUnit](https://www.nuget.org/packages/xunit/2.9.3),
[adapter](https://www.nuget.org/packages/xunit.runner.visualstudio/3.1.4).
The adapter supports .NET 8+ and xUnit v2. Package restore, compilation and
execution check compatibility with .NET 10. Committed lock files pin
transitives and content hashes. Contracts have **no NuGet package dependencies**; JSON and binary primitives
come from the platform BCL. The engine directly pins `Microsoft.Data.Sqlite`
**10.0.13**; its engine/test lock files retain the SQLitePCLRaw **2.1.13**
native bundle and transitives. See [storage](storage.md) for the P02 lifecycle,
schema, transaction, temporal query, failure and continuation contracts.
P03 pins `Microsoft.Diagnostics.Tracing.TraceEvent` **3.2.8** in this same
independent manifest and both engine/test lock files.

## Planned product boundaries, not implementations

* A C# WinUI 3 UI and a typed CLI will share the UI-independent engine.
* Hosts/targets: Windows 11 build 22000+, x64 and ARM64. SDK 26100 API
  declarations can support capability-gated GPU collectors without raising the
  minimum OS version. Native work later uses VS18; there is no native project
  in P01.
* Each session selects exactly one `ProcessInstanceKey`; launch of packaged
  and unpackaged apps and attach are separate live modes. Offline ETL is
  read-only with respect to targets.
* New fre runtime telemetry and OS collectors feed the same canonical
  `EventEnvelope` / `ProducerStreamKey` normalization boundary for live/offline
  input. Backend ETW adapters will decode typed wire fields into the same
  canonical payloads. The JSON codec here is **not an ETL reader**.
* A dedicated bounded processing thread will exclusively own object history,
  frame changes, XAML/composition topology, many-to-many correlations and
  resource/backing projections. UI code will not mutate these states.
* A single SQLite writer uses WAL, temporal rows and atomic committed
  watermarks. Queries will see only committed state, never half a baseline.
  P02 persists explicit changes supplied by future processors; it does not
  manufacture projections from source timestamps or report successful
  unsupported operations.
* A native C++/WinRT TAP will be injected through `IXamlDiagnostics`. ETW is
  the observation channel; an authenticated named pipe carries bounded target
  commands into a new private runtime bridge. P08 owns transport/injection.

## Identity, time and quality invariants

`ProcessInstanceKey` includes PID, nullable creation FILETIME and a stable
trace-local instance GUID. Unknown birth is **null**, not zero. PID-only joins
are prohibited. Reuse or loss that makes identity ambiguous creates a new
instance GUID, even if PID is unchanged. FILETIME is UTC 100-nanosecond units
since 1601; QPC timestamps use the **session's recorded frequency**, never
the analysis machine's `Stopwatch.Frequency`.

`ObjectKey` is target process + domain + producer epoch + explicit lifetime ID.
The five domains are XAML core, XAML peer, CompNode, canonical composition
IVisual and resource. No pointer-based identity. Addresses are nullable
dump/correlation metadata only; managed compaction and address reuse cannot
establish survival. Canonical IVisual means one identity for the same COM
identity across interface aliases, not one ID per wrapper or visual role.
Backing allocations have distinct GUIDs and device epochs so shared resources
are not counted repeatedly.

`ProducerStreamKey` identifies provider + actual source PID + producer epoch.
Collector source PID differs from the semantic target PID. Future ETW adapters
must recover an authoritative epoch from the wire prefix/capabilities, not
guess it from an address or neighboring event. `EventCodec.Normalize` validates
the binding and matching object/type/capability/loss epochs. An epoch transition
or loss invalidates unproven continuity until an explicit baseline; it does not
merge uncertain object/process lifetimes.

QPC intervals are half-open `[StartQpc, EndQpc)`; zero duration represents a
point. Ingest ordinals start at one, are session-wide and deterministic even
when source timestamps tie or arrive late. Source sequences start at one
within a delivery-homogeneous `SequenceDomainKey`: producer stream + target
process instance + event ID/version (fixed keyword/level). They may be null
when genuinely unavailable and are not the committed watermark or a
cross-domain causal clock. Merge/query order uses QPC then ingest ordinal.
Per-listener domain coverage and continuity IDs distinguish filtering from
real missing observations; another listener changing its selection does not
reset existing continuity. Enable continuity or increasing QPC does not prove
that lower reserved emissions or deliveries have finished. Producer cuts
separate largest reserved sequence from the contiguous settled-emission
prefix. Positive receipt and finalized absence have different requirements.
A covering cut plus all distinct expected IDs with bound live eligibility and
completed relevant processing proves that bounded span received, including
reordered 4,7,5,6, without stopping recording or sealing an artifact.
`SequenceReceiptEvidence` carries listener/continuity/cut binding, enabled
sequence bounds and a contiguous processed ordinal independently of closure.
Pending IDs/processing remain Unproven, not an invented missing count.
Honest empty spans need no fake observations or archive. To finalize absent
IDs requires the stronger source-finalized-after-cut, exhaustive enumeration
and completed decode proof; ordinary live ETW, capture End, timeout or
FlushTrace alone cannot supply it. Only a genuinely closed window still
missing 5/6 proves two missing observations. A live baseline can reconcile
once all required settled spans have positive completed receipt and matching
validated/processed chunks, boundaries, epochs, coverage and actual
scope/consistency evidence share the committed view. No End-only publication,
ignored decoder loss or cross-dispatcher atomicity is implied. See the event
contract for the exact receipt/absence and capture-vector rules.
`SequenceContinuity.Assess` is a stateless P01 contract helper, not a
collector, artifact verifier or processing worker.

Measurement (`Measured`, `Estimated`, `Unavailable`) and coverage
(`Complete`, `Partial`, `Unavailable`) are independent. Non-measured or
non-complete data needs a reason. Missing typed values have no numeric value
and unavailable measurement quality: zero remains a real observation.
Every result has selected session/process, committed ordinal/QPC and quality;
paged results also carry validated continuation bounds.

## Startup and frame accounting

Startup starts at observed process creation, not attach or the first tick, and
ends at the first **ten successfully submitted XAML frames total**, globally
across the selected process's roots/dispatchers. Late-attach creation history
and unavailable initial frames remain unknown/partial. Exit after fewer than
ten submits is partial; do not force idle frames to complete the view.

A submission is a **successful actual qualifying primary commit**, not
`Tick` success, outer rendering-function success, presentation, or any arbitrary
commit. In `CCoreServices::SubmitPrimitiveCompositionCommands` the primary
`CommitMainDevice` call precedes fallible `PostCommit` and layout notification
work, and optional debug-counter commits. `CommitMainDevice` itself returns
`S_OK` without a commit if `m_spMainDevice` is null.

The eventual hook must distinguish primary intent and actual device commit:
emit `Submitted` immediately after that actual primary device `Commit` succeeds.
A null-device no-op emits no submission. Later failure emits an independent
`Failed` observation for the same attempt; it **never retracts** the submission.
`AttemptCompleted` is the separate successful terminal attempt observation.
Standalone and debug-counter commits never acquire submitted frame IDs.
Submitted frame IDs are nonzero and monotonic in the authoritative runtime
stream; zero/no frame is not success. If an epoch/loss prevents joining startup
history, preserve partial coverage rather than resetting a ten-frame counter.

`FrameInfo` deliberately separates `AttemptStatus`/`HResult` from nullable
`FrameSubmission`. A failed attempt can therefore still carry its earlier
successful submission. P01 only defines/validates these observations; it does
not infer or implement rendering behavior.

Every non-standalone attempt uses `AttemptReference(ProducerEpoch, AttemptId,
RootId, DispatcherId)`. Typed history/frame observations distinguish lifetime,
type resolution, XAML property previous/current values, membership/order,
visual properties, resource state, correlations, allocation and layout
boundaries. History is not encoded as fake birth/destruction operations.
`ChangeCause` distinguishes unknown (with reason), known outside-attempt and
an explicit attempt. An omitted optional event cause means unknown, never
known outside. `FrameChange.Relation` separates proven causal changes from
temporal overlap; a timestamp or ActivityId alone is not causal evidence.
Frame changes retain QPC/ordinal and are chronological by that pair; causal
rows must match the exact attempt, including epoch/root/dispatcher.

Tree observations/results preserve root and dispatcher plus independent
logical, visual, composition or unknown membership. Authoritative runtime
type/bridge evidence distinguishes UIElement from other DependencyObjects;
an optional type display name is never classification evidence. The default
connected-element projection will count distinct XamlCore UIElement
identities with proven visual connection, not every connected
DependencyObject, logical-only relationship, peer or live object. Unknown
classification/membership means incomplete count evidence, not a guessed
zero. Canonical composition IVisuals remain a separate identity-domain count.
P01 supplies contracts/tests, not either counting processor.

## Memory and snapshots

OS process metrics use the underlying Task Manager OS collectors/semantics,
implemented independently: process-time deltas for CPU, OS GPU engine counters
for GPU, and OS working-set/private-usage queries for memory. CPU is normalized
by elapsed interval and logical processor count; GPU uses the OS per-process
engine accounting policy, not a sum of arbitrarily overlapping engine
percentages. Source, version, interval, adapter/engine and capability are
explicit. GPU identity is a typed physical-adapter LUID plus physical-adapter
index, and engine ID/class; it is not parsed from a display string.
Per-engine utilization is distinct from overall busiest-engine utilization;
do not sum concurrent engine percentages. GPU dedicated/shared commitment
and dedicated/shared residency are four separate byte metrics, scoped to a
physical adapter. An unknown winning adapter requires partial coverage;
unsupported metrics remain Missing/Unavailable and may lack GPU identity.
Unsupported collectors return unavailable, never a successful zero.

Private commit, total working set and private working set are different
metrics. Scoped XAML allocation size is not any of those. Resource creation
descriptions are estimates unless allocation bytes are actually measured.
WinUI hooks cannot supply all-process GPU resource ownership.

Lightweight XAML/resource snapshots and full native/managed heap snapshots are
separate artifacts/capabilities. Immutable `SnapshotMetadata` represents an
attempt: selected process, capture extent, nullable source version/epoch until
known, schema version, QPC frequency, committed ordinal, state, quality and
status reason. Pending/capturing attempts have no completed extent or sealed
artifact. Failed/cancelled attempts require a reason and need no artifact.
Only a materialized Complete/Partial snapshot requires a validated local
relative path/SHA-256, known end, epoch and source version. No placeholder
path/hash or successful empty capture substitutes for failure.

`CaptureExtent` records actual start/nullable end, mechanism, explicit
consistency and per-process/root/dispatcher/heap boundaries. Scope intervals
must fit the extent. Non-atomic bridge traversal, process snapshots, managed
runtime snapshots and polling are not interchangeable atomic instants;
atomic claims require evidence, and bridge/polling cannot claim
process-wide atomicity. Boundary collections are frozen with the metadata. Every scope interval is
also checked against the committed query watermark, even when the overall
extent is unfinished and artifact-free.
Baseline capture chunks carry typed objects, topology,
associations, visual properties, correlations, resource/backing state, allocator
state and proven/uncertain ownership. Per-root/dispatcher boundaries are
explicit; no cross-dispatcher atomicity is implied.

Snapshot object sizes are nonnegative numeric Bytes or explicit
Missing/Unavailable. Resource observations/results retain authoritative
`ResourceClassification` (kind, creation-API/runtime evidence, source);
unknown kind is explicit and never inferred from an optional type name.
API-description sizes are Estimated whether coverage is Complete or Partial;
only authoritative allocation measurements may claim Measured provenance.

Snapshot comparisons select one distinct baseline and 1–64 distinct later
captures. The future service must reject cross-session/process/kind comparisons
and unsupported capabilities with typed errors, not empty successful pages.
Detached means disconnected, not destroyed. WinUI ownership release and
resource close do not prove an external COM object's final destruction.
Ownership paths expose `Proven` versus `UncertainNativeRetention`; arbitrary
native memory does not prove ownership or roots.

Each comparison row has a typed key: attribute (object/connected-element/
canonical-visual count, allocated/requested/actual bytes) and session/object/
type/root/resource-kind grouping with the corresponding qualifier. Aggregate
rows with null Object therefore remain distinguishable. Compatible values
retain the attribute's units; counts are integers and signed deltas are
valid. MissingBaseline, MissingCurrent and Incompatible preserve their
available raw values and an explicit reason; their delta is Missing, not zero.

## Read-only query boundary

`IProfilerQueryService` defines twelve typed operations (no SQL/chat/MCP):
session, metrics, startup timeline, frames, object history, XAML tree,
composition tree, correlations, resource usage, snapshots, snapshot comparison
and retention paths. The saved-session [CLI](cli.md) routes these operations,
not arbitrary SQL; comparison/retention currently report the engine's explicit
unsupported status. There are no target commands on this interface.

`QueryValidation.Validate` is mandatory at the service boundary and checks the
caller cancellation token before work. Paged limits are **1–4096**; offsets
are nonnegative/nonoverflowing. Subsequent pages must supply the first page's
committed watermark. Services must freeze that view (same query/filter/order),
reject unavailable/expired watermarks, order deterministically by stable
identities/timestamp/ordinal, and check cancellation during reads.
`PageInfo` has no total-count promise; `HasMore` requires an exact next offset.
`QueryPage<T>` and `QueryResult<T>` require a supported typed query value and
invoke mandatory typed validation/freezing on construction, including JSON
deserialization. They validate timestamps/ordinals against the committed view,
process/domain membership, source/version, enums, units, discriminators,
provenance, causal references and compatibility. They copy nested retention
edges, frame changes, composition properties, producer/capability arrays and
saved per-listener domain intervals, not just the outer page. No reflection
or generic object-graph walker is used. Session metadata bounds are 4096
producer records and 8192 listener-coverage intervals; callers must fail
visibly rather than truncate required evidence. Composition results expose
typed observed properties alongside scoped topology.

Object queries validate the full process instance and domain. Retention depth
is 1–256. P02 validates persisted snapshot metadata and snapshot-fact membership.
Comparison compatibility and snapshot-scoped graph traversal remain later
deliverables and explicitly throw `NotSupportedException`, rather than
returning invented deltas or empty proven paths.
Cancellation throws `OperationCanceledException`; malformed input throws
argument/JSON errors; unsupported schema/type versions throw
`NotSupportedException`. No broad catch or success fallback is used.

## Safety, limits and local artifacts

All captures and reports stay local by default; no network publishing or
automatic uploads. Offline sessions may query saved artifacts but cannot issue
rundown/snapshot/heap/TAP commands. Authentication and authorization are not
supplied by a JSON nonce alone (see event contract for the P08 obligations).

Every ingestion boundary copies payload storage and validates required fields,
enum values, units, bounded lengths, schema and source/target identity.
`OwnedPayload` never exposes its backing array, including through
`MemoryMarshal`; copying is deliberate in this foundational codec.
Canonical JSON rejects unknown/duplicate properties, numeric/unknown enums,
missing constructor fields, invalid UTF-8 and incompatible typed payloads.
Unknown future schemas fail visibly; no fabricated successful decoding.

The future processor must stop visibly on queue, memory, event/history/store
limits or persistence failure, retain previously committed history and expose
the stop reason and loss interval. It must not silently evict history or keep
reporting complete coverage.

Production instrumentation must check provider **and keyword** before ID/type
lookups, payload construction, strings, graph walks, allocator-size queries or
locks. Provider-off and each keyword's on-path overhead, bytes/event and
allocation effects must be measured in fre x64/ARM64 runs. These are later
acceptance gates, not performance claims from portable P01 tests.

## Synthetic evidence

`FakeClock` is an explicit-advance test clock with checked arithmetic.
`SyntheticEventBuilder` constructs a deterministic session with independently
sequenced runtime/collector delivery domains. `TestData\minimal-session.json` is a
versioned canonical-envelope fixture (payload bytes are base64 as in the codec)
and is checked byte-for-byte against the builder after decoding/re-encoding.
It covers lifecycle, type/core-peer identity, XAML/composition roots/topology,
visual changes, correlations, device/resource/backing, allocator, baseline
chunks, boundaries, a real submitted-frame observation, independent attempt
completion, explicitly causal XAML property/layout observations, authoritative
UIElement versus logical-only DependencyObject evidence, capture sequence
vectors with explicit settled producer cuts (not listener closure),
CPU/unavailable-GPU/private-working-set/private-commit samples,
typed GPU per-engine/overall samples and all four GPU memory distinctions,
detach/release/destruction and process exit. One submitted frame deliberately
does not claim completion of the ten-frame startup requirement.
