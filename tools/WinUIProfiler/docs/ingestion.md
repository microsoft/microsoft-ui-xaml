# Bounded ETW ingestion (P03)

## Entry points and wire support

Create a new `SessionStore`, then a `ProcessingWorker(store, recordingInfo,
options, processors)`. Keep the store alive until the worker has completed.
`LiveEtwSource.RunAsync(worker)` and `OfflineEtlSource.RunAsync(worker)` each
own one acquisition lifetime; they cannot be run twice. A worker permits one
active acquisition. Direct typed collector observations may also enter through
`TryEnqueue(DecodedEvent)`: their envelope's session and semantic target, **not**
its header PID, must match the selected scope.

The live adapter creates a random `WinUIProfiler-<GUID>` real-time ETW session
with `NoRestartOnCreate`, 16 MiB transport buffers and 64 KiB buffer quantum.
Only that owned session is queried/stopped/disposed. It enables the new runtime
and collector providers; it never attaches to/restarts an existing session,
stops a global kernel logger, elevates, or installs/uninstalls a target.
Await `Started` before asking a target to emit; startup/permission errors fault
that task and `RunAsync` and produce failed terminal health, not a skipped test.
Live process selection requires actual creation evidence; `Identify(Process)`
is an explicit **live-only** helper using the process's creation time.

Both adapters use pinned `Microsoft.Diagnostics.Tracing.TraceEvent` **3.2.8**
from this tool's own central manifest/lock files. No other profiler's importer,
packages, capture format or event model is used.

Supported native runtime schemas are all sixteen P04 definitions in
`event-contract.md`, including all nine CaptureChunk state arrays and its five
event-local reference tables. Selection uses provider GUID, exact name,
SemanticEventId, EventSchemaVersion, level and keywords, never TraceLogging's
numeric descriptor ID/version. The decoder validates exact ordered field/struct
shape, primitive representations, bounded array counts, all required/inactive
field storage, strict UTF-8, raw packed payload extent, nullable references,
enum/value/quality arms, scope/epochs and the resulting P01 observations.
The approved schema's typed metadata plus data budget is 48 KiB; normalized
payloads independently retain their 1 MiB bound. This is a decoder, not a
managed copy of the native producer or an EventSource fixture.

The explicit additional platform schema is
`Microsoft-Windows-Kernel-Process`, GUID
`22fb2cd6-0e7b-422b-a0c7-2fad1fd0e716`, ProcessStart/Start, ID 1, **version 4**,
level 4, keyword `0x8000000000000010`. This mapping was verified against an
actual Windows event. PID and creation FILETIME come from `ProcessID` and
`CreateTime` in the payload; the header PID is the creator/emitter. Exact
creation QPC is unavailable, not inferred from arrival. The stream namespace
is trace-local and SourceSequence is null: no native epoch/sequence settlement
is invented. Other platform versions/families are explicit unsupported-schema
diagnostics. The normal live adapter does not enable global kernel families.
`EventCatalog.All` remains the fixed new-profiler schema-1 catalog; the explicit
platform definitions reside in `EventCatalog.Platform` and share codec lookup.

There is **no native collector wire schema/producer yet**. Collector GUID
events cannot be guessed from the runtime schema or treated as known solely
by name: they are retained as unsupported diagnostics. Already typed P01
collector observations are supported without a header-PID-equals-target filter.
P07 counter sampling and its producer are not implemented here.

## Ownership, process identities and clocks

The ETW callback makes an owned bounded copy, decodes typed records, and uses
non-waiting ingress admission. No TraceEvent, StructValue, recycled native
buffer, dispatcher/UI callback, SQLite operation or persistence-capacity wait
escapes into the acquisition admission path.

Actual PID + creation FILETIME binds a native producer's process GUID into the
selected canonical process instance. Those GUIDs need not equal each other.
`SourceProvenance` transactionally retains the original process identity and
raw timestamp separately from the canonical target and emitting header PID/TID.
If creation evidence is absent, it stays null; only the explicitly selected
trace-local instance matches, not another instance sharing its PID. Offline
ingestion never polls a current process to repair old identity/coverage gaps.

Offline reads hold a read-only file lease excluding writers/deletion for the
enumeration. `ReadMetadata` obtains frequency and clock type from the recorded
ETL header. Only raw-QPC clock type 1 is supported. No current-machine frequency
is substituted. TraceEvent's raw `TimeStampQPC` is used intentionally rather
than relative double milliseconds or converted FILETIME. The same checked
Int128 origin/frequency mapping applies to header timestamps and payload QPC
intervals; the default origin is zero (original QPC coordinates).

This lease and an ETL end do **not** manufacture P01 delivery closure, positive
receipt, startup coverage, or a complete baseline. Producer cuts carried by
CaptureBoundary retain their full delivery domain and reserved/settled values.
`ReconcileSequences` uses `SequenceContinuity.Assess` with explicit controller
evidence and rejects receipt ahead of the worker's processed prefix. Arrival
gaps and reordering alone never become producer loss. No private target-control
transport or automatic runtime capture was added.

## Dedicated processing and bounds

