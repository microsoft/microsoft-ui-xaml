# Canonical event and TAP contract, schema 1

This is a requirement-first specification for **new** producers. No providers
are implemented in P01 and no currently shipped WinUI events are assumed to
satisfy it. The C# contracts and catalog are the executable schema.

## Fixed producer identities

| Producer | New fixed GUID | Event version | Levels |
|---|---|---|---|
| `WinUIProfiler-Runtime` | `43a73524-bc37-4f6e-9ba8-84b5346f61c2` | 1 | 4 Informational; Loss = 3 Warning |
| `WinUIProfiler-Collector` | `b58fa3ac-f7e8-4a24-88cd-478ab589de21` | 1 | 4 Informational; Loss = 3 Warning |

Keywords (64-bit masks): Capabilities `0x001`, Objects `0x002`, XamlTree
`0x004`, Composition `0x008`, Frames `0x010`, Resources `0x020`, Allocator
`0x040`, Capture `0x080`, Metrics `0x100`. Each event has exactly the keyword
specified by `EventCatalog`. This corrected initial schema 1 supersedes the
unreleased P01 draft; no released compatibility is claimed. After publication,
adding/changing a field requires a new event version and explicit support;
never reinterpret published bytes.

## Common identities and envelope

All identifiers are explicit, nonzero lifetime IDs/nonempty GUIDs unless
nullable. `ProcessInstanceKey` = `ProcessId:u32`,
`CreationTimeFileTime:i64?`, `InstanceId:GUID`. Creation time is positive UTC
FILETIME (100 ns since 1601), or null. `ObjectKey` = full process key,
`Domain:enum`, `ProducerEpoch:GUID`, `RuntimeObjectId:u64`. `Domain` names:
XamlCore, XamlPeer, CompNode, CompositionVisual, Resource.

Runtime/collector ETW wire schemas must include an authoritative
`ProducerEpoch:GUID` and `SourceSequence:u64` common prefix with semantic
target process identity. The adapter binds the prefix to
`ProducerStreamKey(ProviderId, ProducerProcessId, ProducerEpoch)`; the ETW
header PID/TID is the source, not a substitute for target identity.
Runtime capabilities must precede use of the epoch. Device/allocator epochs
are separately scoped, not aliases for producer epoch.

The normalized `EventEnvelope` has:

| Field | Type / meaning / units |
|---|---|
| SessionId | GUID, selected local profiling session |
| Process | full semantic target `ProcessInstanceKey` |
| ProducerProcessId, ThreadId | u32, actual emitting process/thread; nonzero |
| TimestampQpc | i64 >= 0, session QPC ticks |
| IngestOrdinal | i64 > 0, session total ingestion order |
| SourceSequence | u64? > 0 within the delivery-homogeneous sequence domain below; null if unavailable |
| ActivityId | GUID? correlation metadata, not object identity |
| Identity | ProviderId GUID, Name UTF-8 string, EventId u16 > 0, Version u8 > 0, Keywords u64 |
| OwnedPayload | owned bytes; canonical UTF-8 JSON typed below, maximum 1,048,576 bytes |

The normalizer never retains recycled ETW buffers. Constructor, capture and
decode boundaries copy bytes; payload access returns a copy. Serialized
envelopes encode bytes as base64. Maximum encoded envelope input is 2,097,152
bytes; JSON nesting depth is 32. Canonical properties are camelCase and enums
are strings; required nullable fields must be present with `null` when
unknown. Duplicate/unknown properties, numeric/unknown enums, null required
objects and unsupported schemas are rejected. Source timestamps need not be
monotonic in arrival order; ordinals are.

The ETW producer implementation will use typed primitive/struct/array fields,
not arbitrary caller JSON. Flatten nested identities consistently with their
declared field order. The adapter converts these typed fields into canonical
JSON. ETW transport events must remain below ETW's event-size limit: split
capture chunks to a conservative 48 KiB wire payload maximum. A canonical
normalized payload may be larger due to expansion; the 1 MiB ingestion limit
does not authorize 1 MiB ETW writes.

## Reusable value and quality schemas

`DataQuality` = `Kind:Measured|Estimated|Unavailable`,
`Coverage:Complete|Partial|Unavailable`, `Reason:string?`. The axes are
independent. Any non-measured or non-complete observation requires a
nonblank reason.

