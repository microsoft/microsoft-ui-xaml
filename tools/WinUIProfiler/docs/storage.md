# Durable storage and read-only queries (P02)

## Lifecycle and ownership

`SessionStore.Create(path, scope, info, quality, options)` creates a **new**
single-session/single-process database. Its initial checkpoint is `(0,0)`;
initial metadata must fit that boundary. The caller creates the containing
directory. Existing files are never overwritten/deleted. `OpenWritable` only
opens a compatible existing database; it does not silently initialize an
empty or foreign file. `OpenReadOnly` requires an existing compatible file,
opens SQLite with `Mode=ReadOnly` and `query_only=ON`, and runs no migrations.
The public query/writer API never accepts SQL.

Writable lifecycle setup enables WAL, `synchronous=FULL`, foreign keys and
`trusted_schema=OFF`. A file-share-exclusive `.writer` lease enforces one
writable store (including across processes); one `SessionWriter` may be
created from that store. The store serializes commits/disposal. Dispose the
store after its writer is no longer used. Read-only stores and independent
query connections do not take the writer lease. They use normal SQLite
snapshot isolation, not shared-cache/read-uncommitted connections.

The embedded `Storage\Migrations\001_Initial.sql` runs transactionally only
during explicit creation. `application_id`, `user_version=1`, a canonical
table/index schema hash, SQLite integrity/foreign-key checks, process metadata,
checkpoint payload version/status and durable source count/maxima distinguish
supported, incompatible, corrupt and future databases. Schema 0 is not a
previous profiler format; it is rejected. Future schema versions are explicitly
unsupported. No destructive recovery, migration-on-read or old-format import
is provided.
All logically related open-time checks execute in **one deferred read
transaction**, including the checkpoint and source-history comparison. A WAL
writer committing between those statements cannot create a false corruption
diagnosis. The validation snapshot ends before writable WAL/quota configuration.

`SessionStatus.Recording` remains persisted until a supplied terminal status
commits. `UnfinishedRecording` reports that fact on reopening; reopening is
not evidence of completion. P02 does not infer exit, stop a capture, drain a
worker, or resume an interrupted producer.

## Atomic writes and temporal model

A `SessionBatch` supplies:

* normalized source events with contiguous session-wide ingestion ordinals;
* explicit `ProjectionChange` records anchored to a source ordinal **in that
  batch** (not nearby timestamps);
* complete session metadata and **cumulative quality at the new checkpoint**;
* optional explicit `SnapshotAttributeChange` / `RetentionEdgeChange` facts,
  also source-anchored, referring to known committed or same-batch snapshots.

Sources are re-normalized by the current P01 codec. Query values use the
current P01 validation/freezing boundary. There are at most 4096 sources and
4096 combined projections/facts, with a 16 MiB encoded batch budget and
1 MiB per-projection/fact payload ceiling. Validation precedes the transaction.
Each checkpoint's session metadata and quality payload is also limited to
1 MiB, including initial creation; oversized metadata is rejected before a
database is created or a batch transaction starts.
Sources, projection revisions, specialized facts, quality and the new
checkpoint commit in **one transaction**; there is no separately published
in-memory watermark. The timestamp watermark is the maximum observed QPC,
not the last-arriving event's timestamp. Late source timestamps are allowed.

`Sessions` and `ProcessInstances` retain immutable scope; `Checkpoints` retain
all metadata/quality versions. `SourceEvents` contain typed source identity,
process, time, sequence and an encoded P01 envelope. `ProjectionRows` contain
typed dataset/key, source/commit ordinal, time, object/parent, root/dispatcher,
metric, snapshot kind/ID, owner, tombstone and payload-version columns.
The full immutable P01 record is additionally retained in versioned JSON.
Filtering/paging never deserialize source histories or scan JSON fields.

All planned tables exist:
`Sessions`, `ProcessInstances`, `Capabilities`, `SourceEvents`, `MetricSamples`,
`ObjectLifetimes`, `ObjectProperties`, `Relationships`, `Roots`, `FrameAttempts`,
`FrameSubmissions`, `FrameChanges`, `Resources`, `ResourceAllocations`,
`ResourceOwners`, `Snapshots`, `SnapshotAttributes`, `SnapshotArtifacts`,
`RetentionEdges`, `CaptureEpochs`, `QualityIntervals`, `Checkpoints`.
`ProjectionRows` is the shared immutable revision parent for specialized
tables. Indexes cover process/time, object/time, parent/child validity,
frame, resource/allocation, owner and snapshot identities. Since a database
has exactly one selected process, projection process isolation is implicit
in its validated session scope.