The worker starts two named, dedicated threads:

* The state owner assigns contiguous selected-session ordinals, preserves late
  QPC timestamps, invalidates on health changes and calls every registered
  processor for the same immutable ordered batch. Up to five future projection
  processors are supported. It freezes changes before publication.
* The single SQLite writer calls P02's synchronous writer API. Normalized
  sources, original provenance and associated derived changes commit in one
  transaction. The durable watermark advances only after `Commit` succeeds.
  Readers continue to use durable database checkpoints, never owner memory.

Default logical queue bounds (both independently enforced):

| Boundary | Items | Bytes |
|---|---:|---:|
| Pending ingress | 4,096 | 8 MiB |
| Owner batch | 128 | 4 MiB |
| Pending persistence | 8 | 16 MiB |
| SQLite writer batch | 4,096 hard maximum | 16 MiB hard maximum |

There can additionally be one owner/in-flight batch and one writer/in-flight
batch; limits are not a claim about total CLR object overhead or ETW buffer
memory. Source/envelope bytes, provenance, changes and batch metadata are
accounted, with independent payload/metadata bounds. No history is evicted.
The owner may wait up to five seconds for persistence capacity; the ETW callback
never waits for the writer. A configured bound/deadline breach stops acquisition
and marks an explicitly incomplete tail. Producer metadata retains at most 128
epochs. Full histories live in SQLite; future processors must bound active
indexes/correlation windows rather than retain all events.

A new owner refuses blind resume of existing history/diagnostics: future
recovery must explicitly rebuild processor and continuity state. P02 writable
reopening and read-only recovery of an unfinished session remain supported.

## Health and shutdown

Independent categories cover decode/schema failures, ETW event loss, ETW buffer
loss, producer-reported loss, proven producer sequence gaps, ingress overflow,
persistence backlog, writer/source failure, partial identity and missing
capabilities metadata. Live loss polling uses the public Windows SDK
ControlTrace QUERY structure and **only the owned session name**, including
separate log/real-time buffer loss counters. Offline loss comes from the
recorded header/final enumeration; it is not sampled from today's machine.
Owned native STOP also returns final transport statistics, which are reported
before releasing the acquisition lease, including loss during final flushing.

The reserved control path has a fixed category array, first representative per
category and cumulative counts. It also retains up to 128 distinct unknown/
decode schema representatives (bounded metadata and 256 raw payload bytes),
with repeat counts. The next distinct schema stops acquisition instead of
evicting earlier evidence; its first overflow sample remains in the reserved
DiagnosticLimit category. These records do not depend on normal queue space
and never masquerade as target observations.

Stop seals admission and requests source cancellation asynchronously. Accepted
ingress and available bounded persistence batches drain; an acquisition lease
keeps final loss/control callbacks alive after ingress drains. The owner then
performs final invalidation **on its own thread**. Only after acquisition and
owner finalization does the writer persist terminal health. No diagnostics are
accepted after that terminal boundary. `ProcessingDrained` is not `Completion`.
A user stop/cancellation is `Stopped`, not target loss. An enumerated finite
source can be operationally `Completed` while observational coverage remains
partial; a schema/loss failure cannot become a successful completion.

Control/terminal records bind to the last actual durable source checkpoint,
including `(0,0)` for an empty recording. They neither invent a source ordinal
nor rewrite historical source checkpoints/derived rows. If SQLite fails,
the existing preallocated 4 KiB `.failure.json` stores failed status/reason,
SQLITE_FULL where applicable, and last durable boundary; a sidecar write failure
is also returned explicitly. A failed recording never resumes or reports success.

## Verification and intentionally absent work

Build the native fixture prerequisite from the repository root on Windows:

```powershell
.\initrun.ps1 -Flavor amd64fre msb /q dxaml\test\native\external\framework\profiler\ProfilerInstrumentationHarness.vcxproj /p:Configuration=Release /p:Platform=x64 /p:UseXamlCompiler=false /p:PGOBuildMode=Off
.\BuildOutput\ProfilerHarness\x64\Release\ProfilerInstrumentationHarness.exe
```

Run the managed restore/build/tests from `tools\WinUIProfiler` as documented
in [architecture](architecture.md#scope-and-build), using its independent SDK.

The Windows tests invoke the **actual P04 native harness**. Its standalone
`--emit-fixtures <directory> [--await-input]` and
`--emit-budget <directory>` seams retain uniquely named owned ETLs; ordinary
conformance behavior/deletion is unchanged. Tests replay complete typed schemas
through real ETW readers, the decoder, both worker threads and SQLite, compare
semantic records/query metadata across imports, exercise two live sessions and
continue receiving after the first stops. Native schema samples are explicitly
fixtures, not observed framework object/graphics lifetimes.

Automatic runtime capability arrays remain empty and capture remains
`E_NOTIMPL`. There are no framework hooks, app-baseline completeness claims,
target transport, P07/P08 acquisition algorithms, P09–P11 projections or UI.
Actual transport overflow is not intentionally forced on a shared machine;
deterministic transport-loss inputs and actual native Loss records test health
semantics, while owned live QUERY calls execute for real.
