# Bounded ETW ingestion (P03)

## Entry points and wire support

`TargetConnection` uses `BufferedRuntimeEvents` for its live connection feature:
a dedicated listener owns TraceEvent callbacks, a long-running decoder drains
an owned queue bounded to **16384 records / 64 MiB retained-byte accounting**,
and the existing owner/writer persists input-only session batches. The callback
copies schema data but does not decode typed observations or call SQLite/UI.
`NativeRecord.RetainedBytes` conservatively accounts for retained payload,
field/container and string/array data; it is not a GC working-set measurement.
The listener starts before target selection, then filters by target header PID;
the private-runtime decoder verifies full PID/creation identity. Standard XAML
headers have only PID evidence; their original creation time remains null and
their semantic coverage is partial. Overflow, failed ingress and
ETW loss surface as incomplete acquisition, never successful silent eviction.
Default private connection keywords are `0x11`; explicit detailed capture uses `0x3f`.
The connection also enables standard XAML Core/Layout/Rendering (`0x1044`,
informational), or all standard keywords at verbose level for detailed capture.
The callback copies standard payload bytes without interpreting arbitrary TDH
field shapes; the decoder persists the original descriptor as source evidence.
This is distinct from the lower-level `LiveEtwSource` adapter described below.
See [live controls and scope](app.md#launch-or-connect-to-a-target).

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

Admission precedes full body capture/validation. The runtime provider's
emitter-equals-target rule excludes a provably foreign header PID even for an
unknown or truncated body. For the same PID, a supported schema-1 common prefix
with valid actual creation evidence can exclude a different process instance
before body validation. Unsupported/unattributable prefixes remain diagnosable.
These rules apply **only** to the runtime provider, never to collector/kernel
header PIDs whose payload target can legitimately differ.

The explicit additional platform schema is
`Microsoft-Windows-Kernel-Process`, GUID
`22fb2cd6-0e7b-422b-a0c7-2fad1fd0e716`, ProcessStart/Start, ID 1, **version 4**,
level 4, keyword `0x8000000000000010`. This mapping was verified against an
actual Windows event. PID and creation FILETIME come from `ProcessID` and
`CreateTime` in the payload; the header PID is the creator/emitter. Exact
creation QPC is unavailable, not inferred from arrival. The stream namespace
is trace-local and SourceSequence is null: no native epoch/sequence settlement
is invented.

Standard `Microsoft-Windows-XAML` (`531a35ab-63ce-4bcf-aa98-f88c7a89e455`)
events use an explicit local `ObservedXaml` wrapper, provider
`c558a4ad-4e42-4ea1-b547-c406a47f9409`, ID/version 1/1. This GUID is a local
normalization namespace, not an enabled ETW provider or a target producer epoch.
The typed payload preserves original provider/name/ID/version/level/keywords/
opcode/task and all raw bytes up to 48 KiB, including empty payloads and native
descriptor version zero. Header PID/TID/activity/QPC and source clock are retained.
Target filtering precedes payload capture. SourceSequence remains null and the
stream is trace-local. Original creation FILETIME remains unavailable rather
than being copied from the selected live process. These events establish source
evidence only, never private lifetimes, submitted frames or baseline completeness.
The live connection enables this provider; offline decoding accepts it when present.

Other platform versions/families are explicit unsupported-schema
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
It also references immutable `SourceClockMetadata`: acquisition ID, recorded
clock type, source frequency, source origin and normalized session frequency.
All actual decoded records carry that mapping; registration is bounded to
16 acquisitions per worker/store. Clock registration is persisted even if a
metadata-known import accepts no observations. Close/reopen needs neither an ETL
nor a current-machine frequency query to interpret original raw timestamps.
Schema-1/2 histories and explicitly mapping-unavailable typed input retain
unknown/null mapping; migration never invents it.
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
  processor for the same immutable ordered batch. The default five projections
  are ObjectState, Frame (composing metrics), XamlTree, CompositionTree and
  ResourceUsage. It freezes changes before publication.
* The single SQLite writer calls P02's synchronous writer API. Normalized
  sources, original provenance and associated derived changes commit in one
  transaction. The durable watermark advances only after `Commit` succeeds.
  Readers continue to use durable database checkpoints, never owner memory.

Passing no processor list selects all five, including through
`LiveMetricRecording`. An explicitly supplied list replaces that default; `[]`
selects input-only recording. Verified capture End routing and atomic baseline
page delivery are not yet integrated, and raw capture events cannot establish
baseline completeness.

### Owner-side baseline page preparation (bounded P08 building block)

`BaselinePagePreparer` is constructed and invoked synchronously on the calling
state-owner thread. The caller must provide the actual next-canonical validated
runtime baseline End, its preceding canonical watermark, and already-reconciled,
owned chunks. `PreparedBaseline` assembly/delivery evidence alone is **not**
reconciliation or permission to invoke this operation. Neither this component
nor `CommitBaseline` establishes controller publication authority.

The preparer accepts the complete bounded ordinary processor set (at most five).
It exhausts `PublishBaseline` for each `IVerifiedBaselineProcessor` and ordinary
`Process` on the singleton actual End for every other supplied processor, on
that owner, freezing and checking each candidate incrementally. No processor is
silently filtered. The returned
`BaselineProjectionPages` contains only immutable End/info/quality/provenance
and read-only frozen pages, never a deferred processor iterator, chunks, or
processor state. The writer may later pass those values to `CommitBaseline`;
there is no writer queue/control integration or automatic raw-End routing here.

The aggregate allowance is shared across all supplied processors: at most
131,072 derived records and 64 MiB of logical encoded accounting. Each page
contains at most 4,096 rows and 16 MiB of changes, and each derived serialized
value is at most 64 KiB. Optional smaller aggregate allowances cannot exceed
those hard limits. Header/row validation and freezing are shared with
`CommitBaseline`: encoded original End, frozen info/quality, original provenance
including clock mapping, one 32-byte header, and 16 bytes per change are counted
exactly once. Pages do not introduce additional logical headers. These limits
do not increase ordinary `Commit`'s 4,096-row/16-MiB bounds and are not a total
CLR allocation bound. Composition's local emission allowance is independent;
only the preparer consumes the shared output budget.

Preparation is single-attempt. Validation failure, output overflow, processor
failure or cancellation returns no result; disposal stops enumeration and no
storage publication occurs. Discard **all involved processor state** after a
failed attempt: partial state mutation is terminal, not rollback or retry.
Successful preparation is still not a durable commit or publication permission.
The writer revalidates the actual durable predecessor and immutable session
mode/frequency when it commits.

**All-five End preparation prerequisite is implemented:** the exact ordinary
default set remains ObjectState, Frame, XamlTree, CompositionTree and
ResourceUsage, with a single resource correlation owner and no sixth metric
processor. Frame does not read CaptureBoundary as a frame fact, but its ordinary
`Process([End], context)` preserves retained frame/startup revisions when quality
changes or invalidation is pending. These revisions reference the real canonical
End ordinal; their historical ranges, submission timestamps and change
provenance retain Frame's existing semantics. No Frame fact is invented for an
empty history or an unchanged context.

`CommitBaseline` admits `FrameInfo` and `StartupEntry` in addition to object,
tree, composition, correlation and resource rows. Both preparation routes share
the same aggregate budget, cancellation, source/key validation, ordinary row
freezing and immutable pages. All rows and original End target/QPC/clock commit
atomically or not at all. `MetricSample` remains rejected for a baseline End;
snapshot/session results are not newly admitted. This closes the fifth-processor
preparation/storage gap, not the remaining coordinator/reconciliation, raw-End
routing or owner/writer queue integration gates, and does not claim P08 completion.

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
The logical encoding budget includes the complete encoded envelope/provenance
(including clock mapping), encoded info/quality, a 32-byte batch header and
16 bytes per change for source/deletion/type bookkeeping. An initial source
must fit **with metadata/provenance** before any event processor is invoked.
Subsequent input that does not fit is carried into the next batch, not dropped.

`IEventProcessor.Process` returns an incremental `IEnumerable<ProjectionChange>`.
Implementations must yield one bounded candidate at a time, not eagerly build
an output list. `ProcessingContext.OutputBudget` exposes the shared remaining
bytes/records across all five processors. Each candidate is counted through a
capped, non-retaining serialization stream before freezing; one output record
is capped at 64 KiB. A failure disposes that enumeration without requesting more
outputs or invoking remaining processors. Serializer transient buffers and the
candidate object itself are CLR overhead, not an allocator-level byte guarantee.
One input lookahead and one bounded output candidate are additional transient
work, not unbounded retained output collections.

If input, metadata or output cannot fit, the entire uncommitted source batch is
marked incomplete and acquisition stops at the preceding queued/durable source
boundary; earlier queued batches still drain. Zero remaining output allowance
conservatively stops rather than invoking more projection code to guess whether
it would yield nothing. A full input-only batch with no processors can commit
at the exact byte boundary; default projections require additional output allowance.
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

Accepted stream/provider/PID/epoch tracking is independently bounded to 128.
Metadata is matched to each accepted epoch, not checked globally. An announced
epoch does not hide a different unannounced epoch. Each unmatched epoch reports
MissingMetadata with its provider/header PID and epoch identity and a bounded
representative in the same 128-entry diagnostic pool. Missing metadata alone
remains partial coverage, not unknown schema or a pre-enablement admission gate.
Tracking/diagnostic capacity exhaustion stops rather than evicts evidence.

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
Native-emitting test classes share an xUnit collection so separate conformance
fixtures do not pollute one another's globally enabled provider sessions; the
two-live-session test still explicitly exercises concurrent independent sources.

Automatic runtime capability arrays remain empty and capture remains
`E_NOTIMPL`. There are no framework hooks, app-baseline completeness claims,
target transport, P07/P08 acquisition algorithms, P09–P11 projections or UI.
Actual transport overflow is not intentionally forced on a shared machine;
deterministic transport-loss inputs and actual native Loss records test health
semantics, while owned live QUERY calls execute for real.
