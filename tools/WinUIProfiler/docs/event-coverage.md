# Requirement-to-observation coverage and initial hook inventory

All hooks below are **proposed instrumentation sites inspected in the actual
WinUI framework source** at starting commit
`1af995b98a02d86b1951c4babe35caa269a6d569`. Nothing is instrumented in P01.
Paths are repository-relative; symbol names are more stable than line numbers.
Runtime provider/header work belongs to P04–P06; transport/injection to P08.
Existing telemetry is not the required schema and is not a dependency.

## P04 foundation status and overlapping telemetry decisions

P04 adds the fixed new runtime provider and typed, guarded writers for catalog
IDs 1–16 in optimized and checked native builds, including complete schema-1
capability/domain vectors and all nine capture state arrays. The concrete
TDH-visible layouts and private diagnostics ABI are in event-contract.md.
**At the P04 checkpoint, no lifecycle, property, membership, frame, device,
resource or allocator sites in the inventory below were hooked.** The supported-event bitmask
describes writers, not observational coverage; runtime capability arrays are
empty. Capture traversal/reconciliation, weak identity maps, the TAP/pipe,
collector and listener receipt/absence acquisition are later work; the native
capture **schemas/writers**, not those algorithms, are implemented here.
Requesting a capture returns E_NOTIMPL, not an empty successful artifact.

The private interface reuses the generic XamlDiagnostics WRL ClassicCom
connection and DiagnosticsInterop thread-to-dispatcher lookup. It does not
instantiate diagnostics on disabled telemetry paths, convert arbitrary
IInspectable property values, add public exports or change app dispatching.
Object IDs are explicit epoch-qualified monotonically assigned lifetimes,
not addresses; future owning hooks must supply actual lifetime boundaries.

Requirement-driven overlap decisions (not reuse of any profiler schema):

| Existing observation family | Decision for the new profiler | Existing framework behavior |
|---|---|---|
| Generic initialization/layout/render timing and performance markers | **Retain** for their existing diagnostics consumers; **replace** as this profiler's input with new Boundary/Frame definitions | Unchanged; no renaming, forwarding or dependency on their schemas |
| Diagnostic visual-tree mutation/root notifications and property inspection | **Retain** the diagnostics transport/service; **modify** only its private COM interface list/forwarding; **replace** profiling observations with epoch/lifetime/scoped typed records at later authoritative hooks | Existing notifications, property getters and dispatching are not intercepted or repurposed |
| Existing object/type/peer/tree/composition diagnostic telemetry | **Replace** as input for the new profiler; new typed object/domain/cause/classification contracts are independent | **Retain** for unrelated consumers; no dependency or source-schema adaptation |
| Existing device/resource and allocator/debug tracking | **Replace** as input for this profiler with explicit device/backing/allocator epoch records | **Retain** existing consumers; do not turn on TRACE_ALLOC, inspect debug headers, globally query HeapSize or trace AddRefs |
| Existing profiler-specific protocols, helpers and event layouts | **Retire** from this new design's dependency/reference set, not delete another tool's code | Excluded from implementation/design references; no source compatibility claim |

No overlapping existing event is claimed to satisfy the new contract, and no
existing telemetry is removed or globally enabled by this foundation.
The table is a migration/ownership decision, not a claim that the new runtime
hooks or final telemetry rollout have already happened.

The focused native tests compile the actual producer source without a
framework PCH/debug allocator, start uniquely named owned ETW sessions,
write typed events and decode their real ETLs with TDH. They test per-ID
reservation/contiguous settlement, multiple listeners, independently selected
fresh Frames-only/Warning-only Loss epochs, typed-value quality/discriminator
parity, nonempty typed arrays, actual near-limit transport budgets, disabled
lazy factories, reentrancy and failed writes. An isolated executable using the
same test/producer translation units avoids the external framework module's
packaged-WPF/elevated fixture prerequisites. Compilation/discovery of the
framework DLL is not execution of that fixture or of app lifecycle hooks.
No all-process runtime instrumentation or measured fre overhead is established
by structural/build validation; P17 remains the acceptance gate.

Run the isolated x64 optimized harness from the repository root:

```powershell
.\initrun.ps1 -Flavor amd64fre msb /q dxaml\test\native\external\framework\profiler\ProfilerInstrumentationHarness.vcxproj /p:Configuration=Release /p:Platform=x64 /p:UseXamlCompiler=false /p:PGOBuildMode=Off
.\BuildOutput\ProfilerHarness\x64\Release\ProfilerInstrumentationHarness.exe
```

