# Object and XAML projections

`ObjectStateProcessor` and `XamlTreeProcessor` implement the owner-thread
`IEventProcessor` pipeline. They emit existing typed `ProjectionChange` values;
`SessionWriter` commits these with their source events, and the existing
`GetObjectHistory` / `GetXamlTree` endpoints read the durable revisions. Both
processors are in the default five-processor ordinary pipeline. They do not
open another database or issue target commands.

## Identity and time

Objects are keyed by the complete `ObjectKey`, including the process instance,
identity domain and producer epoch. An address is only an observed lifetime
attribute. Reuse of an address cannot merge histories. Type IDs are qualified by
producer epoch; conflicting reuse of an immutable type ID is an explicit error.
Classification comes from authoritative classification payloads, never a name.

History retains source QPC, ingestion ordinal, typed observations, quality and
causality. Late metadata adds a type observation; it does not rewrite a birth.
Property or membership observation does not establish birth. Capture presence
is `BaselinePresent`, not `Birth`. Detached objects remain queryable and are
not destroyed. Core/peer links are bidirectional typed correlation observations.

Tree revisions are keyed by object, relationship and membership kind. Logical
and visual memberships remain distinct. The point query selects by QPC, with
ingestion ordinal breaking ties, at the requested committed watermark. Thus a
revision at `b` ends the preceding relationship state's validity at `b`:
`[a,b)`. The typed `Relationships` table retains observation points; no mutable
end-time rewrite is made to earlier committed facts. Tests probe both sides of
these interval boundaries, including reparenting and detachment.

Destroyed objects remain in history but cannot be resurrected by late topology.
Parent destruction also ends its child relationships. Selected-root queries retain
a detached boundary row as `Connected=false`, but do not expand through it into
descendants. Destroyed intermediate relationships are excluded entirely.
Tree display metadata is resolved independently from topology at the same
QPC/watermark, so a type update cannot shift a relationship's observation time.

## Verified baseline handoff

Both processors implement:

```csharp
IEnumerable<ProjectionChange> IVerifiedBaselineProcessor.PublishBaseline(
    NormalizedEvent validatedEnd,
    IReadOnlyList<CaptureChunkPayload> chunks,
    ProcessingContext context);
```

Ordinary `Process` ignores raw capture chunks and boundaries. Only the
coordinator may call `PublishBaseline`, on the state-owner thread, after
validating the entire capture: Begin/chunk/End identity and scopes, chunk
completeness, per-root/dispatcher cuts, delivery continuity and reconciliation
with concurrent live deltas. The supplied chunks must be owned, stable,
reconciled staged state, not an unverified traversal.

The handoff defensively checks End identity/scope, chunk ordering/count,
qualified payloads, bounded size, duplicate objects/memberships, dangling
parents and membership cycles **before yielding**. It does not replace the
coordinator's capture/continuity validation.

Every candidate uses the validated End's actual QPC and source ordinal, and End
must be a source in the current writer transaction. The prior chunks are not
repersisted, and storage does not require source event 14 for history or trees.
Enumerate candidates incrementally through the owner's normal output accounting.
Publish each End once per processor.

A complete, verified tree scope with complete tree-domain coverage can tombstone
previously observed memberships absent from that scope. A partial capture cannot
infer absence. Neither case infers object destruction. Capture quality is
preserved, and historical lifetime coverage stays partial; neither a successful
End nor a type/property delta fabricates a complete lifetime baseline.

## Loss and resource limits

Reported loss intervals are read from the durable source events at the selected
watermark. They degrade membership certainty from the actual interval start,
including late-delivered loss reports, until a fresh observation after the gap.
Runtime gaps remain producer-epoch qualified; collector transport loss is
global. Irrelevant runtime delivery domains do not invalidate XAML membership.

The worker calls `Invalidate` before processing the containing batch. Explicit
producer loss is therefore not stamped onto the batch's first, potentially
earlier event. Generic health invalidation makes existing memberships uncertain
on the next real source without inventing detachment or a clock. Persisted
diagnostics cover terminal invalidation with no subsequent source; a diagnostic
without a timestamp remains conservatively unplaced/uncertain.

Each processor defaults to 65,536 retained index entries and a 16 MiB accounted
state-payload bound. Constructor limits can reduce the entry bound. Object/type
metadata and relationship tombstones are never silently evicted: exhausting a
bound throws the existing processing-limit signal, and the worker stops at its
prior durable boundary. Indexes do not contain full event histories.

Candidates honor the existing incremental record/byte budget (64 KiB per
candidate, at most 4,096 changes per writer transaction). Verified baseline
input is bounded to 4,096 chunks and 16 MiB. Query topology, metadata, loss and
diagnostic materialization share the existing 16 MiB query budget; exhausting it
is an explicit query error, not an apparently successful empty tree.

## Integration gates

The default worker now invokes all five processors for ordinary events. It does
not yet route verified End publication: the parent pipeline must supply reconciled
staged state and invoke owner-side preparation into owned immutable pages for
the atomic End transaction. `BaselinePagePreparer` now accepts that exact
five-processor set: verified processors publish the staged baseline, while Frame
processes the singleton actual End normally. Retained `FrameInfo`/`StartupEntry`
quality or invalidation revisions share the aggregate limits and atomic commit
with the baseline rows; their historical facts are not restamped as End facts.
`MetricSample` is still not a baseline-End row. Preparation is single-attempt,
not publication authority; failure requires discarding all involved processor
state. Live TAP/P08 qualification, concurrent-delta
reconciliation and owner/writer baseline integration remain independent gates.