Queryable values retain numeric kind/unit/value/measurement/coverage columns;
missing is SQL NULL plus an explicit discriminator/reason, never zero.
UInt64 identities are fixed-width uppercase hexadecimal text, preserving
the entire unsigned domain, including the high bit. Allocation byte counts
also have exact numeric high/low 32-bit columns for unsigned comparison and
ordering without signed overflow or floating-point rounding.

Revisions are unique by dataset/key/**source ordinal**, not commit alone:
one batch may contain many successive changes to the same object.
Historical object observations distinguish property/membership/etc. keys.
State rows are append-only, including explicit tombstones. To remove an
ownership/correlation/topology edge, processors must supply its exact key
as a tombstone; changing an owner is not evidence that another owner was
removed. Resource owners and correlations are many-to-many typed edges,
not one mutable `OwnerId`. Multiple resource-owner entries must not be
summed as distinct allocations.

Capability and capture-boundary source payloads additionally populate their
typed tables. Projection records populate their applicable specialized
tables without inferring other observations. Snapshot attributes/ownership
edges persist explicit facts; this does **not** establish snapshot completeness,
capture consistency, root reachability or comparison compatibility.

## Queries, boundaries and continuations

The service implements the existing `IProfilerQueryService`, without changing
P01 contracts. Every page reports scope, a **real committed checkpoint** and
quality. Arbitrary ordinals between batch commits and mismatched ordinal/QPC
pairs are rejected. A reader transaction binds metadata and rows together.
SQL applies selection, temporal anti-joins, ordering and `LIMIT limit+1 OFFSET`.
Only the requested page is materialized; limits are 1–4096 rows and **16 MiB**
encoded payload per page. A byte-budget violation throws
`QueryMaterializationLimitException`; reduce the requested page size.
There is no unbounded total-count query or silent truncation.

Retain a page's `Context.Watermark` for synchronized subsequent queries,
including page zero. Every state anti-join excludes revisions beyond that
checkpoint. Later commits/tombstones cannot rewrite old pages. Paging order
is QPC, source ordinal, then stable typed row key; tree entries carry explicit
order/parent fields but are not promised in traversal order.
For point-effective state, dominance is **QPC then source ordinal**, restricted
to evidence at or before the requested point. Late arrivals with older
effective timestamps cannot suppress a later-time state or tombstone. Equal
QPC ties use the larger ordinal. Recursive root closure applies the same
dominance rule. Non-point revision datasets (for example frame attempts and
snapshot metadata) still use the latest source-ordinal revision: the start
of a frame/capture interval is not its correction/arrival revision order.

P01's `PageRequest`/`PageInfo` expose offsets, not a token property. P02's
`Continuation(request, page, context)` and `Resume(request, token)` bridge that
boundary without changing it: `Resume` returns the next `PageRequest`.
Tokens are deterministic versioned HMAC-SHA256 envelopes bound to the
persisted session secret, request type, full scope, all filters, limit,
checkpoint and next offset. Modified/malformed tokens, other databases,
changed filters/limits or conflicting explicit boundaries are rejected.
Tokens survive reopening and later commits. Direct caller-selected offsets
still represent explicit P01 queries at a checkpoint; use these helpers when
continuation request/filter binding is required.

Supported operations read **explicit** metrics, startup phase entries, frame
attempt/submission/change projections, object history, XAML/composition
topology/properties, correlations, resource states and snapshot metadata.
No ETW event pairing, property/lifetime inference or capture processing occurs.
Range intervals are half-open, except zero-duration point observations.
Point frame queries include an interval's start/interior, not its exclusive
end; an actual zero-duration frame remains selectable at its own point.
Topology queries apply latest explicit revisions at the requested QPC;
optional root selection uses a database recursive descendant closure with
deduplication. Correlations require their supplied validity to include the
point (they are not extended to a query timestamp). Resource usage returns
latest explicitly observed resource-owner/backing states as of the range's
exclusive end (or its point), clamped to the committed timestamp; it is not
an allocation total or an inferred lifetime integration.

If a dataset/selected metric/object has never been observed at that boundary,
an empty page explicitly reports **Unavailable** coverage and a reason.
A filtered interval with no rows is distinct from an unobserved dataset.
Root selection requires explicit evidence for that root, in the selected
dataset, at/before the requested QPC and within the retained checkpoint.
An unrelated observed root or a future root observation does not establish
coverage for an unknown selection. Previously observed roots can still yield
genuinely observed empty pages after explicit removal; emptiness alone does
not imply unavailable coverage.
Supplied estimated/missing values and per-entry quality are preserved;
checkpoint quality is supplied by the future processing owner and is not
inferred from a successful SQLite write.

Snapshot comparison and retention-path traversal validate requests and throw
explicit `NotSupportedException`s. Their compatibility/graph algorithms
belong to later snapshot deliverables; storing attributes/edges alone is not
permission to fabricate deltas or proven root paths.

Cancellation is checked before validation/work, before/through writes, and
through materialization. Read execution installs the pinned SQLitePCLRaw
native progress handler (every 64 VM instructions) and a token-triggered
`sqlite3_interrupt` on its dedicated connection. Both availability and result
SQL, including work before the first row, are covered. The persistent progress
check closes the idle-interrupt race between a managed check and the next
native step. Token-attributable `SQLITE_INTERRUPT` maps to
`OperationCanceledException` with its native inner exception/token; unrelated
SQLite errors retain their meaning. The registration is drained and progress
handler removed before the caller disposes the connection. Read transactions
and statements unwind normally, and cancelled reads do not fault recording.
This does not use the provider's no-op `SqliteCommand.Cancel`. Hard real-time
native/lock-wait latency is not promised; this is not an asynchronous worker.
`GetSession` has no historical boundary parameter in P01 and returns the
latest session status, including a known persistence failure.

## Failure durability

SQLite I/O/full/authorization write errors roll back the whole transaction,
throw `SessionWriteException` (including native `SqliteErrorCode`), and
permanently fault that recording's writer. Previously committed history and
its watermark remain readable; a failed recording cannot become Completed
or silently resume. Argument validation/cancellation do not label a recording
failed or advance its boundary.

Creation reserves a fixed **4096-byte** `.failure.json` sibling outside the
SQLite write path, initially JSON null plus padding. On write failure it is
overwritten in-place and flushed with session ID, last durable checkpoint,
error code and a bounded non-sensitive reason. Read-only reopening recognizes
this failure and reports `SessionStatus.Failed` honestly while retaining the
last durable DB boundary. The failure record never overwrites committed
projection/quality history.

Preallocation improves behavior under full-disk conditions, but storage
hardware/ACL failure can also prevent the fallback write. That secondary
error is exposed in `SessionWriteException.FailureRecordError`, never hidden.
A still-open store also retains its known Failed status in memory if writing
the sibling fails; a later process can only identify the durable unfinished
recording if no failure record could be persisted.
A torn/invalid failure record is rejected instead of claiming completion.
SQLite recovery/WAL and all session siblings must be preserved together;
P02 does not implement archival/export/repair. `MaximumDatabasePages` is an
optional connection-local SQLite page quota; tests exercise actual
`SQLITE_FULL` with a real filesystem database and quota, not an invented
success/failure mock or filling the shared machine's volume.

## Verification

From `tools\WinUIProfiler`:

```powershell
dotnet restore WinUIProfiler.sln --locked-mode
dotnet test WinUIProfiler.Engine.Tests\WinUIProfiler.Engine.Tests.csproj -c Release --no-restore --filter 'FullyQualifiedName~StorageTests|FullyQualifiedName~QueryTests'
dotnet test WinUIProfiler.Engine.Tests\WinUIProfiler.Engine.Tests.csproj -c Release --no-restore
dotnet build WinUIProfiler.sln -c Release --no-restore
```

Tests use real file-backed SQLite databases beneath ignored `TestResults`
and remove their own database/WAL/SHM/lease/failure siblings. Controlled SQL
triggers are test-only fault seams. No capture, processor, UI, CLI, OS collector
or other profiler is a dependency of this foundation.
`P02ReviewRegressionTests` additionally coordinates an actual WAL commit
between validation phases with a native trace callback, exercises late-time
tree/resource/tombstone dominance and root coverage for both tree APIs, and
cancels running recursive native SQL using barriers (without a timing SLA).
Separate trace-coordinated tests cancel availability/result steps; a pre-step
case verifies idle interrupts cannot lose cancellation. Tests verify native
error 9, preserved recording/history and subsequent usable reads/connections.
