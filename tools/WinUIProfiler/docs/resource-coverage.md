# Graphics resource observation coverage

This is the P06 **working coverage manifest**, not a claim that P06 or GPU
memory accounting is complete. The creation inventory at source revision
`1af995b98a02d86b1951c4babe35caa269a6d569` identified 1,620 source expressions
in 232 production files. Expressions include wrappers, factories, generated
visuals and callers of other factories; they are not 1,620 distinct allocations.
Current hooks observe the native device owner, logical `DCompSurface`
wrappers and driver-visible `SystemMemoryBitsDriver` staging-texture owners.
Solid-color composition-brush ownership slots are also instrumented; the
classification explicitly describes WinUI ownership, not final COM lifetime.
Existing diagnostic/debug telemetry is not an input.

## Implemented authoritative sites

Paths below are relative to `dxaml\xcp`; symbols identify the maintained sites
without relying on pre-instrumentation line numbers.

| Observation | Successful source boundary | End, reset or mutation | Meaning and limitations |
|---|---|---|---|
| D3D11 device creation | `plat\win\desktop\D3D11DeviceInstance.cpp`, `CD3D11DeviceInstance::CreateGraphicsDeviceFromAdapter`, after successful `D3D11CreateDevice` | `ReleaseResourcesOnDeviceLost` emits Lost and OwnershipRelease; destructor emits OwnershipRelease if still tracked | One shared native device-instance owner, not each `CD3D11Device` wrapper. A release does not prove final external COM destruction. All coverage is partial. |
| Existing native device | A surface observation supplies `CD3D11Device::GetProfilerDeviceOwner` | Resource selection gaps and producer epochs invalidate observer associations | Device BaselinePresent proves current association, not historical creation. The transient owner address is never a durable device ID. |
| Native-device composition surface wrapper | `plat\win\desktop\DCompSurface.cpp`, successful native-device `InitializeSurface` | Later successful initialization emits Rebind; successful `Resize` emits new dimensions | Classification is CompositionSurface. This covers the logical wrapper across underlying surface changes, not a driver allocation. |
| Externally supplied surface factory | Successful external-factory `DCompSurface::InitializeSurface` | Same wrapper rebind/resize/release/destructor boundaries | The factory does not prove the actual device. An isolated per-wrapper device-scope GUID keeps the required wire field well-formed, but no Device event, known hardware device, shared backing or byte count is invented. |
| No-hardware composition wrapper | `DCompSurface::CreateWithNoHardware`, after successful WinRT-surface activation | Later initialization emits Rebind; destructor ends the wrapper | Initial dimensions are zero; device/backing association is unknown. No GPU allocation is asserted. |
| WinUI legacy surface ownership release | `DCompSurface::ReleaseLegacyDCompResources`, after actual nonempty legacy references are released | Reinitialization restores observed ownership without creating another logical wrapper | OwnershipRelease is not physical backing destruction. `resetWucSurface=false` deliberately preserves the WinRT wrapper and may preserve backing. Repeated empty release produces no extra release event. |
| Logical wrapper destruction | `DCompSurface::~DCompSurface`, after legacy release and monitor removal | Removes the transient observer entry | Destroy ends only the tracked native wrapper. Its WinRT surface or externally held resources may outlive it. |
| Driver-visible staging-texture owner | `plat\win\desktop\D3D11DeviceInstance.cpp`, `CD3D11DeviceInstance::AllocateFromSysMemBitsPool`, after successful `SystemMemoryBitsDriver::Create` | `SystemMemoryBitsDriver::~SystemMemoryBitsDriver`, after the texture reference is actually released, emits OwnershipRelease and logical Destroy | Classification is Texture2D. The identity names the native owner wrapper, not unique physical GPU storage. A later mapping failure can legitimately destroy an already-created owner. Heap-only `SystemMemoryBits` objects are excluded. |
| Existing pooled staging owner | Successful reuse from the native system-memory-bits pool | Returning to the pool retains ownership and does not destroy the wrapper | First observed reuse emits partial BaselinePresent. Already-known reuse emits no duplicate presence, Create or Destroy. Device loss releases the old pool through its actual owners, not through synthetic observer cleanup. |
| Solid-color composition-brush ownership slot | `components\brushes\SolidColorBrush.cpp`, `CSolidColorBrush::GetWUCBrush`, after successful brush activation, interface query and initial color | `ReleaseDCompResources` and destructor reset the nonempty COM holder, then emit OwnershipRelease and logical-slot Destroy | Classification is CompositionBrush. The transient key is the holder's stable address, never a persisted COM address. Empty reset is silent. Later recreation receives a new qualified resource ID. External references can outlive the WinUI slot. |
| Existing solid-color brush reuse | `CSolidColorBrush::GetWUCBrush`, before returning the existing holder | First observed use emits partial BaselinePresent; already-known use emits no duplicate presence | Changing the color preserves the ownership-slot identity and does not emit Create or Rebind. Hardware association, backing and pixel dimensions are unavailable. This does not cover gradient, transition, surface or app-supplied brush owners. |
| Solid-color holder's XAML owner | The same successful creation/presence sites supply the owning `CDependencyObject*` as a transient join | Resource release/destruction retains the established qualified owner, even after the Core address has been destroyed or reused | Owner identifies the native solid-color brush, not every element using it. The existing bounded Core registry supplies an epoch-qualified XamlCore key; no pointer is persisted as an identity or held strongly. Resources-only selection keeps Core lifetime tracking active. |