`TypedValue` = `Kind:Integer|Number|Boolean|Text|Missing`, `Unit`,
`Integer:i64?`, `Number:finite f64?`, `Boolean:bool?`, `Text:string?`,
`Quality:DataQuality`. Exactly one matching value is present, or none for
Missing with unavailable quality. Units: Count, Bytes, Percent, QpcTicks,
Pixels, Boolean, Scalar, String. Numeric fields cannot masquerade as
Boolean/String units. Signed byte deltas are permitted in snapshot comparisons.
Metric samples themselves must be nonnegative; percentages are 0–100.

`TimeRange` is `[StartQpc:i64, EndQpc:i64)` in session ticks with
`0 <= Start <= End`. Convert durations with the session frequency using
checked wide arithmetic; zero/negative frequency and overflow fail.

`AttemptReference` = ProducerEpoch GUID, AttemptId u64 > 0, RootId GUID,
DispatcherId u64 > 0. `ChangeCause` = Kind Unknown/OutsideAttempt/
ExplicitAttempt, Attempt reference?, Reason string?. Unknown requires a
reason; outside/unknown cannot carry an attempt. Explicit requires the full
reference, matching the observation's authoritative runtime epoch.
An omitted optional event Cause is unknown, not OutsideAttempt. An
ActivityId or temporal overlap alone does not establish causation.

`XamlTypeClassification` = Kind Unknown/DependencyObject/UIElement/
NotApplicable, Evidence Unavailable/RuntimeType/RuntimeBridge, Source string.
Unknown requires Unavailable evidence; known classification requires
authoritative evidence. Null classification means unknown. UIElement or
DependencyObject classification cannot label composition/resource identities;
XamlCore cannot claim NotApplicable. Display names are not evidence.

`ResourceClassification` = Kind Unknown/Texture2D/CompositionSurface/
CompositionBrush/Bitmap/RenderTarget/Buffer/Geometry/Other, Evidence
Unavailable/CreationApi/RuntimeType/RuntimeBridge, Source string. Known kind
requires authoritative evidence; unknown is explicit with unavailable
evidence and source/reason, never inferred from optional type text.

## Runtime events

Field order/types below follow the named payload record constructors.
All strings are UTF-8; counts and IDs are dimensionless. `?` means honest
nullable data, never a magic zero.

| ID / name | Keyword | Typed payload and units |
|---|---|---|
| 1 RuntimeCapabilities | Capabilities | Epoch GUID, Build string, Architecture X64/Arm64, Process key, QpcFrequency i64 ticks/second, Capabilities array |
| 2 ObjectLifetime | Objects | Object key, Operation Birth/Destroy/BaselinePresent, TypeId u64?, Address u64? bytes-address metadata, Quality, Classification?, Cause? |
| 3 TypeResolved | Objects | ProducerEpoch GUID, TypeId u64, Name string, Classification? |
| 4 CorePeerAssociation | Objects | Core key (XamlCore), Peer key (XamlPeer), Linked bool, Cause? |
| 5 TreeEdge | XamlTree | Parent key?, Child key, Relationship enum, Order i32 >= 0, RootId GUID, DispatcherId u64, Connected bool, Cause?, Membership Unknown/Logical/Visual/Composition |
| 6 VisualDelta | Composition | Visual key (CompositionVisual), Property enum, Value TypedValue, Cause? |
| 7 Correlation | Composition | From key, To key, Kind enum, Linked bool, Cause? |
| 8 Boundary | Frames | Kind enum, Phase Begin/End, OperationId u64, HResult i32?, Cause? |
| 9 Frame | Frames | Attempt reference?, SubmittedFrameId u64?, Outcome enum, HResult i32? |
| 10 Device | Resources | DeviceEpoch GUID, Operation Create/Lost/Destroy, Api string, Quality |
| 11 Resource | Resources | Resource key, Operation enum, DeviceEpoch GUID, BackingAllocationId GUID?, AllocationBytes u64?, SizeProvenance enum, Owner key?, WidthPixels u32?, HeightPixels u32?, Quality, Classification, Cause? |
| 12 Allocation | Allocator | AllocationId u64, Operation Allocate/Free/Reallocate, PreviousAllocationId u64?, RequestedBytes u64, ActualBytes u64?, AllocatorEpoch GUID, BaselineCaptureId GUID?, Quality, Cause? |
| 13 CaptureBoundary | Capture | CaptureId GUID, Kind Baseline/LightweightSnapshot/OwnershipGraph, Phase Begin/End, RootId GUID?, DispatcherId u64?, Domains SequenceDomainRange[], ChunkCount u32?, Quality |
| 14 CaptureChunk | Capture | CaptureId GUID, ChunkIndex u32 (zero-based), RootId GUID, DispatcherId u64, typed arrays described below |
| 15 Loss | Capabilities | ProducerEpoch GUID, Interval TimeRange, LostEvents u64? (positive if known), Reason string, Domains SequenceDomainKey[]? |
| 16 XamlPropertyChanged | Objects | Object key (XamlCore), Property XamlPropertyKey, Previous TypedValue, Current TypedValue, Cause? |