Use amd64chk/Debug for checked x64 execution, or arm64fre/Release/ARM64
and arm64chk/Debug/ARM64 for cross-compilation. Building an ARM64 executable
on an x64 host is not executing its tests.

## P06 allocator working-checkpoint validation

The allocator hooks observe successful heap operations through an optional,
constant-initialized runtime callback table. The shared allocation library also
links into build tools without the runtime bridge; with no registered table,
the wrappers call the original heap APIs directly. Registration introduces no
allocating global initializer or destructor. Suppression is shared with the
other profiler observers, not limited to allocator recursion.

The actual matched x64 checked/fre lifecycle host includes an
`AllocatorTransitions` mode. It constructs and releases real XAML objects with
allocation observations enabled, performs more churn during a full provider
disable, then re-enables and repeats. It requires measured/partial quality,
no fabricated baseline, distinct allocator/producer epochs, renewed epochs
across the gap, globally unreused allocation lifetime IDs, contiguous
per-producer allocation sequences, tracked predecessor/size semantics and
zero ETW transport loss. Foreign-process events are excluded. Build and run
from the implementation checkout, after building the matching Product:

```powershell
.\initrun.ps1 -Flavor amd64fre msb /q dxaml\test\native\external\framework\profiler\ProfilerLateEnableHarness.vcxproj /p:UseXamlCompiler=false /p:PGOBuildMode=Off
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -Run -TestMode AllocatorTransitions
```

Use `amd64chk` for checked execution. Standalone allocator tests separately
exercise failed operations, capacity, observer suppression and runtime context
races. Pending coverage retains its original producer epoch and the earliest
context-acquisition QPC until successful publication. Coverage gaps do not prove
an exact number of lost events: `LostEvents` remains absent. Deferred old-epoch
Loss continues that epoch's Loss sequence, including prior nonallocator Loss,
rather than borrowing the later producer's identity or sequence.

Origin leases and pending evidence use 64 fixed, allocation-free slots. Active
operations and pending publications pin their origins; acknowledged, unreferenced
slots can be reused. Capacity exhaustion and saturated accounting explicitly
fault the provider instead of evicting evidence or reporting success.

```powershell
.\tools\WinUIProfiler\WinUIProfiler.Native.Tests\Run-AllocatorRuntimeTests.ps1
dotnet test .\tools\WinUIProfiler\WinUIProfiler.Engine.Tests\WinUIProfiler.Engine.Tests.csproj --no-restore -c Release --filter FullyQualifiedName~Deferred_allocator_loss
```

The native tests cover actual multi-epoch ETW publication, shared Loss sequence
continuation, in-flight context retirement, 192 origin renewals, pending-origin
capacity and counter overflow. The managed test retains a real native ETL,
decodes and replays deferred Loss under its original domain, rejects an invalid
clock mapping instead of clipping the interval, and reopens persisted incomplete
health. These are not proof of complete allocator coverage, graphics/resource
observations, native ARM64 execution or disabled-path performance; this working
checkpoint does not complete P06.

## P06 logical graphics working-checkpoint validation

Actual D3D11 device creation/loss/WinUI ownership release and native
`DCompSurface` initialization/rebind/resize/ownership release/destruction now
have Resource-keyword producers. Driver-visible `SystemMemoryBitsDriver`
staging-texture owners now use the same typed logical-resource observer at
successful factory creation, present-only pool reuse and actual owner destruction.
Pool return retains ownership and does not create a new lifetime.
Solid-color composition-brush holders are instrumented at successful activation
and actual nonempty holder reset/destruction. Their identities name logical
WinUI ownership slots, not final COM lifetimes; color-only changes preserve
identity, pixel dimensions are absent and hardware association remains unknown.
Their Owner field now joins the actual native solid-color brush through the
existing qualified XamlCore identity registry. Ending records retain that
established owner; transient address reuse cannot relabel the earlier slot.
Other resource families still have unknown XAML owners.
These describe logical wrappers and observed
device associations, not complete physical allocation accounting. Backing IDs,
allocation bytes and unknown external device associations are not fabricated.
The [resource coverage manifest](resource-coverage.md) lists each implemented
boundary and the still-uncovered texture/bitmap/brush/ownership families.

The standalone owned-ETW suite checks lifecycle reuse, failed operations,
external-device rebind, shared native device associations, repeated release,
resource-only/full selection gaps, runtime restart, exact 65,536-entry bounds,
post-reservation selection/epoch races and termination while another
thread holds the eligibility lock. The termination test reproduced a blocked
hook before its state gate was moved ahead of eligibility lookup.
Retained native ETLs decode/replay and preserve partial quality and unknown
backing/bytes and known qualified brush owners after SQLite reopen. Owner tests
also exercise late baseline, address reuse, independent Objects/Resources
selection gaps and epoch races. Finite ETL enumeration may complete while
observation coverage remains partial; Completed never means a complete baseline.

