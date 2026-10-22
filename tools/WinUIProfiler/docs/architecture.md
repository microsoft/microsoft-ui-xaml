# WinUI Profiler foundation (P01–P04)

## Scope and build

This is a new, independent profiler. P01 contains portable `net10.0` contracts,
bounded event/protocol codecs, validation, and deterministic synthetic tests.
P02 adds transactional SQLite persistence and bounded, read-only typed queries.
P03 adds real live/offline ETW ingestion, typed native decoding, bounded
dedicated processing/writer threads and durable diagnostic/stop control.
P04 supplies the native typed writers/private protocol foundation.
There is no UI, target injection/control transport, metric collector,
CLI implementation or framework observation hooks here. Automatic runtime
capabilities remain empty and capture is unsupported. See [ingestion](ingestion.md).
No other profiler is a source, project, package, binary or design dependency.

The complete suite includes Windows ETW integration tests, which require
permission to create/enable the uniquely owned sessions and an x64 Release
native fixture harness. Build that prerequisite from the repository root
using the command in [ingestion verification](ingestion.md#verification-and-intentionally-absent-work);
permission or missing-harness failures are not silently skipped.

Then, from `tools\WinUIProfiler`:

```powershell
dotnet restore WinUIProfiler.sln --locked-mode
dotnet test WinUIProfiler.Engine.Tests\WinUIProfiler.Engine.Tests.csproj -c Release --no-restore
dotnet build WinUIProfiler.sln -c Release --no-restore
```

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
and retention paths. The future CLI will expose these operations, not arbitrary
SQL. There are no target commands on this interface.

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