Capabilities are unique bounded arrays (maximum 128) of
`CapabilityInfo(Capability enum, Version i32 > 0, Quality)`. Capabilities include
object lifetime, XAML/composition topology, resource backing, traced allocator,
lightweight/ownership/native/managed snapshot and CPU/GPU/process-memory
observation. Each capability can be unavailable independently.

### Object, tree and composition semantics

Type IDs are scoped to producer epoch; ObjectLifetime TypeId uses the object's
epoch. BaselinePresent establishes current presence, **not historical birth**.
Destruction is emitted only from authoritative final lifetime termination.
Address zero is invalid metadata; use null when unavailable.

`XamlPropertyKey` contains authoritative PropertyId u64, DeclaringType and Name
strings. Previous/current retain the same declared unit, including explicit
Missing when the old value was not observed. Property changes are not fake
lifetime events. Typed history can preserve lifetime/type, property,
membership, visual, resource, correlation, allocation and boundary
observations; frame changes include QPC/ordinal, optional object, quality,
cause and Causal versus TemporalOverlap relation. Causal query rows must
match the exact frame attempt; temporal rows make no such causal claim.

Tree relationships: Root/Child/Content/Template/Popup for XamlCore,
CompRoot/CompChild for CompNode, VisualRoot/VisualChild for CompositionVisual.
Root relationships have no parent; other relationships require one.
Parent and child share process/domain, cannot be equal, and order is explicit.
Connected=false removes membership; it is never a destruction event.
Membership is independently Logical/Visual/Composition/Unknown. XAML edges
cannot claim Composition membership, and composition edges cannot claim
logical/visual XAML membership. Count distinct XamlCore UIElements with
authoritative type and visual-connection evidence for the default connected
element count; exclude arbitrary DependencyObjects, peers and logical-only
connections. Unknown evidence yields incomplete coverage, not inferred type.
Reparent/reorder records terminate the old relationship and establish the new
one in source order. Popup roots and template/content relationships must retain
their relationship kind, root and dispatcher instead of flattening into
children. The later processor applies many-to-many correlations, not a 1:1 map.

Visual properties: OffsetX/OffsetY/Width/Height in Pixels, Opacity (0–1) and
TransformM11/M12/M21/M22 in Scalar, IsVisible in Boolean. Missing values
remain explicitly missing. This version is a declared property subset, not
a claim to reproduce arbitrary compositor state or animations.

Correlation directions and required domains:
CorePeer (XamlCore -> XamlPeer), XamlCompNode (XamlCore -> CompNode),
CompNodeVisual (CompNode -> CompositionVisual), XamlVisual (XamlCore ->
CompositionVisual), VisualResource (CompositionVisual -> Resource),
ResourceBacking (Resource -> Resource, for resource alias relationships).
The actual unique backing GUID is in Resource, not invented as an ObjectKey
or used to double-count shared storage.

### Boundaries and frames

Boundary kinds: Initialization, DeviceCreation, LayoutMeasure, LayoutArrange,
Render. Begin has no result; End carries success/failure when known.
Pair OperationId within the authoritative source epoch/thread/activity.

