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

## P04 native runtime wire mapping

P03 consumes these exact layouts; its additional, explicitly supported platform
ProcessStart v4 mapping, canonical identity binding, clock conversion and
unsupported-schema diagnostic policy are documented in [ingestion](ingestion.md).
No native collector schema is defined by the runtime layouts below.

The fre/chk foundation is `WinUIProfilerProtocol.h`,
`WinUIProfilerTracing.h` and `WinUIProfilerRuntimeBridge.cpp`. It registers
the fixed runtime provider above using SDK TraceLogging, **not** a manifest
event-ID namespace. Select by provider GUID, exact event name,
`SemanticEventId:u16` and `EventSchemaVersion:u8`; the latter is 1.
Do not select a schema by TraceLogging's numeric `EVENT_DESCRIPTOR.Id` or
descriptor version. Level and keyword must also match the catalog.

Every implemented event starts, in this order, with:

```
SemanticEventId:u16, EventSchemaVersion:u8, ProducerEpoch:GUID,
SourceSequence:u64,
TargetProcess:{
  ProcessId:u32, HasCreationTimeFileTime:bool8,
  CreationTimeFileTime:i64, InstanceId:GUID
}
```

`TargetProcess` is the actual runtime process; its creation time comes from
`GetProcessTimes`, not attach time. A false presence bit maps to null, not
zero. Source PID/TID/QPC come from the ETW header; source PID must equal
target PID for this runtime provider. The adapter supplies session ID,
ingest ordinal and selected-session QPC conversion. The process instance GUID
is stable for the loaded producer; all-listener disable/re-enable changes
producer epoch, not process creation identity. ETW GUID fields use the SDK's
native GUID representation; the TAP RFC-4122 byte rule is not an instruction
to reinterpret ETW GUID bytes.

Controllers selecting raw QPC timestamps use Wnode.ClientContext=1. ETL readers
must request PROCESS_TRACE_MODE_RAW_TIMESTAMP when interpreting the header as
raw QPC; default ProcessTrace conversion is system time, not QPC. Adapters must
validate the recorded clock and apply the same checked session-origin/frequency
conversion to header timestamps and payload QPC intervals.

Wire type aliases below describe **TDH-visible nested structs**, not raw C++
struct dumps. Field order is significant. All integers have explicit widths;
`bool8` is TraceLoggingBoolean (u8 with Boolean output type).

```
Optional<T> = { Present:bool8, Value:T }
NullableText = { Present:bool8, Value:utf8z }
Quality = { Kind:u8, Coverage:u8, Reason:NullableText }
Classification = { Kind:u8, Evidence:u8, Source:utf8z }
Object = { Domain:u8, ProducerEpoch:GUID, RuntimeObjectId:u64 }
Attempt = { ProducerEpoch:GUID, AttemptId:u64, RootId:GUID, DispatcherId:u64 }
Cause = { Kind:u8, Attempt:Optional<Attempt>, Reason:NullableText }
Value = { Kind:u8, Unit:u8, Integer:i64, Number:f64, Boolean:bool8,
          Text:utf8z, Quality:Quality }
```

Object process identity is inherited from `TargetProcess` for **each**
Object occurrence; this runtime schema cannot express cross-process objects.
All object and explicit-attempt epochs must equal the common epoch.
Optional false storage is ignored; it is never a valid zero identity.
For Value, only the arm selected by Kind is normalized; the other arms
become null. Missing selects no arm. Enum codes are the zero-based declaration
order of the named P01 C# enums (not strings, flags, guessed type names or
Windows ABI enums). The adapter maps them to the canonical string enums and
rejects unknown codes. Classification means XamlTypeClassification except
Resource.Classification, which uses ResourceClassification's enum tables.

After the common prefix, these are the concrete schema-1 payloads:

| Name / semantic ID | TDH fields, in order |
|---|---|
| RuntimeCapabilities / 1 | Epoch:GUID, Build:utf8z, Architecture:u32, QpcFrequency:i64, CapabilityCount:u32, Capabilities:CapabilityEntry[] |
| ObjectLifetime / 2 | Object:Object, Operation:u8, TypeId:Optional<u64>, Address:Optional<u64>, Quality:Quality, Classification:Optional<Classification>, Cause:Cause |
| TypeResolved / 3 | TypeId:u64, Name:utf8z, Classification:Optional<Classification> |
| CorePeerAssociation / 4 | Core:Object, Peer:Object, Linked:bool8, Cause:Cause |
| TreeEdge / 5 | Parent:Optional<Object>, Child:Object, Relationship:u8, Order:i32, RootId:GUID, DispatcherId:u64, Connected:bool8, Cause:Cause, Membership:u8 |
| VisualDelta / 6 | Visual:Object, Property:u8, Value:Value, Cause:Cause |
| Correlation / 7 | From:Object, To:Object, Kind:u8, Linked:bool8, Cause:Cause |
| Boundary / 8 | Kind:u8, Phase:u8, OperationId:u64, HResult:Optional<i32>, Cause:Cause |
| Frame / 9 | Attempt:Optional<Attempt>, SubmittedFrameId:Optional<u64>, Outcome:u8, HResult:Optional<i32> |
| Device / 10 | DeviceEpoch:GUID, Operation:u8, Api:utf8z, Quality:Quality |
| Resource / 11 | Resource:Object, Operation:u8, DeviceEpoch:GUID, BackingAllocationId:Optional<GUID>, AllocationBytes:Optional<u64>, SizeProvenance:u8, Owner:Optional<Object>, WidthPixels:Optional<u32>, HeightPixels:Optional<u32>, Quality:Quality, Classification:Classification, Cause:Cause |
| Allocation / 12 | AllocationId:u64, Operation:u8, PreviousAllocationId:Optional<u64>, RequestedBytes:u64, ActualBytes:Optional<u64>, AllocatorEpoch:GUID, BaselineCaptureId:Optional<GUID>, Quality:Quality, Cause:Cause |
| CaptureBoundary / 13 | CaptureId:GUID, Kind:u8, Phase:u8, RootId:Optional<GUID>, DispatcherId:Optional<u64>, Domains:DomainRange[], ChunkCount:Optional<u32>, Quality:Quality |
| CaptureChunk / 14 | CaptureId:GUID, ChunkIndex:u32, RootId:GUID, DispatcherId:u64, nine typed state arrays followed by five typed shared tables, below |
| Loss / 15 | Interval:{StartQpc:i64, EndQpc:i64}, LostEvents:Optional<u64>, Reason:utf8z, HasDomains:bool8, Domains:DomainReference[] |
| XamlPropertyChanged / 16 | Object:Object, Property:{PropertyId:u64, DeclaringType:utf8z, Name:utf8z}, Previous:Value, Current:Value, Cause:Cause |

RuntimeCapabilities.Epoch must equal the prefix epoch; its normalized Process
is TargetProcess. TypeResolved/Loss's payload ProducerEpoch is the prefix
epoch. Build is `WinUI-<WINUI_BUILD_VERSION>` (0 in the isolated harness).
Architecture codes are X64=0, Arm64=1. The ordinary handshake/opportunistic
announcement currently emits zero capabilities: writer availability does not
establish actual lifecycle/frame/resource coverage. The schema-1 encoder also
supports bounded nonempty capability entries. CapabilityCount must equal the
array length. Localized loss uses HasDomains=true with a typed domain array;
false means null, with canonical empty backing storage, not an authoritative
empty affected-domain list. True with a zero-length array means an explicit
empty list. These are complete **version-1** encodings, not future versions.

### Typed runtime vectors and capture state (native schema 1)

Arrays are tightly packed TraceLogging variable-count struct arrays: a
little-endian u16 element count followed by typed elements, **without native
padding or pointers**. TDH exposes each element's named primitive/struct
metadata even for empty arrays. Bounds: capabilities 128 unique entries;
loss/capture domain vectors 256 (capture domains unique); objects and each
other state array 4,096 except ownership 8,192. All still share the stricter
48 KiB total ETW transport budget; a caller must partition oversized state
in its later capture algorithm, not silently truncate it.