The actual matched checked/fre `GraphicsResources` host renders text and
WriteableBitmaps, changes content, then releases and destroys its wrappers.
It requires positive creation/ownership-release/destruction observations,
zero ETW transport loss and no fabricated physical Device Destroy.
The additional `GraphicsReset` mode uses the matching existing private test
interface to retire only its own process's device instance and request WinUI
recovery. It requires actual Lost cleanup, old-device staging-owner destruction,
successful later replacement-device creation and staging creation associated
with that replacement's epoch. Both checked/fre actual workloads observe three
staging-owner creates and two old-owner destroys, with zero transport loss.
This is controlled WinUI recovery, not physical GPU removal.
Standalone tests alone do not qualify those actual source sites.

```powershell
.\tools\WinUIProfiler\WinUIProfiler.Native.Tests\Run-GraphicsObservationTests.ps1
dotnet test .\tools\WinUIProfiler\WinUIProfiler.Engine.Tests\WinUIProfiler.Engine.Tests.csproj --no-restore -c Debug --filter "FullyQualifiedName~IntegrityTests|FullyQualifiedName~Native_graphics_ETL"
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -Run -TestMode GraphicsResources
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -Run -TestMode GraphicsReset
```

Build the matching Product/host first; use `amd64chk` for checked validation.
Full resource families, physical backing/deduplication, physical GPU removal,
native ARM64 execution and measured overhead remain open. This checkpoint
does not complete P06.

## P05a app-local host gate

The separate `ProfilerLifecycleHarness.vcxproj` now provides an unpackaged
`WindowsAppSDKSelfContained=true`, `WindowsPackageType=None` native smoke host.
It uses the SDK's ordinary native auto-initialization and self-contained manifest
generation, not hand-written WinRT activation registration or installed packages.
Its Foundation/IXP paths are the same evaluated `LiftedMRTPackagePath` and
`LiftedIXPPackagePath` used by the native build; sample-app package defaults are
not substituted. The entire evaluated local Product runtime/resource payload is
staged as a local WinUI component; all overlapping package files defer to that
payload, not just MUX/MUXC. No profiler UI is introduced.

From the initialized implementation checkout:

```powershell
.\initrun.ps1 -Flavor amd64fre msb /q dxaml\test\native\external\framework\profiler\ProfilerLifecycleHarness.vcxproj /p:Configuration=Release /p:Platform=x64 /p:UseXamlCompiler=false /p:PGOBuildMode=Off
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -Run
```

The host needs matching local Product outputs, including the `res\19h1` resource
target and generated C++/WinRT projections. The SDK creates and embeds its
activation manifest. Evaluated deployment properties, the complete payload hash
manifest, actual loaded-module paths/hashes and bounded runtime output are saved
under `BuildOutput`. The runner verifies hashes against original inputs and local
Product, rejects loaded files outside the expected deployment or unmanifested
app-local DLLs, and terminates only its own child if it fails to exit.

P05a executes XAML initialization and one real Grid construction/teardown, followed
by WindowsXamlManager teardown. It proves this hosting prerequisite only: it does
not claim producer-hook correctness, frame semantics, checked/ARM64 execution,
four-flavor validation, or measured P17 overhead. Those remain separate gates.

## P05 working-checkpoint validation

P05 producer and lifecycle-test changes remain under implementation and review.
The following results describe selected actual x64 optimized runtime tests, not
completed hook coverage or an end-to-end profiler. The separate
`ProfilerLateEnableHarness.vcxproj` reuses the supported self-contained smoke
host's SDK initialization, complete local Product payload and loaded-module
verification. Its output directory and evaluated deployment properties are
separate from the smoke host. Rebuild the selected host after changing Product;
the verifier rejects a stale deployment rather than silently testing old DLLs.

The current checked and optimized x64 products pass SimultaneousManagers,
LateEnable, ManagerRestart, OrderedEdges, FailedInsertion, Roots, IdleStartup,
FrameSubmissions, DetachedLayout, AnimationRoot and DebugCounter, with verified local DLLs
and zero observed ETW loss. These are qualified working checkpoints, not full
P05 acceptance or ARM64 execution. Simultaneous-core/root testing exposed a
real CoreWindow fallback attribution defect: even
`GetContentRootForElement(...NoFallback)` can supply an unrelated fallback.
Profiler scope now uses the actually associated no-fallback VisualTree, or
unknown, without changing the framework's functional lookup behavior.
Construction before Window.Content attachment must not fabricate a connected
root; immutable scope, survivor, property and genuine shutdown checks remain.