Frame outcomes: Attempt, Submitted, Failed, StandaloneCommit,
AttemptCompleted. Every non-standalone observation requires the full
epoch/root/dispatcher-qualified AttemptReference. Standalone commits may use
null Attempt, not a fabricated zero ID. Only Submitted has a nonzero
SubmittedFrameId. Submitted requires success; Failed requires a failing
HRESULT. Failed/AttemptCompleted are attempt terminal observations separate
from successful submission. FrameInfo represents them on independent axes.

The qualifying hook is the primary actual device Commit in
`SubmitPrimitiveCompositionCommands`, **not** success of that outer function:
after primary commit there is fallible post-commit/layout work and additional
debug-counter commits. `CommitMainDevice` returning S_OK with no
`m_spMainDevice` is a no-op, not a submission. Emit the submission at actual
primary commit success; later failure cannot erase it. Standalone/debug
commits never increment submitted counts. Submitted IDs are total across
roots/dispatchers for the stream, not a per-root startup counter. Startup
counts ten total and never forces idle frames, ticks or presentations.

### Resources and allocation

Resource operations: Create, Destroy, OwnershipRelease, Close, Rebind,
Resize, Share. Rebind/resize close the previous backing association and open
the next at the observed time. A shared BackingAllocationId identifies unique
storage within device lifetime, including aliases across owners/resources.
OwnershipRelease/Close do not prove final external COM destruction.

Size provenance: ActualAllocation requires known backing GUID and bytes;
ApiDescriptionEstimate requires bytes and Estimated measurement kind;
Unknown requires null bytes. ActualAllocation requires Measured kind.
Coverage remains independent: estimates may have Complete or Partial
coverage with an honest scope/reason. Texture dimensions alone
cannot establish actual residency/allocation size. Device loss terminates
the relevant device epoch and incomplete observations remain partial.

Allocator IDs are explicit lifetimes scoped to AllocatorEpoch. Reallocate
requires a positive predecessor and a new lifetime ID; non-realloc events
have no predecessor. Requested and actual bytes are separate; unknown actual
is null and known actual cannot be less than requested. Free terminates the
allocation, not every object that ever used it. BaselineCaptureId/quality
make pre-enable coverage explicit; a traced allocator is not the entire
native heap.

### Capture and loss

CaptureBoundary scopes a global or per-root/dispatcher capture. Begin/end
retain sequence-domain checkpoint vectors and End reports chunk count. An End with
complete coverage is a claim to be validated against all chunks and loss by
the future processor, not proof merely from receipt of End. Root/dispatcher
captures are not implicitly atomic across the process.

CaptureChunk arrays:

* Objects: `SnapshotObject(Key, TypeName:string?, Address:u64?, Size:TypedValue, Classification?)`.
  Size is nonnegative numeric Bytes or explicit Missing/Unavailable.
* Ownership: `OwnershipEdge(From, To, Relationship:string, Evidence, Quality)`.
* Tree: TreeEdgePayload; Correlations: CorrelationPayload.
* Resources: ResourcePayload; Visuals: VisualDeltaPayload.
* Associations: AssociationPayload; Types: TypeResolvedPayload.
* Allocations: AllocationPayload.

Arrays are required, empty when no entries, never null. Each list is bounded
to 4096 items (Ownership 8192), additionally constrained by the byte budget.
Nested Tree entries must match the containing chunk's RootId/DispatcherId;
single-scope chunks cannot smuggle another scope's topology.
Chunks must be split well before the ETW limit. Baseline is authoritative
only for the explicitly covered roots/dispatchers/capabilities. Missing
chunks, proven delivery-domain gaps, decoder/ETW loss or changed epochs
invalidate affected completeness. Filtering or unknown listener continuity
is unproven coverage, not evidence of a missing producer observation.
Preserve previously committed history and explicit gap intervals;
do not pretend a new baseline reconstructs pre-attach births.

### Filtering-aware sequence and capture-vector specification

`SequenceDomainKey` = Stream(ProviderId, ProducerProcessId, ProducerEpoch),
Target ProcessInstanceKey, EventId u16, EventVersion u8. The fixed event
definition determines keyword/level; domain counters do not interleave event
definitions or target instances. Runtime source PID equals target PID;
collector source PID remains independent.

Native implementation is later, but must implement these semantics:

* Each domain starts at one in a fresh producer epoch. While at least one
  listener enables that entire definition/target domain, reserve/increment
  exactly once immediately before each emission attempt, including failed
  writes. A failed write therefore leaves independently detectable sequence
  evidence. Disabled domains do not perform expensive payload work.
