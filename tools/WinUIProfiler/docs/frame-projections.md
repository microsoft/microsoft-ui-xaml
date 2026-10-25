# Frame projection (P11)

`FrameProcessor` is an owner-thread `IEventProcessor`. It composes the existing
stateless `MetricProcessor` for collector `MetricPayload` sources; do not also
register a separate metric processor. The default five state projections are
ObjectState, Frame (including metrics), XamlTree, CompositionTree, ResourceUsage.
`ProcessingWorker` and `LiveMetricRecording` use this pipeline for ordinary events
when no explicit processor list is supplied. An explicit empty list remains
input-only recording.

## Authoritative submissions and attempts

Only a validated `FrameOutcome.Submitted` reports successful primary physical
XAML submission. Attempts/ticks, layout/render boundaries, successful no-device
attempt completion, failures and standalone/debug commits do not increment the
submission count. No idle frames, timestamps, roots or attempt IDs are invented.
A submission and its later failed attempt completion remain independent axes.
Shared physical commits reported by multiple participating roots count once by
canonical process, producer epoch and submitted ID; conflicting timestamps for
one physical commit are rejected.

`FrameInfo` retains the complete epoch/root/dispatcher/attempt reference, original
submission QPC and HRESULT/terminal status. Without a terminal observation it is
InProgress, even after submission. Changes preserve exact source ingestion
ordinal, event QPC, object identity, typed observation and producer cause.
Changes are sorted by QPC then source ordinal, not arrival order.

The frame range is the **extent of observed attempt evidence**, not a summed CPU
duration. Explicit attempt changes before begin or after terminal extend this
extent without moving submission QPC or terminal result. Only the separately
observed begin-to-terminal interval (or latest observed same-epoch runtime event
for an unterminated attempt) admits unknown temporal overlaps. Extending the
extent for an explicit change does not extend that temporal interval.

ExplicitAttempt is Causal only for the exact qualified attempt. Unknown causes
remain TemporalOverlap with partial quality, never promoted by proximity.
OutsideAttempt changes are not assigned to an attempt. Unknown changes outside
all observed intervals remain unassigned, including changes after the final
terminal event; they stay in bounded retained evidence and persisted source
events, not a fabricated frame. Late begin/submission/change events can revise
previously published frames. Exact changes after the final submission are not
discarded or dependent on another submission arriving.

## Startup and phases

Only `ProcessPayload(Start, CreationQpc: non-null)` establishes the exact process
creation timeline origin. Attach's CreationQpc, recording time, and kernel
ProcessStart event time when its payload lacks exact CreationQpc do not.
`StartupUnknown` records partial/unavailable evidence at an actual observed source
timestamp. It is an evidence marker, not a launch interval; if exact creation
evidence arrives later, that historical unknown marker can coexist with the
new creation timeline.

`ProcessCreation` is a point at the actual creation QPC. `StartupFrame:01` through
`:10` are cumulative **creation-to-submission** intervals across all roots and
dispatchers of the canonical process, ordered by QPC/ordinal. They are not
per-root counters, attempt durations or inferred display presentations.
Their stable phase/creation keys permit event-time ranking revisions even after
ten submissions, without leaving an obsolete eleventh startup row.
`StartupWindow` ends at the tenth distinct physical submission, or latest
qualifying submission when fewer than ten. Its quality explicitly says N/10 and
remains partial on early target exit or ordinary unfinished capture. No remaining
frames are manufactured. Input quality, loss and multi-epoch discontinuity prevent
claiming complete first-ten coverage, even if ten submissions were observed.
Epoch evidence is scoped to the inclusive creation-to-window-end interval.
A later epoch's standalone commit or attempt/submission strictly after the tenth
submission does not downgrade that completed window or its ten cumulative ranks.
Late-arriving epoch evidence inside the window still makes coverage partial;
previously committed-watermark queries retain their original historical result.

Complete boundary pairs become `Phase:<kind>:<epoch>:<producer PID>:<thread>:
<activity>:<operation>` timeline entries, using their exact begin/end QPC.
Initialization, device creation, measure, arrange and render may overlap.
Pairing uses epoch, producer PID, thread, activity, kind and operation ID, never
a single nesting stack. Unpaired/reversed boundaries remain raw evidence, not
fabricated duration intervals. Reused boundary identities are rejected.

## Loss, bounds and persistence

Runtime loss invalidates overlapping same-epoch interval attribution; collector
loss applies across epochs. Health continuity failure conservatively invalidates
all temporal attribution. Exact producer causality and successful submissions
remain reported evidence with partial change/startup quality; loss is not repaired
by a later frame. Unrelated epochs never share attempts or temporal changes.