DetachedLayout performs six real measure and six arrange operations on detached
content with no window activation or dispatcher pump, observes balanced layout
boundaries and zero physical submissions despite pending work. AnimationRoot
uses an actual dependent Width animation in one of two roots and verifies
qualified attempt containment, root identity and physical timestamps. Completed
animation and positive primary submissions are required; legitimate work in
another root is not falsely forbidden. Submission count is observational, not
an artificial fixed frame target.

DebugCounter uses a real Application.Start host and public
DebugSettings.EnableFrameRateCounter. Successful extra standalone commits
have neither Attempt nor SubmittedFrameId, alongside independently qualified
primary submissions for the tagged root. Parent checked/optimized reruns
observe four/three standalone commits and two primary submissions respectively,
with zero ETW loss; these are observations, not fixed test thresholds.
No-device/no-commit, post-primary failure and resource-specific outcomes remain
separate qualification gates.

```powershell
.\initrun.ps1 -Flavor amd64fre msb /q dxaml\test\native\external\framework\profiler\ProfilerLateEnableHarness.vcxproj /p:Configuration=Release /p:Platform=x64 /p:UseXamlCompiler=false /p:PGOBuildMode=Off
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -TestMode Lifetime -Run
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -TestMode Roots -Run
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -TestMode IdleStartup -Run
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -TestMode FrameSubmissions -Run
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -TestMode FailedInsertion -Run
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -TestMode OrderedEdges -Run
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -TestMode SimultaneousManagers -Run
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -TestMode DetachedLayout -Run
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -TestMode AnimationRoot -Run
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -TestMode DebugCounter -Run
```

| Selected runtime check | Working-checkpoint result | Scope and remaining limitation |
|---|---|---|
| Core/peer construction, safe type resolution, association and destruction | Passed with actual hooked local Product | Does not prove every lifetime domain or construction path |
| Existing Grid first observed after Objects enable | Passed | Actual Width(43) emits Missing previous value, matching Scalar units, baseline-present rather than fabricated birth, type and destruction |
| Objects disable/reenable with another family listener still enabled | Passed | Selected test preserves identity/epoch continuity and receives a new Objects baseline; not every listener combination |
| Failed construction and allocator address reuse | Passed | Selected actual factory/parser and transient-object paths; reused addresses receive distinct lifetime IDs |
| Multiple roots, tree/property observations and core-to-comp-node correlation | Passed in root-only and full Roots modes | Actual framework events include core/comp/visual correlations; complete physical-edge coverage remains under review |
| Visual property observations | Passed in root-only and full Roots modes after owner-slot wiring | The selected workload emitted 24 actual visual-property observations with non-retaining partial owner-interval identities; no complete COM-lifetime capability is claimed |
| Shared physical submission ID/timestamp and attempt containment | Passed in isolated FrameSubmissions and full Roots modes | The same actual two-root workload emits submissions sharing one physical ID and exact QPC across roots; submitted attempts remain open through commit. Failed/standalone/no-device commit paths remain separate gates |
| Idle startup with fewer than ten submissions | Passed in isolated IdleStartup and full Roots modes | The unchanged idle assertion observed one distinct SubmittedFrameId, not a count of Frame-family records or boundaries; this does not prove every commit outcome |
| Rejected native hand-in insertion | Passed in FailedInsertion mode with rebuilt local Product | The exact ignored native E_INVALIDARG diagnostic occurs within the replacement phase; the original physical parent retains the visual and no successful second-parent edge is emitted |
| Exact physical sibling order and inter-node edges | Passed in OrderedEdges mode with rebuilt local Product | Fifteen exact qualified-ID/index/phase checks cover hand-in migration and both inter-node child moves. Removal indexes are checked against independently snapshotted physical sibling collections, evolving after each verified removal; a phase-start index is not incorrectly reused after sibling removal |

The actual failing visual returns E_NOINTERFACE for IWeakReferenceSource.
This unsupported observation now emits a scoped diagnostic without poisoning
the control protocol or unrelated core-to-comp-node correlations. The native
regression preserves the original HRESULT and verifies unexpected allocation
failure still surfaces explicitly. That is not proof of visual lifetime support.
XAML object
pointers may be used as non-retaining in-process join keys at authoritative
owner boundaries, resolved to the existing process/epoch/lifetime-qualified
object IDs for durable observations. Pointer reuse must not merge lifetimes.
Visual ownership release alone does not prove final COM-object destruction.