* Counters are shared producer observations, not per-listener counters.
  Other listeners changing keywords/levels do not reset or reinterpret a
  counter or an existing listener's continuity. Overflow requires a visible
  stop/fresh epoch, never wrap.
* Each listener records a local ListenerId and per-domain ContinuityId with
  half-open enabled intervals and quality. Disabling/re-enabling that domain,
  changing its delivery filter, unobserved enable history or payload/root
  filtering breaks its proven segment. Changing a different domain does not.
  Only whole-domain, continuously enabled delivery supports complete
  continuity unless additional filtering is explicitly accounted for.
* Enable/filter continuity is necessary but **not delivery closure**. Reservation
  5 or 6 can still be in flight when 7 arrives, even with increasing QPC.
  Arrival-pair assessment is always Unproven; it cannot retract an earlier
  definitive loss claim when a delayed event arrives. A source epoch change
  creates a different domain, not an inferred loss count.
* Timestamp/ingest-ordinal merge order and explicit causal attempt identity
  are separate from loss sequences. Concurrent listeners selecting Frames
  versus Objects see independent counters, not false gaps from filtered IDs.
  Saved `SessionInfo.ListenerIntervals` retains typed coverage evidence.
* Every capture lists its relevant domains with
  `SequenceDomainRange(Domain, BeforeSequence, ThroughSequence, Quality)`.
  Before is the counter immediately before Begin emission; Through is the
  counter immediately before End emission (including any earlier capture
  events in their own domains). Zero means no emissions yet, null means
  unknown. Begin may have null Through; a complete End requires both.
  These are reservation checkpoints, not settled cuts. Entries also carry
  nullable `SettledCut`, defined below. A complete producer End range requires
  a settled prefix covering Through; outstanding reservations require partial
  coverage, not a fabricated settled watermark. Even a fully settled End is
  not listener delivery/reordering closure.
  Entries are unique, bounded to 256, and source/target-bound by Normalize.
  A complete capture needs a nonempty vector; the future processor must also
  verify that the vector covers every relevant capability/domain.
* Loss optionally identifies affected domains; null means unlocalized loss.
  ETW loss, decoder loss and capture chunk counts remain independent evidence.
  No unconditional provider-wide-gap rule is valid under keyword filtering.

### Settled emission, positive live receipt and finalized absence

`ProducerSequenceCut(CutId, Domain, ReservedThrough, SettledThrough)` is an
authoritative immutable producer acknowledgement. ReservedThrough is the
largest reservation at that cut. SettledThrough is the largest **contiguous
prefix** for which every emission attempt has returned a terminal result,
including failed writes; it cannot exceed ReservedThrough. Completion of 7
alone does not settle delayed reservations 5/6. Zero is the empty prefix.
CutId identifies those exact acknowledged facts, not just a sampled counter.
The cut does not imply event arrival, decoding, or causal mutation order.

`SequenceReceiptEvidence(ListenerId, ContinuityId, SettledCutId,
EnabledAfterSequence, EnabledThroughSequence, ProcessedThroughOrdinal)` is
independent of any sealed source. It binds the same domain-qualified cut and
listener segment to measured eligibility for every attempt in the enabled
exclusive/inclusive sequence span. ProcessedThroughOrdinal is the contiguous
session prefix that has completed relevant normalization/decoding and consuming
processing, not the largest callback or assigned ordinal. Zero honestly means
no completed records. A checkpoint counted as positive receipt must be a
successfully normalized observation whose relevant work has completed by that
ordinal; an explicitly accounted decoder loss is not a successful checkpoint.
Capture/replay owners must establish those facts, not manufacture them from
counter maxima or raw callbacks.

For `(AfterSequence, ThroughSequence]`, a covering settled cut plus **every
distinct expected ID** with completed receipt proves Contiguous with zero
missing observations while recording continues. It needs no file, final
enumeration, transport stop or proof that no further event can arrive.
Domain/listener/continuity, quality, QPC bounds, enabled sequence coverage and
cut binding are still validated. Duplicate delivery cannot replace a missing
ID; a later duplicate with pending work does not invalidate an already
processed instance. Reservations above the assessed settled prefix do not
block it.