Resource Create is emitted only at an observed successful initialization.
An already-known wrapper initialized again emits Rebind, not a second Create.
An unseen wrapper first encountered at Resize/Rebind emits BaselinePresent
followed by the mutation. Failed operations emit no successful mutation.
Device loss does not synthesize resource releases: actual wrapper release sites
must supply them.

Every current Resource observation has null `BackingAllocationId`, null
`AllocationBytes`, `SizeProvenance.Unknown` and partial quality. Solid-color
brush slots have the qualified XAML owner described above; other current
resource families still have unknown XAML owners.
Pixel dimensions cannot establish physical allocation size: formats, alignment,
tiling, atlases, virtual surfaces, residency and sharing remain unknown.
These events must not be summed as GPU committed/resident bytes or unique
physical allocations. Separately collected process/adapter GPU metrics do not
retroactively establish per-resource backing ownership.
Composition-brush observations have absent pixel dimensions, not manufactured
zero-sized textures. Their logical-slot counts are not counts of uniquely
allocated COM objects or GPU storage.

## Inventory families not yet covered

These concrete inventory sites remain gaps even when their callers eventually
create an observed `DCompSurface`. A caller and its wrapper are not two physical
allocations. All paths are repository-relative.

| Inventory family / representative sites | Current gap and required end/reset evidence |
|---|---|
| Other native textures: `dxaml\xcp\plat\win\desktop\d3d11device.cpp` upload intermediate, `windowsgraphicsdevicemanager.cpp`, `D2DPrintTarget.cpp` | Staging-owner coverage does not cover these other owners or establish shared-allocation identity, requested/actual byte size or complete cache replacement accounting. Must observe each holder's replacement/release and device generation, not infer final COM destruction. |
| D2D bitmaps/render targets: `dxaml\xcp\plat\win\desktop\D2DAcceleratedRT.cpp`, `dxaml\xcp\components\comptree\DCompTreeHost.cpp` | No bitmap wrapper/backing deduplication or owned render-target lifetime. A DXGI-surface view is not a new proven backing allocation. |
| Other WUC brushes/effects/lights: `dxaml\xcp\components\brushes\WUCBrushManager.cpp`, `LinearGradientBrush.cpp`, `controls\dev\Lights\MaterialHelper.cpp`, `controls\dev\ColorPicker\SpectrumBrush.cpp` | Solid-color ownership-slot coverage does not cover transition, gradient or surface-brush caches, source-surface rebind or XAML owner correlation. Changing a brush's surface must not recreate its brush identity. |
| Drawing brushes/geometries/command resources: `dxaml\xcp\plat\win\desktop\D2DAcceleratedRT.cpp`, `dxaml\xcp\core\core\elements\imagebrush.cpp` | No drawing-resource lifecycle or native-retention graph. CPU wrappers and drawing API objects do not establish GPU allocations. |
| Image decoding/CPU bitmaps: `dxaml\xcp\components\imaging\SvgImageDecoder.cpp`, `WicAnimatedGifDecoder.cpp`, `RenderTargetBitmapImplUsingSpriteVisuals.cpp` | No WIC buffer/decoder lifetime or unique storage accounting. These must remain separate from uploaded texture/backing accounting. |
| Surface callers/atlases: `dxaml\xcp\core\hw\hwtexturemgr.cpp`, `dxaml\xcp\components\graphics\AlphaMask.cpp` | The lower-level logical wrapper may be observed, but atlas suballocation, shared backing, owner and mask/texture associations are not. |
| Composition visuals, animations, property sets, clips, shapes and generated animated visuals | P05 visual observations are separate from physical resource accounting. No claim of complete resource coverage, allocation bytes or runtime-created internal animation resources. |
| Compositor initialization, content islands, input/interaction, DirectManipulation, text/font and media-sharing factories | External creation boundaries do not reveal hidden driver/compositor allocations or final ownership. Dedicated observation/release/reset coverage is still required. |
| App-supplied visuals/swap chains and external graphics libraries | WinUI accepting, wrapping or referencing an object is not evidence that WinUI created it. External lifetime and backing remain unknown unless independently proven. |