These selected runtime results do not close the owner-scope review. The
resource-only observation-gap and late correlation enablement regressions pass
in the native bridge harness; actual exact physical sibling-order, inter-node
edge and rejected-insertion validation now pass after rebuilding Product.
Independent owner-scope rereview resolved all four product findings after the
MatchAll eligibility-generation correction. Six actual bridge/ETW cases cover
Resource-only slot gaps, renewed correlation announcements and saved edge-context
invalidation, each with and without a genuinely eligible continuous second
listener. The rebuilt optimized runtime passes the selected actual-app modes.
A subsequent late-enable failure was traced to two proven owned debugger ETW
sessions that kept Objects enabled before test construction, not lost events or
a product baseline race. After stopping only those proven orphan sessions, the
independent full Roots rerun passed with zero ETW loss. The fixture now rejects
an already-enabled local Objects listener before construction and tests that
precondition with a real owned listener. Its MatchAll predicate also permits
a genuinely Resource-only listener rather than falsely declaring contamination.
Checked x64 startup exposed allocation of a global registry before PAL
initialization. Lazy registry construction now passes actual checked startup,
LateEnable, all fifteen ordered-edge assertions, rejected native insertion and
full Roots, with verified rebuilt local DLLs and zero ETW loss. The independent
lifecycle review required nonblocking process-termination handling and
reclamation of empty per-thread layout stacks. The c21 interlocked terminal
gate and stack reclamation now have narrow independent review approval and
checked/optimized product qualification. Remaining actual-hook, capability and
architecture gates still block P05 acceptance; successful ordinary teardown
alone does not prove safe termination while another thread holds an observation lock.
The owner-scope product review is not full milestone approval.
Passing nonzero event assertions does not establish those transition/ordering
properties or complete topology coverage.

`-TestMode LateEnable` selects only the existing-object test;
`-TestMode IdleStartup` selects only the existing idle-startup test so an
unrelated Roots failure cannot prevent its execution. `-TestMode FrameSubmissions`
executes the same two-root workload with its existing layout/frame/submission
assertions, independently of visual observations. Default Roots still requires
all original tree/property/correlation/visual/frame assertions; a focused frame
pass alone cannot establish visual coverage. Omitted TestMode
still runs the original smoke host. Lifecycle test success does not establish
popup/content/template coverage, all composition mutation paths, failed or
standalone commit semantics, checked/ARM64 runtime correctness, or measured
disabled-path overhead. Those remain open acceptance gates. P04's standalone
schema harness remains separate and cannot substitute for actual-hook tests.

## Product requirement matrix

