# Requirement-to-observation coverage and initial hook inventory

All hooks below are **proposed instrumentation sites inspected in the actual
WinUI framework source** at starting commit
`1af995b98a02d86b1951c4babe35caa269a6d569`. Nothing is instrumented in P01.
Paths are repository-relative; symbol names are more stable than line numbers.
Runtime provider/header work belongs to P04–P06; transport/injection to P08.
Existing telemetry is not the required schema and is not a dependency.

## Product requirement matrix

| Requirement | Required observations/contracts | Initial actual hook / producer boundary | Consumer/query and coverage limits |
|---|---|---|---|
| 1. C# WinUI UI, independent .NET engine, native TAP | EventEnvelope, typed payloads, IProfilerQueryService, TAP header/DTOs | Future UI/CLI and C++/WinRT TAP; `dxaml\xcp\components\xamlDiagnostics\LiveVisualTree.cpp` is an existing framework diagnostics boundary to investigate | All twelve typed queries; no UI/WinRT dependency or native project in P01 |
| 2. Windows 11 x64/ARM64, one process instance | RuntimeCapabilities architecture/build/capability versions; semantic ProcessInstanceKey; collector Process | `dxaml\xcp\core\dll\xcpcore.cpp` `CCoreServices::Create` / `Initialize`; future OS process discovery | GetSession; never join PID alone; min OS 22000, API declarations 26100 only behind capability checks |
| 3. Packaged/unpackaged launch or attach, OS collector semantics | Process Start/Attach/Exit, nullable creation QPC/FILETIME, CPU/GPU/memory Metric source/interval/version | Future packaged activation / process creation collector, process handle creation identity, documented OS process-time/working-set/private-usage and GPU engine collectors | GetSession/GetMetrics/GetStartupTimeline; missing pre-attach history explicit; no launcher/collector implemented |
| 4. New fre telemetry independent of current events | New runtime GUID, schema 1, provider/keyword gating, explicit lifetime IDs | Object/device/frame/tree/resource/allocation sites below | Every consumer; actual provider/hook implementation and fre overhead measurements later |
| 5. Creation to first ten submitted XAML frames total | Process CreationQpc, Frame Attempt/Submitted/Failed/AttemptCompleted/StandaloneCommit, root/dispatcher, independent submission ID | `xcpcore.cpp` `CCoreServices::Tick` / `SubmitPrimitiveCompositionCommands` primary call to `DCompTreeHost::CommitMainDevice`; actual underlying device commit in `components\comptree\DCompTreeHost.cpp` | GetStartupTimeline/GetFrames; no-op S_OK is not submit; post-commit failure does not retract; no standalone/debug/tick/display counting or idle forcing |
| 6. CPU/GPU/private WS/private commit/scoped XAML/connected UIElements/canonical IVisual counts | Metric typed units + quality, ObjectLifetime, TreeEdge, Correlation, Allocation, Resource backing | OS collectors; `uielement.cpp` EnterImpl/LeaveImpl; `HWCompNodeWinRT.cpp` EnsureVisual/CreateWUCSpineVisual; `XcpAllocation.cpp` allocation/free/resize | GetMetrics/GetResourceUsage/trees; all metrics distinct; WinUI hooks do not establish all-process GPU resource attribution |
| 7. Lightweight vs full heap artifacts, immutable metadata, compare/roots | CaptureBoundary/Chunk; SnapshotMetadata Kind/State/versions/hash; OwnershipEdge/RetentionPath evidence; 1–64 snapshot IDs vs baseline | Future private runtime bridge visits covered roots/dispatchers; allocator/resource sites below; separate native/managed heap providers | GetSnapshots/CompareSnapshots/GetRetentionPaths; proven roots vs uncertain retention; arbitrary native bytes/COM release/managed addresses do not prove ownership |
| 8. Same live/offline normalization, no offline target commands | EventCodec.Normalize + ProducerStreamKey; SessionMode.Offline; stored artifacts | Future live ETW adapter and offline ETL adapter feed the same owned codec | All read-only queries; command executor must reject offline; saved artifacts remain viewable |
| 9. ETW plus authenticated named pipe, diagnostics-injected TAP/private bridge | Length-prefixed TAP protocol version/type/request ID/bounded typed body, capabilities/capture IDs/nonce | Future IXamlDiagnostics injection + private runtime bridge; framework diagnostics boundary above | Target controller later; no arbitrary remote commands, nonce alone is not auth |
| 10. One bounded processing owner for histories/trees/correlations/resources | IngestOrdinal/source sequences/epochs, typed topology/deltas/correlations, explicit backing IDs | All producers below; future dedicated processing thread | GetObjectHistory/GetXamlTree/GetCompositionTree/GetCorrelations/GetResourceUsage; no concurrent UI ownership or fake processor in P01 |
| 11. SQLite WAL/single writer/temporal state/watermarks/typed cancellable paged API + CLI | CommittedWatermark, ResultContext, PageRequest/PageInfo, twelve typed request/results, explicit CancellationToken | Future store writer after normalized ingestion; no runtime hook needed | P02 storage; deterministic pages frozen to watermark; no SQL/chat/MCP or fake success API |
| 12. Honest quality/capability versions/loss/epochs/identity/history/resource stops | DataQuality axes, Missing TypedValue, Loss interval, baseline chunk counts/sequences, SessionStatus/StopReason | Runtime/collector Loss + capabilities; allocator baseline; future queue/store limits | Every result; preserve committed history; stop visibly instead of silent eviction, merge, zero-fill or decoder fallback |
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
| Core/peer and type association | `dxaml\lib\DependencyObject.cpp`; `components\DependencyObject\dependencyobject.cpp`; existing diagnostics boundary `components\xamlDiagnostics\LiveVisualTree.cpp` | Further P04 bridge/peer creation investigation must identify exact post-construction association hooks; do not infer peer identity from core pointers or promise constructor-time final type |
| Connected UIElement/root membership | `core\core\elements\uielement.cpp` `CUIElement::EnterImpl` (1299), `LeaveImpl` (1740); `core\dll\VisualTree.cpp` | Root/dispatcher-scoped connection edges and connected UIElement counts; a leave is not destruction |
| Ordered children | `components\Collection\UIElementCollection.cpp` append/insert/remove/clear operations | Emit old/new order/membership at successful mutations, not from an unordered walk |
| Content/template/popup relationship kinds | `core\core\elements\ContentControl.cpp` content mutation; `Control.cpp` template application; `Popup.cpp` popup transitions; `components\elements\Popup.cpp` | P04 bridge must instrument successful relationship changes with explicit root/dispatcher; include popup roots, content and template kinds; static visual-children-only traversal is incomplete |
| Initialization and render attempt | `core\dll\xcpcore.cpp` `CCoreServices::Initialize` (3657), `Tick` (4336), `SubmitPrimitiveCompositionCommands` (7179) | Initialization/attempt boundaries, not frame-submission counters; process creation belongs to OS collector, not Initialize |
| Measure/arrange boundaries | `core\layout\LayoutManager.cpp` `CLayoutManager::EnterMeasure` (166), `ExitMeasure` (180), `EnterArrange` (189), `ExitArrange` (203), `UpdateLayout` | Pair operation IDs in authoritative epoch/thread; preserve failure and incomplete intervals |
| Qualifying primary commit | `core\dll\xcpcore.cpp` primary `CommitMainDevice` call around 7265; `components\comptree\DCompTreeHost.cpp` `CommitMainDevice` (1130) actual `m_spMainDevice->Commit()` | Must carry primary XAML-attempt intent into the actual commit hook; emit Submitted immediately after successful actual commit. Null main device returns S_OK with no commit. Fallible PostCommit/layout work and optional debug-counter commit around 7315 are separate observations |
| Standalone commits | `components\comptree\DCompTreeHost.cpp` `RequestMainDCompDeviceCommit`, `CommitMainDevice`; nonprimary callers | Record commit diagnostics without SubmittedFrameId; do not infer primary status from being inside a tick |
| CompNode/canonical IVisual identity/topology | `components\comptree\HWCompNodeWinRT.cpp` `HWCompTreeNodeWinRT` constructor/Create, `EnsureVisual` (310), `CreateWUCSpineVisual` (376), `InsertWUCSpineVisual` (419), `GetWUCVisual`; `DCompTreeHost.cpp` `CreateContainerVisual` (1241), `SetRootForCorrectContext` | Track CompNodes separately from canonical IVisual COM identity; aliases/hand-off/spine/content visuals can correlate many-to-many. External visual mutations require explicit capability limits or additional observation hooks |
| Typed visual state and XAML/visual/resource correlations | `components\comptree\HWCompNodeWinRT.cpp` visual/property writes; `core\hw\VisualContentRenderer.cpp` `RenderPrimitive`, `GetCompositionBrush` | Record known typed property deltas and observed resource assignment; no claim to full external compositor state or animation evaluation |
| Device birth/loss/epochs | `plat\win\desktop\D3D11DeviceInstance.cpp` `CD3D11DeviceInstance::EnsureResources` (165), `ReleaseResourcesOnDeviceLost` (313); `d3d11device.cpp` `CD3D11Device::Create`, `EnsureResources`, `ReleaseScratchResources` | Separate device epochs and ownership scopes; device loss must invalidate incomplete resource/backing state |
| Surface resource/backing/rebind/resize | `plat\win\desktop\DCompSurface.cpp` Create overloads (33/75/110), InitializeSurface (142/226), ReleaseLegacyDCompResources (352), Lock/Unlock/QueueUpdate; `d3d11device.cpp` graphics resource API boundaries | Distinguish logical resource IDs and unique backing allocations; report measured API allocation sizes only where authoritative; dimensions are estimates, not residency |
| D2D creation/share | `plat\win\desktop\D2DAcceleratedRT.cpp` `CD2DRenderTarget<T>::CreateBitmap` (183), CreateSolidColorBrush (247), CreateBitmapBrush (286+), CreateSharedBitmapBrush (408) | Observe backing sharing and scoped ownership/release; WinUI relinquishing a COM reference does not prove final external destruction |
| Composition brush/cache ownership | `components\brushes\WUCBrushManager.cpp` EnsureResources (43), ReleaseDCompResources (81), ReleaseGraphicsDevice (92), GetMaskBrush/GetNineGridBrush/GetSurfaceBrush/GetColorBrush | Observe creation/rebind/cache ownership, close and release distinctly; share backing IDs across aliases |
| Traced allocator requested/actual/baseline coverage | `components\allocation\lib\XcpAllocation.cpp` OSMemoryAllocateFailFast (174), OSMemoryAllocateZeroMemoryFailFast (212), OSMemoryAllocateNoFailFast (250), OSMemoryResize (282), OSMemoryFree (343), OSMemoryGetBlockSize (40) | Preserve realloc failure/original lifetime, new allocation ID on successful reallocation, requested vs actual bytes. Never call block-size or build payloads while keyword is disabled. Scope is this allocator, not all native heaps |
| Baseline/snapshot per-dispatcher traversal | Future private bridge over roots, object/association/topology/resource/allocator sites above | Begin/chunk/end with capture IDs, bounded typed state, source sequences and per-root/dispatcher boundaries. Late attach begins known presence only; no synthetic historical births |
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
* Before production rollout: measure provider-off and keyword-on overhead in
  fre x64/ARM64, prove no payload work while disabled, exercise loss/reuse,
  queue/store limits and replay equivalence, compare OS samples against the
  declared semantics, and verify Windows 11 capability fallback.