```
CapabilityEntry = { Capability:u8, Version:i32, Quality:Quality }
DomainReference = { EventId:u16, EventVersion:u8 }
CutValue = { CutId:GUID, ReservedThrough:u64, SettledThrough:u64 }
DomainRange = { Domain:DomainReference, BeforeSequence:Optional<u64>,
                ThroughSequence:Optional<u64>, Quality:Quality,
                SettledCut:Optional<CutValue> }
```

Every domain inherits ProviderId=RuntimeProviderId, ProducerProcessId=
TargetProcess.ProcessId, ProducerEpoch=the common epoch, and Target=
TargetProcess. CutValue inherits its enclosing range's full domain; it is
not an unqualified maximum or consumer receipt. EventId must be in the
runtime catalog and EventVersion must be 1. Preserve all P01 range rules:
ordered optional checkpoints; settled <= reserved; complete ranges have a
before checkpoint, and at End a through checkpoint plus a cut settling it.
Complete boundaries have nonempty domain vectors; End has a chunk count.
This encodes producer evidence only, not listener continuity/delivery closure.

CaptureChunk's full nested P01 state, naively repeated, exceeds TDH's property
limit. Its **typed** representation uses 1-based shared-table row indices.
Zero denotes null **only** for ParentIndex, OwnerIndex and the nullable XAML
ClassificationIndex; it is never an object/lifetime ID. All other references
must be nonzero and within the named table. Resolve references after reading
all tables; reject unknown enum codes, invalid references/optional backing
values and invalid resulting P01 observations. Duplicate table rows are
legal; they do not create distinct semantic objects.

In exact order after the chunk header:

| Array | Typed element fields, in order |
|---|---|
| Objects | KeyIndex:u32, HasTypeName:bool8, TypeName:utf8z, HasAddress:bool8, Address:u64, SizeIndex:u32, ClassificationIndex:u32 |
| Ownership | FromIndex:u32, ToIndex:u32, Relationship:utf8z, Evidence:u8, QualityIndex:u32 |
| Tree | ParentIndex:u32, ChildIndex:u32, Relationship:u8, Order:i32, Connected:bool8, CauseIndex:u32, Membership:u8 |
| Correlations | FromIndex:u32, ToIndex:u32, Kind:u8, Linked:bool8, CauseIndex:u32 |
| Resources | ResourceIndex:u32, Operation:u8, DeviceEpoch:GUID, HasBackingAllocationId:bool8, BackingAllocationId:GUID, HasAllocationBytes:bool8, AllocationBytes:u64, SizeProvenance:u8, OwnerIndex:u32, HasWidthPixels:bool8, WidthPixels:u32, HasHeightPixels:bool8, HeightPixels:u32, QualityIndex:u32, ClassificationIndex:u32, CauseIndex:u32 |
| Visuals | VisualIndex:u32, Property:u8, ValueIndex:u32, CauseIndex:u32 |
| Associations | CoreIndex:u32, PeerIndex:u32, Linked:bool8, CauseIndex:u32 |
| Types | TypeId:u64, Name:utf8z, ClassificationIndex:u32 |
| Allocations | AllocationId:u64, Operation:u8, HasPreviousAllocationId:bool8, PreviousAllocationId:u64, RequestedBytes:u64, HasActualBytes:bool8, ActualBytes:u64, AllocatorEpoch:GUID, HasBaselineCaptureId:bool8, BaselineCaptureId:GUID, QualityIndex:u32, CauseIndex:u32 |
| Keys | Domain:u8, RuntimeObjectId:u64 |
| Values | Kind:u8, Unit:u8, Integer:i64, Number:f64, Boolean:bool8, Text:utf8z, QualityIndex:u32 |
| Classifications | Kind:u8, Evidence:u8, Source:utf8z |
| Causes | Kind:u8, HasAttempt:bool8, AttemptId:u64, RootId:GUID, DispatcherId:u64, HasReason:bool8, Reason:utf8z |
| Qualities | Kind:u8, Coverage:u8, HasReason:bool8, Reason:utf8z |