| Requirement | Required observations/contracts | Initial actual hook / producer boundary | Consumer/query and coverage limits |
|---|---|---|---|
| 1. C# WinUI UI, independent .NET engine, native TAP | EventEnvelope, typed payloads, IProfilerQueryService, TAP header/DTOs | Future UI/CLI and C++/WinRT TAP; `dxaml\xcp\components\xamlDiagnostics\LiveVisualTree.cpp` is an existing framework diagnostics boundary to investigate | All twelve typed queries; no UI/WinRT dependency or native project in P01 |
| 2. Windows 11 x64/ARM64, one process instance | RuntimeCapabilities architecture/build/capability versions; semantic ProcessInstanceKey; collector Process | `dxaml\xcp\core\dll\xcpcore.cpp` `CCoreServices::Create` / `Initialize`; future OS process discovery | GetSession; never join PID alone; min OS 22000, API declarations 26100 only behind capability checks |
| 3. Packaged/unpackaged launch or attach, OS collector semantics | Process Start/Attach/Exit, nullable creation QPC/FILETIME, CPU/GPU/memory Metric source/interval/version | Future packaged activation / process creation collector, process handle creation identity, documented OS process-time/working-set/private-usage and GPU engine collectors | GetSession/GetMetrics/GetStartupTimeline; missing pre-attach history explicit; no launcher/collector implemented |
| 4. New fre telemetry independent of current events | New runtime GUID, schema 1, provider/keyword gating, explicit lifetime IDs | Object/device/frame/tree/resource/allocation sites below | Every consumer; actual provider/hook implementation and fre overhead measurements later |
| 5. Creation to first ten submitted XAML frames total | Process CreationQpc, Frame Attempt/Submitted/Failed/AttemptCompleted/StandaloneCommit, root/dispatcher, independent submission ID | `xcpcore.cpp` `CCoreServices::Tick` / `SubmitPrimitiveCompositionCommands` primary call to `DCompTreeHost::CommitMainDevice`; actual underlying device commit in `components\comptree\DCompTreeHost.cpp` | GetStartupTimeline/GetFrames; no-op S_OK is not submit; post-commit failure does not retract; no standalone/debug/tick/display counting or idle forcing |
| 6. CPU/GPU/private WS/private commit/scoped XAML/connected UIElements/canonical IVisual counts | Metric units/quality and typed physical-adapter/engine/scope; four dedicated/shared committed/resident GPU kinds; authoritative UIElement type evidence; logical/visual membership; resource kind/evidence | OS collectors; `uielement.cpp` EnterImpl/LeaveImpl; `HWCompNodeWinRT.cpp` EnsureVisual/CreateWUCSpineVisual; `XcpAllocation.cpp` allocation/free/resize | GetMetrics/GetResourceUsage/scoped trees; distinct visual-connected XamlCore UIElements, not all DependencyObjects; busiest-engine versus per-engine utilization; no all-process GPU attribution from WinUI hooks |
| 7. Lightweight vs full heap artifacts, immutable metadata, compare/roots | Artifact-free snapshot attempts; optional validated artifact; capture extent/consistency/scopes; attribute+group comparison keys, explicit incompatibility/missing; OwnershipEdge/RetentionPath evidence; 1–64 snapshot IDs vs baseline | Future private runtime bridge visits covered roots/dispatchers; allocator/resource sites below; separate native/managed heap providers | GetSnapshots/CompareSnapshots/GetRetentionPaths; failed/cancelled have reasons, no fake hash; multi-root captures are not implicitly atomic; proven roots versus uncertain retention |
| 8. Same live/offline normalization, no offline target commands | EventCodec.Normalize + ProducerStreamKey; SessionMode.Offline; stored artifacts | Future live ETW adapter and offline ETL adapter feed the same owned codec | All read-only queries; command executor must reject offline; saved artifacts remain viewable |
| 9. ETW plus authenticated named pipe, diagnostics-injected TAP/private bridge | Length-prefixed TAP protocol version/type/request ID/bounded typed body, capabilities/capture IDs/nonce | Future IXamlDiagnostics injection + private runtime bridge; framework diagnostics boundary above | Target controller later; no arbitrary remote commands, nonce alone is not auth |
| 10. One bounded processing owner for histories/trees/correlations/resources | Chronological typed lifetime/type/property/membership/visual/resource/correlation/allocation/boundary observations; explicit epoch/root/dispatcher attempt causes versus temporal overlap; unique backing IDs | All producers below; future dedicated processing thread | GetObjectHistory/GetFrames/scoped trees/GetCorrelations/GetResourceUsage; composition properties survive query representation; no fake processor in P01 |
| 11. SQLite WAL/single writer/temporal state/watermarks/typed cancellable paged API + CLI | CommittedWatermark, ResultContext, PageRequest/PageInfo, mandatory typed result validation and nested freezing, twelve operations, explicit CancellationToken | Future store writer after normalized ingestion; no runtime hook needed | P02 storage; deterministic committed pages reject invalid/foreign rows and caller mutation; no SQL/chat/MCP or fake success API |
| 12. Honest quality/capability versions/loss/epochs/identity/history/resource stops | DataQuality axes, Missing TypedValue, domain-localized Loss, delivery-homogeneous sequences, settled cuts, independent live receipt/processed progress, listener eligibility spans, sealed-source absence proof, capture vectors/chunks, SessionStatus/StopReason | Runtime/collector Loss + capabilities; future per-domain reservation/completion and live receipt; coordinated source finalization for negative proof; allocator baseline; queue/store limits | All distinct settled IDs with completed receipt, including 4,7,5,6, prove positive live coverage without a sealed source; pending IDs remain Unproven; only genuinely closed absence proves loss; preserve committed history and stop visibly |
| 13. Local artifacts and measured off-path cost | Relative local snapshot path/hash; provider/keyword tests before payload work | Every proposed producer; especially constructor/destructor, allocator, render loop, graph rundown | Artifact owner/capture UI later; fre x64/ARM64 provider-off and enabled-keyword overhead is unmeasured in P01 and remains an acceptance gate |

## Inspected initial runtime hook sites

All short paths in the following table are relative to `dxaml\xcp`.
Instrumentation must be added around authoritative operations, not inferred
from return values or trace names. Constructor type resolution may be delayed
until the object's final runtime type is available.