An honestly empty expected span needs no observation, invented processed
ordinal or archive. It still requires the covering cut, measured eligibility
and bound receipt metadata. A zero-duration eligibility interval is sufficient
only for an empty span with no checkpoints and exactly matching empty enabled
sequence bounds. This proves no nonempty interval and does not fill unavailable
data with zero or reconstruct unobserved history.
An incomplete live set or pending relevant processing stays Unproven with no
fabricated lost count, until receipt completes or the stronger absence proof
below is available.

`SequenceDeliveryClosure` carries listener/continuity IDs,
FinalizedAfterCutId, a local immutable SealedSource artifact path/SHA256,
FinalByteLength, EnumeratedByteLength, nullable FinalIngestOrdinal,
DecodedThroughOrdinal, and EnabledAfterSequence/EnabledThroughSequence.
It describes a concrete finite-source reconciliation, not a boolean
"flushed" flag. Its acquisition contract is:

1. While the listener's delivery-homogeneous domain remains continuously
   enabled with unchanged filters, establish an authoritative lower reservation
   bound and obtain a producer settled-prefix acknowledgement covering the
   desired upper reservation bound. The enabled sequence span is exclusive
   at its lower bound and inclusive at its upper bound. It certifies delivery
   eligibility for **every attempt** in that span, not merely the QPCs of
   observations that happened to arrive. Other listeners cannot change it.
2. Only **after** that settled-cut acknowledgement, finalize the finite
   capture transport using a verified terminal-completion guarantee: every
   previously accepted write through the cut has been materialized in that
   source or permanently lost to this finalized capture, with none still
   pending outside it. A local manifest must bind the acknowledged cut's exact
   contents/ID, listener/continuity and finalized source length/digest.
   Captures of other domains or targets do not establish this binding.
   P01 does not implement or assert an ETW API guarantee. If an adapter cannot
   substantiate this guarantee, it cannot supply authoritative closure.
3. Verify that source's seal/length/digest, enumerate all bytes to its known
   end and assign the final ingest ordinal. FinalIngestOrdinal is null until
   known; DecodedThroughOrdinal is the contiguous **completed** decode prefix,
   not the largest record seen. Wait for every assigned record through the
   final ordinal to be normalized or explicitly accounted as a decoder loss.
   No queued decode/reordering work may remain below that prefix.
4. Supply **all** observed checkpoints for the selected listener/domain/
   continuity and assessed sequence window, independent of arrival order.
   The helper cannot read/verify an artifact or discover withheld observations:
   upstream capture/replay owners must establish these provenance and
   exhaustive-enumeration obligations before claiming the evidence.

No timestamp, timeout, capture End event, maximum sequence received,
`FlushTrace` return, or EOF of a still-growing/unbound file alone satisfies
these steps. In particular, sealing a file before lower producer reservations
settle cannot turn the missing writes into a valid closed cut. The supported
**absence** proof is finite-source finalization **after** settled emissions,
followed by exhaustive enumeration and completed decoding; an ordinary live ETW callback
does not provide it. Future native/collector work must implement/validate the
guarantees or leave absent IDs Unproven. This stronger negative proof is not
required for a positively received settled live prefix. There is no native
sequencer, drain, worker, archive verifier or transport implementation in P01.

The window overload of `SequenceContinuity.Assess` accepts
`SequenceObservationWindow(AfterSequence, ThroughSequence, Observations)`,
the listener interval and optional cut, closure and independent receipt
evidence. The assessed span is `(AfterSequence, ThroughSequence]`; an observed
lower anchor is optional.
Checkpoints include QPC and positive session IngestOrdinal. Windows contain
at most 8192 checkpoints and can partition a large capture using the same
adequate receipt or finalized proof; they must not silently truncate observations.
Repeated delivery is counted once, not as a replacement for a missing ID.

The helper validates domain/listener/cut binding, counters, processing progress
and enabled sequence coverage; supplied closure metadata also has its artifact
syntax and byte/ordinal bounds checked. Positive live receipt uses the distinct
processed set. Finalized absence uses the exhaustive closed-source set.
Incomplete settlement, filtering or continuity remains Unproven. Missing IDs
or incomplete relevant live processing remain Unproven without adequate
terminal evidence, rather than acquiring a premature loss count:

* Arrival 4,7,5,6 is Contiguous as soon as the settled span and completed
  receipt are proven, regardless of arrival/QPC ordering and without a sealed
  source. Until then it is provisional, never premature permanent loss.
* A genuinely closed window `(4,7]` containing only 7 has two missing
  observations. A missing tail is detectable even if no higher event arrived.
* Reservations beyond the assessed settled prefix can remain outstanding;
  they cannot justify extending that window's completeness.

Listener interval quality describes measured enable/filter eligibility,
not a promise of loss-free transport. Proven producer gaps remain independent
of ETW/decoder loss reports. A live baseline can complete while capture
continues once all required settled domain spans have positive completed
receipt, required Begin/End and chunks are validated/processed with matching
IDs/counts/epochs/coverage, and actual scope/consistency evidence is reconciled
under the same committed view. This requires neither a sealed artifact nor
terminal delivery closure when no expected ID is absent. Closure can finalize
genuine gaps for a partial/failed capture, but cannot make lost state complete.
Neither End alone, a maximum received ID nor a complete producer range proves
a baseline. Do not ignore decoder loss, unavailable capabilities, mismatched
chunks or non-atomic root/dispatcher scope. Preserve provisional history until
positive reconciliation or explicit failure/loss is finalized.

## Collector events

| ID / name | Keyword | Typed payload |
|---|---|---|
| 1 Process | Capabilities | Operation Start/Exit/Attach, CreationQpc i64? session ticks, Quality |
| 2 Metric | Metrics | Metric enum, Value TypedValue, Interval TimeRange, Source string, SourceVersion string, Gpu GpuSampleIdentity? |
| 3 Loss | Capabilities | Same LossPayload fields as runtime, collector producer epoch |

Collector semantic process is the selected instance even though the emitting
PID is the profiler. Attach with unknown creation QPC uses null. Collection
source/version/interval are mandatory; missing GPU support does not erase
CPU or memory samples.

Metrics/units: CpuUtilization and GpuUtilization in Percent; ProcessPrivateWorkingSet,
ProcessPrivateCommit, ProcessWorkingSet, XamlBytes, GpuDedicatedCommittedBytes,
GpuSharedCommittedBytes, GpuDedicatedResidentBytes and GpuSharedResidentBytes in Bytes;
ConnectedXamlElements and CanonicalCompositionVisuals in Count.
Private working set excludes shared resident pages; private commit is not
residency. XamlBytes is scoped traced memory with explicit baseline coverage.
Connected XAML UIElements are not all live DependencyObjects; canonical
IVisuals are not CompNodes, wrappers, or correlated-XAML-element counts.

`GpuSampleIdentity` contains Adapter `PhysicalAdapterKey?` (LuidLowPart u32,
LuidHighPart i32, PhysicalAdapterIndex u32), Scope PerEngine/
OverallBusiestEngine/PerAdapterMemory and Engine `GpuEngineKey?` (EngineId u32,
Class Unknown/ThreeD/Copy/Compute/VideoDecode/VideoEncode/VideoProcessing/
Other). Engine zero is valid. Unknown adapter is null, never a zero LUID.
Per-engine utilization requires adapter+engine; overall busiest-engine
utilization is separately labelled and may retain the winning engine.
An unavailable winning adapter makes coverage partial. Memory requires
adapter and no engine; commitment/residency and dedicated/shared are
distinct kinds, not display strings. Nonmissing GPU values require typed
scope; Missing GPU values may lack it. Non-GPU samples cannot claim GPU
identity. Collectors/capability gating and busiest-engine aggregation are not
implemented in P01.

## Snapshot and committed query publication

`SnapshotMetadata` represents Pending/Capturing/Complete/Partial/Failed/
Cancelled attempts. `SnapshotArtifact(RelativePath, Sha256)` is optional;
only actual artifacts need validated local relative paths and 64 hexadecimal
digest characters. Pending/capturing cannot claim a completed extent or sealed
artifact. Failed/cancelled require a reason, not a fabricated path/digest.
Complete/Partial require artifact, end, epoch and source version. Nullable
unknown source metadata remains null, never an empty version.