Relevant source facts are retained with a configurable 1–1,048,576-entry limit
(default 65,536) and a combined 16 MiB serialized-byte limit for original source,
decoded payload, indexed changes, index metadata reserves and publication
fingerprints. Derived rebuild work is bounded by
that retained evidence. No historical fact is silently evicted. Metrics are
delegated without retaining source facts or duplicate state. Reaching a retained
state bound, 4,096 changes/frame, 64 KiB projection-record bound, or the shared
`context.OutputBudget` raises `ProcessingLimitException`; the worker's established
limit path stops recording rather than dropping history.

Output is incremental: unchanged rows are fingerprinted and suppressed; changed
rows are checked and yielded one at a time against the owner's current output
budget. The owner consumes that budget and may dispose enumeration. Each revision
references a source in its current transaction; original change ordinals remain
inside FrameChanges. Source facts are not reconstructed from wall-clock time.
Tests commit through the actual SessionWriter and query/reopen actual SQLite
sessions using private `build\p11-frame-validation` artifacts.

### Indexed association and review finding 2

Attempt frame facts and exact causal observations are indexed by full
AttemptReference. Unknown observations use a per-epoch QPC/ordinal sorted index;
binary lower-bound lookup reads only observations inside a dirty frame's actual
temporal interval. Original source facts remain retained without eviction.
Only dirty attempts are rebuilt and serialized. New exact changes dirty their
matching attempt; new frame facts and late begins dirty their full qualified key.
Unknown changes visit retained attempt intervals to mark overlaps, not historical
change arrays. Advancing an epoch's latest observed runtime time dirties its open
attempts; closed attempts do not change just because unrelated runtime time
advances. Loss and global health/quality invalidation can revisit all attempts.
Startup/phase dirtiness is separate from ordinary object-change dirtiness.

This removes the repeated historical attempt-by-change Cartesian scans and avoids
constructing/serializing unchanged historical frames for ordinary OutsideAttempt
changes. Unknown-change dirty selection still scans retained interval metadata;
timeline events still rebuild the timeline, and late insertion in a sorted index
can shift entries. No general constant-time or throughput guarantee is claimed.

`Outside_attempt_change_allocation_does_not_rebuild_completed_frame_history`
is an allocation regression at 32, 128 and 512 completed
attempts, each retaining one explicitly causal change. It warms the processor,
then measures eight synchronous OutsideAttempt additions using actual per-thread
allocated bytes and logs elapsed time separately. Timing is diagnostic only;
the regression asserts a 64 KiB/event allocation ceiling, not a throughput SLA.
Setup supplies each retained attempt with its exact causal change; ordinary
unassigned additions emit no frame revisions. Before indexing, measured allocation
was 307,584 / 1,145,006 / 4,506,436 bytes per addition at these retained counts.
The first indexed Release run measured 25,040 bytes per addition at all three
counts. Exact commands, repeated raw measurements and limits of the scenario are
recorded in the implementation report; these numbers are not production SLAs.

## Existing contract gaps (no contract changes in P11)

* There is no `CausalAttribution` DTO in the current contracts. The available
  `ChangeCause`, `AttemptReference` and `FrameChangeRelation` are used exactly as
  defined. Activity IDs survive raw sources and boundary pairing but FrameInfo/
  FrameChange cannot expose an independent activity attribution or source stream.
* AttemptReference has no core/manager identity. Root, dispatcher, epoch and
  attempt are preserved; manager/core IDs must not be guessed from them.
* FrameInfo requires a non-null AttemptReference and has no standalone outcome
  or per-frame quality field. A null-attempt standalone commit is retained in
  source evidence only, not represented by a counterfeit FrameInfo. Frame quality
  is carried by query context and individual changes; an empty frame cannot carry
  a separate row-local partial marker.
* There is no unassigned-change query or phase-interval DTO. Unassigned changes,
  incomplete boundaries and standalone commits are preserved in raw source
  evidence. Paired phases use StartupEntry with a clearly distinct `Phase:` prefix;
  they are **not** counted as startup frames. StartupEntry cannot expose boundary
  HRESULT, causal attempt or source ordinal; those remain in source events and,
  where attributable, BoundaryObservation frame changes.
* Kernel ProcessStart currently provides creation FILETIME but a null CreationQpc;
  that confirms process creation evidence without an exact creation-QPC origin.
  P11 therefore reports unknown exact startup rather than substituting event time.
* Resource sizes exceeding signed query storage remain explicitly unavailable in
  ResourceObservation rather than wrapped; the exact unsigned source payload is
  retained. Boundary failure/result and full source identity likewise remain raw
  when the projected DTO lacks a field.