| Observation family | Initial actual file/symbol | Hook intent, provenance and limitations |
|---|---|---|
| Core object birth | `components\DependencyObject\dependencyobject.cpp` `CDependencyObject::CDependencyObject` (line 51) | Assign explicit lifetime ID from the new producer epoch; guard all work before payload/type queries. Constructor address is optional metadata, not ID. Emit final type via TypeResolved when known |
| Core object final destruction | `core\core\elements\depends.cpp` `CDependencyObject::~CDependencyObject` (line 36) | End authoritative core lifetime once; not merely detachment or peer release |
| Core/peer and type association | `dxaml\lib\DependencyObject.cpp`; `components\DependencyObject\dependencyobject.cpp`; existing diagnostics boundary `components\xamlDiagnostics\LiveVisualTree.cpp` | The later lifecycle-hook task must identify exact post-construction association hooks; do not infer peer identity from core pointers or promise constructor-time final type |
| Connected UIElement/root membership | `core\core\elements\uielement.cpp` `CUIElement::EnterImpl` (1299), `LeaveImpl` (1740); `core\dll\VisualTree.cpp` | Root/dispatcher-scoped connection edges and connected UIElement counts; a leave is not destruction |
| Ordered children | `components\Collection\UIElementCollection.cpp` append/insert/remove/clear operations | Emit old/new order/membership at successful mutations, not from an unordered walk |
| Content/template/popup relationship kinds | `core\core\elements\ContentControl.cpp` content mutation; `Control.cpp` template application; `Popup.cpp` popup transitions; `components\elements\Popup.cpp` | Later lifecycle hooks must instrument successful relationship changes with explicit root/dispatcher; include popup roots, content and template kinds; static visual-children-only traversal is incomplete |
| Initialization and render attempt | `core\dll\xcpcore.cpp` `CCoreServices::Initialize` (3657), `Tick` (4336), `SubmitPrimitiveCompositionCommands` (7179) | Initialization/attempt boundaries, not frame-submission counters; process creation belongs to OS collector, not Initialize |
| Measure/arrange boundaries | `core\layout\LayoutManager.cpp` `CLayoutManager::EnterMeasure` (166), `ExitMeasure` (180), `EnterArrange` (189), `ExitArrange` (203), `UpdateLayout` | Pair operation IDs in authoritative epoch/thread; preserve failure and incomplete intervals |
| Qualifying primary commit | `core\dll\xcpcore.cpp` primary `CommitMainDevice` call around 7265; `components\comptree\DCompTreeHost.cpp` `CommitMainDevice` (1130) actual `m_spMainDevice->Commit()` | Must carry primary XAML-attempt intent into the actual commit hook; emit Submitted immediately after successful actual commit. Null main device returns S_OK with no commit. Fallible PostCommit/layout work and optional debug-counter commit around 7315 are separate observations |
| Standalone commits | `components\comptree\DCompTreeHost.cpp` `RequestMainDCompDeviceCommit`, `CommitMainDevice`; nonprimary callers | Record commit diagnostics without SubmittedFrameId; do not infer primary status from being inside a tick |
| CompNode/canonical IVisual identity/topology | `components\comptree\HWCompNodeWinRT.cpp` `HWCompTreeNodeWinRT` constructor/Create, `EnsureVisual` (310), `CreateWUCSpineVisual` (376), `InsertWUCSpineVisual` (419), `GetWUCVisual`; `DCompTreeHost.cpp` `CreateContainerVisual` (1241), `SetRootForCorrectContext` | Track CompNodes separately from canonical IVisual COM identity; aliases/hand-off/spine/content visuals can correlate many-to-many. External visual mutations require explicit capability limits or additional observation hooks |
| Typed visual state and XAML/visual/resource correlations | `components\comptree\HWCompNodeWinRT.cpp` visual/property writes; `core\hw\VisualContentRenderer.cpp` `RenderPrimitive`, `GetCompositionBrush` | Record known typed property deltas and observed resource assignment; no claim to full external compositor state or animation evaluation |
| XAML property and causal change representation | `core\core\elements\depends.cpp` `CDependencyObject::NotifyPropertyChanged` (2793), `OnPropertyChanged` (1474); investigate authoritative setter/notification coverage in the later lifecycle-hook task | Actual notification candidate accepts property-change parameters and has fallible callbacks; it is not proof of all successful mutations or a source dependency on existing telemetry. New XamlPropertyChanged retains property key, previous/current values and explicit/unknown/outside-attempt cause. Carry qualified attempts through layout/tree/visual/resource/allocation observations; proximity or ActivityId is not proof |
| Device birth/loss/epochs | `plat\win\desktop\D3D11DeviceInstance.cpp` `CD3D11DeviceInstance::EnsureResources` (165), `ReleaseResourcesOnDeviceLost` (313); `d3d11device.cpp` `CD3D11Device::Create`, `EnsureResources`, `ReleaseScratchResources` | Separate device epochs and ownership scopes; device loss must invalidate incomplete resource/backing state |
| Surface resource/backing/rebind/resize | `plat\win\desktop\DCompSurface.cpp` Create overloads (33/75/110), InitializeSurface (142/226), ReleaseLegacyDCompResources (352), Lock/Unlock/QueueUpdate; `d3d11device.cpp` graphics resource API boundaries | Distinguish logical resource IDs and unique backing allocations; report measured API allocation sizes only where authoritative; dimensions are estimates, not residency |
| D2D creation/share | `plat\win\desktop\D2DAcceleratedRT.cpp` `CD2DRenderTarget<T>::CreateBitmap` (183), CreateSolidColorBrush (247), CreateBitmapBrush (286+), CreateSharedBitmapBrush (408) | Observe backing sharing and scoped ownership/release; WinUI relinquishing a COM reference does not prove final external destruction |
| Composition brush/cache ownership | `components\brushes\WUCBrushManager.cpp` EnsureResources (43), ReleaseDCompResources (81), ReleaseGraphicsDevice (92), GetMaskBrush/GetNineGridBrush/GetSurfaceBrush/GetColorBrush | Observe creation/rebind/cache ownership, close and release distinctly; share backing IDs across aliases |
| Traced allocator requested/actual/baseline coverage | `components\allocation\lib\XcpAllocation.cpp` OSMemoryAllocateFailFast (174), OSMemoryAllocateZeroMemoryFailFast (212), OSMemoryAllocateNoFailFast (250), OSMemoryResize (282), OSMemoryFree (343), OSMemoryGetBlockSize (40) | Preserve realloc failure/original lifetime, new allocation ID on successful reallocation, requested vs actual bytes. Never call block-size or build payloads while keyword is disabled. Scope is this allocator, not all native heaps |
| Baseline/snapshot per-dispatcher traversal | Future private bridge over roots, object/association/topology/resource/allocator sites above | Begin/chunk/end with capture IDs, bounded typed state, per-domain checkpoint vectors and matching per-root/dispatcher chunk topology. Save actual scope intervals/consistency; late attach establishes presence only, no synthetic historical births |
| Collector process/CPU/GPU/memory | Future OS collector, no WinUI runtime hook | Documented Task Manager underlying OS collector semantics implemented independently, target process handle identity, source/version/interval/capability; not resource ownership from WinUI-only telemetry |