References named Key/Object/Parent/Child/From/To/Resource/Owner/Visual/Core/Peer
index Keys. SizeIndex/ValueIndex index Values; other table names are explicit
in the reference name. Keys inherit the common target and epoch. Tree entries
inherit the chunk's root and dispatcher; the native encoder verifies those
equalities before omitting repeated fields. Types inherit the common epoch.
Causes with HasAttempt inherit the common producer epoch but preserve their
own AttemptId/root/dispatcher. The Classification table is interpreted as
XamlTypeClassification for Objects/Types and ResourceClassification for
Resources. Restore Has* nullable fields and reasons, setting absent values
to null; inactive backing values are canonical defaults. Restore only the
selected Values arm, setting all other normalized arms to null.

Each shared table has at most 4,096 rows and is scoped to this one event.
There is no persistent identity/capture registry or pointer identity. This
layout has **127 TDH properties including all fourteen array count fields**,
within TDH's 128-property limit, while preserving every P01 state field.
Native validation covers shape, units/quality, source epochs, nested topology
scope and bounded pointers/counts before encoding. Actual capture requests
still return E_NOTIMPL: schema writers do not traverse, capture, reconcile or
advertise any observational capability. No collector provider is implemented.

Native text is validated UTF-8, at most 1,024 bytes excluding its terminating
zero per field. Factories run only after definition/keyword enablement and
reentrancy guards. The synchronous writer borrows text only for that call.
The bounded encoder follows the public SDK's TraceLogging format and calls
EventWriteTransfer with typed provider/event metadata and explicitly packed
primitive fields. It refuses payload + metadata over 48 KiB, including when
the payload alone would fit. This transport budget is independent
of the 1 MiB owned normalized payload limit. No JSON payload is sent over ETW.
Native validation is an early guard, not a replacement for full P01
normalization/context validation at the adapter trust boundary.

### Private diagnostics ABI and producer settlement

`IWinUIProfilerRuntimeDiagnostics` IID
`289e7441-674b-45fa-a276-b9f59b7ec233` is added to the existing framework
WRL ClassicCom diagnostics class. It is private, version 1, not a new DLL
export or public WinRT API. The header is authoritative for fixed-width,
8-byte-aligned structures (ProcessIdentity 32 bytes, ProtocolInfo 80,
SequenceCut 96) on x64 and ARM64.

* GetProfilerProtocolInfo validates exact ABI size/version and returns actual
  process/epoch/frequency, architecture, a bit-per-semantic-ID writer mask
  (bit N describes ID N), and zero runtime capabilities. With Capabilities
  enabled, this handshake also publishes the native capabilities event without
  needing any later lifecycle/frame hooks.
* GetProfilerSequenceCut validates the selected implemented definition and
  returns a fresh immutable cut GUID with its epoch/target/event/version and
  **both** reserved and contiguous settled prefixes. The provider is implicit
  in this runtime-only interface. No listener/session evidence is returned.
* GetProfilerDispatcher uses the existing diagnostics interop's thread-to-queue
  lookup; zero thread ID is invalid, an unknown queue returns ERROR_NOT_FOUND.
  It does not enumerate objects, call application getters, convert boxed
  values into newly created core objects, or claim cross-dispatcher atomicity.
* RequestProfilerCapture returns E_NOTIMPL for version 1. Every entry rejects
  incompatible protocol versions with ERROR_REVISION_MISMATCH; buffer methods
  reject null pointers/wrong sizes without writing undersized buffers.

Provider registration registers/unregisters ETW and sets static provider traits.
Enable callbacks update
aggregate enabled state and a notification serial under a lock, without
COM, traversal, allocation or dispatcher calls. Process/GUID initialization
occurs outside loader-lock callbacks. The last-listener disable followed by
the first enable starts a fresh epoch lazily, after in-flight writes settle.
Changing another listener, keyword or level while any listener remains does
not reset IDs/counters. A valid producer epoch is established independently
of capabilities publication. Each definition's own keyword/level suffices,
including fresh Frames-only and Warning-only Loss listeners. Capabilities
publication is opportunistic when its informational definition is enabled;
neither its enablement nor successful publication gates another definition.
Consumers needing additional build/capability information must acquire that
explicitly; a handshake/publication is not proof any listener received it.