No complete ResourceBacking capability is advertised by these hooks.
Shared-texture deduplication, brush/bitmap coverage, owner correlations, physical
device-removal workloads and overhead measurements remain acceptance work.
The controlled in-process reset below exercises actual WinUI recovery and pool
retirement, but is not a claim of physical GPU removal or driver-memory parity.

## Lifetime, bounds and teardown

Registries are lazy, bounded to 65,536 device scopes and 65,536 logical resources,
and keep no strong graphics/COM references. A full producer disable creates a
new producer epoch. A Resources-only selection gap invalidates observer resource
and device scopes even when another listener keeps the producer epoch alive.
Runtime shutdown/reinitialization also drops observer associations. Any later
presence uses a new qualified identity; old history is not silently joined by
address. Numeric IDs are scoped by producer epoch, not globally unique numbers.

All hooks preserve last error. Closed/terminating runtime gates run before
eligibility queries, which can acquire locks and allocate query scratch memory.
Process termination abandons registries without traversal, locking or COM
release; the operating system reclaims the dying process. This does not create
synthetic Destroy events for surviving wrappers.

Exact-capacity tests fill each registry to 65,536 and require the next insertion
to fault explicitly with `HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW)`, without
eviction. Their owned ETW sessions flush every 256 writes so ETW buffer exhaustion
does not mask the registry limit. Deterministic post-reservation selection/epoch
races require canceled writes not to publish stale associations; later presence
has a fresh qualified identity. Real reserved-but-canceled source-sequence holes
are preserved, not relabeled as proven transport loss.

## Reproducible validation

From the implementation checkout:

```powershell
.\tools\WinUIProfiler\WinUIProfiler.Native.Tests\Run-GraphicsObservationTests.ps1
.\tools\WinUIProfiler\WinUIProfiler.Native.Tests\Run-GraphicsObservationTests.ps1 -Architecture arm64
dotnet test .\tools\WinUIProfiler\WinUIProfiler.Engine.Tests\WinUIProfiler.Engine.Tests.csproj --no-restore -c Debug --filter "FullyQualifiedName~IntegrityTests|FullyQualifiedName~Native_graphics_ETL"
.\initrun.ps1 -Flavor amd64fre msb /q dxaml\test\native\external\framework\profiler\ProfilerLateEnableHarness.vcxproj /p:UseXamlCompiler=false /p:PGOBuildMode=Off
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -Run -TestMode GraphicsResources
.\dxaml\test\native\external\framework\profiler\Stage-ProfilerLifecycleHarness.ps1 -Flavor amd64fre -Run -TestMode GraphicsReset
```

Use `amd64chk` for checked runtime execution. Build the matching local Product
before staging either host. The real workload renders text and two differently
sized WriteableBitmaps, then releases and destroys its wrappers; it requires
positive creation/release/destruction observations, staging-texture creation,
solid-color ownership-slot creation/destruction, qualified Core-owner joins and
zero ETW transport loss. Pooled textures may legitimately survive that workload.
An additional color-only update is bracketed by real QPC timestamps and must
not emit brush creation or rebind within that interval.
`GraphicsReset` uses the existing, matching `IXamlTestHooks` ABI to mark only
this owned process's device instance lost and request device-only recovery.
It requires actual loss cleanup, old-device staging-owner destruction and
replacement-device staging creation; it does not reset a system GPU.
Standalone tests cover reuse, failed operations, external/known-device rebind,
multiple wrappers sharing a native device, duplicate release, resource-only and
full selection gaps, runtime restart, exact registry bounds, emission races and
termination with an eligibility lock held by another thread. Managed tests
decode retained real ETLs and reopen the
SQLite events, preserving unknown physical bytes/backing and partial quality.
Owner-specific tests cover late Core baselines, address reuse before resource
retirement, Objects-only/Resources-only/full selection gaps and post-reservation
selection/producer-epoch races. Owner keys must remain in the same process and
producer epoch as their resource; release never invents a new owner lifetime.
ARM64 compilation is not native ARM64 execution.