## Honest gaps and acceptance gates

* P01 tests codec/contracts using synthetic data only; they do not validate ETW
  manifests, fre emission, native identity lifetimes, collectors or TAP auth.
* Existing framework APIs are starting points. A site listed here is not proof
  of exhaustive lifetime/ownership/visual property observation. Exact peer,
  template/content/popup, external visual and heap-root hooks need capability
  coverage checks in their owning future implementation tasks.
* Current canonical visual properties are a declared subset. Unsupported
  properties and mutations outside observed WinUI paths are unavailable/partial,
  never silently assumed unchanged.
* Full native ownership cannot be reconstructed reliably from arbitrary memory;
  full managed heaps require runtime-aware identities/roots across compaction.
  Surface dimensions and WinUI COM references do not establish actual
  all-process GPU allocation/residency or final object death.
* A baseline covers what was enumerated at recorded boundaries. Cross-dispatcher
  changes, loss, epochs and missing chunks must survive as explicit quality
  limitations. Later snapshots do not repair unobserved past history.
* Resource kind and UIElement classification require authoritative creation-API,
  runtime-type or bridge evidence, not names. Missing evidence remains unknown.
  Description-derived bytes are Estimated independent of coverage.
* Positive settled-prefix receipt uses measured listener eligibility and
  completed relevant decoding/processing, independently of an archive.
  Every expected distinct ID must be present; pending IDs or processing are
  Unproven. Live baselines additionally require all validated/processed
  chunks/boundaries, epochs, capabilities and actual scope/consistency under
  the committed view. End alone is insufficient.
* Finalizing absence still requires genuinely settled/closed source
  reconciliation. Another listener's selection does not alter continuity;
  own-domain filter/enable changes remain Unproven. P04 supplies native
  reservation counters and contiguous settled cuts only. Receipt acquisition,
  terminal transport guarantees, source verification and projections remain
  downstream; P01 tests contracts and the pure helper only.
* Completed scope intervals inside unfinished artifact-free snapshots must
  still fit the committed query watermark; no invented overall end/artifact
  is needed to enforce that publication boundary.
* Before production rollout: measure provider-off and keyword-on overhead in
  fre x64/ARM64, prove no payload work while disabled, exercise loss/reuse,
  queue/store limits and replay equivalence, compare OS samples against the
  declared semantics, and verify Windows 11 capability fallback.