SDK aggregate keyword/level checks (including EventEnabled) can combine one
listener's Informational level with another listener's Frames keyword even
when neither enables Informational Frames. After the cheap aggregate callback
mask guard, the
foundation lazily caches the actual current process's individual OS session
selections from EnumerateTraceGuidsEx/TraceGuidQueryInfo. This happens only on
an enabled path after a notification, **not** in the enable callback. The
query uses a bounded 64 KiB scratch buffer outside the hot cached guard;
query failure/overflow stops the producer with a private-bridge HRESULT,
not guessed eligibility. Reentrant query work is suppressed. This cache is
only current definition-level writer eligibility, not persisted listener
continuity, payload/root-filter accounting, decoded receipt or absence proof.
The tests exercise the actual two-listener level/keyword false-positive case.

There are no new per-object fields, permanent listeners, allocation maps,
AddRef tracing or HeapSize calls. NewObjectIdentity only allocates monotonic
epoch-qualified IDs while an object-related family is enabled; callers in
later hook tasks must retire their weak mappings with the epoch and establish
a new baseline, not preserve pointer-only IDs. The foundation has no maps.
CaptureChunk-only enablement also permits IDs for its baseline state.
Enabled writes use bounded, call-local heap scratch (8 KiB metadata plus
48 KiB data, and five 4,096-pointer tables for a capture chunk), released on
every return. There are no allocations on a disabled writer/factory path;
these structural facts are not P17 measured overhead acceptance.

Each definition has its own 256-slot, allocation-free concurrent reservation
window. A reservation is made immediately before the write attempt and
settled on its terminal return, including failed/skipped writes. Completing
7 before 5/6 cannot advance a settled prefix past 4. Snapshots hold the
domain lock and cut GUIDs identify immutable returned facts. Window exhaustion,
ID overflow and sequence overflow return explicit failure; pre-reservation
exhaustion stops the producer rather than silently omit unsequenced work.
Counters never wrap. Another listener's changes cannot reinterpret a cut.

None of this establishes listener eligibility, positive decoded/processed
receipt, ETW transport finalization, sealed-source enumeration or absence.
Neither S_OK from a native write nor a settled cut is consumer proof.
The receipt/finalized-absence contracts below remain binding on future
capture/adapters. Foundation tests use synthetic native calls, not actual
framework lifecycle/render/resource hooks; P17 overhead remains unmeasured.

## Reusable value and quality schemas

`DataQuality` = `Kind:Measured|Estimated|Unavailable`,
`Coverage:Complete|Partial|Unavailable`, `Reason:string?`. The axes are
independent. Any non-measured or non-complete observation requires a
nonblank reason.

`TypedValue` = `Kind:Integer|Number|Boolean|Text|Missing`, `Unit`,
`Integer:i64?`, `Number:finite f64?`, `Boolean:bool?`, `Text:string?`,
`Quality:DataQuality`. Exactly one matching value is present, or none for
Missing with unavailable **measurement kind**, a required reason, and
independently Complete/Partial/Unavailable coverage. Every present kind
rejects Unavailable measurement quality; Estimated is permitted with a
reason. Units: Count, Bytes, Percent, QpcTicks,
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

One qualifying physical commit allocates one submitted ID. When that commit
includes work from multiple roots, each participating root's qualified attempt
can emit a Submitted observation with the **same** producer epoch, submitted
ID and commit timestamp. Startup counts distinct
`(ProcessInstanceKey, ProducerEpoch, SubmittedFrameId)` values, not Submitted
records or root-qualified FrameInfo rows.

Participation requires evidence of pending XAML composition work included in
the commit, not merely an existing root or a render walk in the current tick.
Root-associated animation or commit-request work can participate without a new
render walk when its root association is proven. Layout/render boundaries may
end before submission, but a submitted attempt's completion and query range
must include its actual commit timestamp. An attempt can complete without a
submission; failed commits produce no submitted ID. Unknown root attribution
is explicitly partial and diagnosed, never replaced with a synthetic root.
These are producer/consumer contract requirements, not claims that current
framework hooks or startup processing implement every path.

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

P04 implements the producer reservation/settlement foundation below. Listener
continuity, capture vectors and transport reconciliation remain later work:

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
