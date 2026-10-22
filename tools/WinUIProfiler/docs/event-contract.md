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
specified by `EventCatalog`. Adding/changing a field requires a new event
version and explicit support; never reinterpret v1 bytes.

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
| SourceSequence | u64? > 0 within authoritative producer stream; null if unavailable |
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

## Runtime events

Field order/types below follow the named payload record constructors.
All strings are UTF-8; counts and IDs are dimensionless. `?` means honest
nullable data, never a magic zero.

| ID / name | Keyword | Typed payload and units |
|---|---|---|
| 1 RuntimeCapabilities | Capabilities | Epoch GUID, Build string, Architecture X64/Arm64, Process key, QpcFrequency i64 ticks/second, Capabilities array |
| 2 ObjectLifetime | Objects | Object key, Operation Birth/Destroy/BaselinePresent, TypeId u64?, Address u64? bytes-address metadata, Quality |
| 3 TypeResolved | Objects | ProducerEpoch GUID, TypeId u64, Name string |
| 4 CorePeerAssociation | Objects | Core key (XamlCore), Peer key (XamlPeer), Linked bool |
| 5 TreeEdge | XamlTree | Parent key?, Child key, Relationship enum, Order i32 >= 0, RootId GUID, DispatcherId u64, Connected bool |
| 6 VisualDelta | Composition | Visual key (CompositionVisual), Property enum, Value TypedValue |
| 7 Correlation | Composition | From key, To key, Kind enum, Linked bool |
| 8 Boundary | Frames | Kind enum, Phase Begin/End, OperationId u64, HResult i32? |
| 9 Frame | Frames | AttemptId u64, SubmittedFrameId u64?, Outcome enum, DispatcherId u64, RootId GUID, HResult i32? |
| 10 Device | Resources | DeviceEpoch GUID, Operation Create/Lost/Destroy, Api string, Quality |
| 11 Resource | Resources | Resource key, Operation enum, DeviceEpoch GUID, BackingAllocationId GUID?, AllocationBytes u64?, SizeProvenance enum, Owner key?, WidthPixels u32?, HeightPixels u32?, Quality |
| 12 Allocation | Allocator | AllocationId u64, Operation Allocate/Free/Reallocate, PreviousAllocationId u64?, RequestedBytes u64, ActualBytes u64?, AllocatorEpoch GUID, BaselineCaptureId GUID?, Quality |
| 13 CaptureBoundary | Capture | CaptureId GUID, Kind Baseline/LightweightSnapshot/OwnershipGraph, Phase Begin/End, RootId GUID?, DispatcherId u64?, FirstSequence u64?, LastSequence u64?, ChunkCount u32?, Quality |
| 14 CaptureChunk | Capture | CaptureId GUID, ChunkIndex u32 (zero-based), RootId GUID, DispatcherId u64, typed arrays described below |
| 15 Loss | Capabilities | ProducerEpoch GUID, Interval TimeRange, LostEvents u64? (positive if known), Reason string |

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

Tree relationships: Root/Child/Content/Template/Popup for XamlCore,
CompRoot/CompChild for CompNode, VisualRoot/VisualChild for CompositionVisual.
Root relationships have no parent; other relationships require one.
Parent and child share process/domain, cannot be equal, and order is explicit.
Connected=false removes membership; it is never a destruction event.
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
AttemptCompleted. Attempt IDs are positive except a standalone commit may use
zero to indicate no XAML attempt. Only Submitted has a nonzero
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
ApiDescriptionEstimate requires estimated bytes; Unknown requires null bytes.
Use estimated/partial quality for inferred sizes; texture dimensions alone
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
retain source-sequence intervals and End reports chunk count. An End with
complete coverage is a claim to be validated against all chunks and loss by
the future processor, not proof merely from receipt of End. Root/dispatcher
captures are not implicitly atomic across the process.

CaptureChunk arrays:

* Objects: `SnapshotObject(Key, TypeName:string?, Address:u64?, Size:TypedValue)`.
* Ownership: `OwnershipEdge(From, To, Relationship:string, Evidence, Quality)`.
* Tree: TreeEdgePayload; Correlations: CorrelationPayload.
* Resources: ResourcePayload; Visuals: VisualDeltaPayload.
* Associations: AssociationPayload; Types: TypeResolvedPayload.
* Allocations: AllocationPayload.

Arrays are required, empty when no entries, never null. Each list is bounded
to 4096 items (Ownership 8192), additionally constrained by the byte budget.
Chunks must be split well before the ETW limit. Baseline is authoritative
only for the explicitly covered roots/dispatchers/capabilities. Missing
chunks, sequence gaps, decoder/ETW loss or changed epochs invalidate
completeness. Preserve previously committed history and explicit gap intervals;
do not pretend a new baseline reconstructs pre-attach births.

## Collector events

| ID / name | Keyword | Typed payload |
|---|---|---|
| 1 Process | Capabilities | Operation Start/Exit/Attach, CreationQpc i64? session ticks, Quality |
| 2 Metric | Metrics | Metric enum, Value TypedValue, Interval TimeRange, Source string, SourceVersion string, AdapterLuid string?, Engine string? |
| 3 Loss | Capabilities | Same LossPayload fields as runtime, collector producer epoch |

Collector semantic process is the selected instance even though the emitting
PID is the profiler. Attach with unknown creation QPC uses null. Collection
source/version/interval are mandatory; missing GPU support does not erase
CPU or memory samples.

Metrics/units: CpuUtilization and GpuUtilization in Percent; ProcessPrivateWorkingSet,
ProcessPrivateCommit, ProcessWorkingSet and XamlBytes in Bytes;
ConnectedXamlElements and CanonicalCompositionVisuals in Count.
Private working set excludes shared resident pages; private commit is not
residency. XamlBytes is scoped traced memory with explicit baseline coverage.
Connected XAML UIElements are not all live DependencyObjects; canonical
IVisuals are not CompNodes, wrappers, or correlated-XAML-element counts.

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