`CaptureExtent` records start/nullable end, CaptureMechanism, explicit
NonAtomic/PerScopeAtomic/ProcessAtomic consistency, evidence and
CaptureScopeBoundary arrays. Scopes are process, root+dispatcher, dispatcher,
or named heap; actual intervals fit the overall extent. Atomic claims need
evidence; runtime traversal/polling cannot claim process-atomic capture.

`SnapshotComparisonKey` distinguishes attribute and grouping (session,
object, type, root, resource kind), with exactly the applicable qualifier.
`SnapshotDelta` keeps baseline/current/delta, Compatibility and reason.
Compatible values use attribute units; signed deltas remain valid. Missing or
incompatible attributes preserve their available values and explicit Missing
delta, not zero. Counts retain integer discriminators.

Every snapshot scope interval is checked against the committed result QPC,
including completed scopes inside an artifact-free Capturing attempt with
no overall end. An unfinished extent cannot publish future scope evidence.

All typed `QueryPage`/`QueryResult` constructors validate supported row
identities, units/discriminators/enums/timestamps/ordinals/source/provenance
against their committed context and freeze nested collections. This includes
retention edges, frame changes, composition property lists and session
producer/capability/listener metadata. Caller array mutation cannot change a
published result. `GetCompositionTree` returns scoped topology plus typed
observed properties; resource and metric queries preserve their classification
and hardware identities. There is no P01 store, query service implementation,
collector or counting/projection processor.

## TAP pipe message schema

All multibyte integer header fields are little-endian. GUID bytes use RFC 4122
network order, independent of .NET's legacy mixed-endian representation.
One message consists of this **28-byte header** followed by exactly BodyLength
UTF-8 JSON bytes:

| Offset | Type | Meaning |
|---|---|---|
| 0 | u32 | BodyLength, 1–1,048,576; excludes header |
| 4 | u16 | Protocol version = 1 |
| 6 | u16 | Message type below |
| 8 | 16 bytes | Nonempty request GUID |
| 24 | u32 | Body schema version = 1 |

No trailing bytes, truncated message, oversized allocation, unknown type,
unsupported protocol/schema or mismatched typed body is accepted. The future
stream reader must bound the length **before allocation** and assemble exactly
one message before calling `TapCodec`. Depth/required/enum/UTF-8/property
validation is the same as event JSON. Request IDs correlate replies and
cancellation; retries must not create duplicate capture side effects.

| Type | Direction | Typed body |
|---|---|---|
| 1 Handshake | host -> TAP | SessionId GUID, Process key, SchemaVersion i32 = 1, NonceBase64 string exactly 32 decoded bytes |
| 2 Capabilities | host -> TAP | SchemaVersion i32 = 1 (`CapabilitiesRequestBody`) |
| 3 Rundown | host -> TAP | CaptureId GUID, RootId GUID?, DispatcherId u64? |
| 4 LightweightSnapshot | host -> TAP | CaptureId GUID, RootId GUID?, DispatcherId u64? |
| 5 OwnershipGraph | host -> TAP | CaptureId GUID, MaximumObjects i32 1–1,000,000, MaximumEdges i32 1–4,000,000 |
| 6 Cancel | host -> TAP | TargetRequestId GUID |
| 7 Disconnect | either | Reason nonblank string |
| 8 Acknowledgement | TAP -> host | Status enum, CaptureId GUID?, Reason string? |
| 9 CapabilitiesResult | TAP -> host | Epoch GUID, SchemaVersion i32 = 1, Capabilities array |

Statuses: Accepted, Completed, Unsupported, Rejected, Cancelled, Failed.
Unsupported/Rejected/Failed require a reason. Accepting a capture is not
completion; the matching ETW capture ID/boundary/chunks and committed coverage
establish its result. Full heap snapshots are separate capabilities/collectors,
not silently substituted for the lightweight command.

P08 must create an ACL-restricted local pipe for the same authorized user,
verify peer process instance/token, and authenticate the fresh cryptographic
nonce delivered by the trusted injection channel before enabling commands.
A nonce parsed successfully by this codec is **not authentication**.
Reconnect/reinjection establishes a fresh epoch and cancels unfinished
commands visibly. Validate live session/capability before command dispatch;
offline ETL may view saved snapshot artifacts but may not communicate with a
target or request new snapshots. P01 provides no pipe or command executor.
