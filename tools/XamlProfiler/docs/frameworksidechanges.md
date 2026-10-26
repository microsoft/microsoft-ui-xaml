# Framework-Side Changes — Emitting Trees & Properties for the XAML Profiler

This document is the **single, consolidated summary of every change made to the
lifted XAML runtime** (`Microsoft.UI.Xaml.dll`, a.k.a. the *producer*) so that
an out-of-process tool — the **XamlProfiler** — can reconstruct and highlight
the four live UI trees without ever reading back into the target process.

It is deliberately a *map*, not a replacement, for the deep-dive docs. Each
section links to the detailed doc that covers that specific change.

> For **how the consumer (profiler app + tap) uses** all of this, see the
> companion doc `profilerworking.md`.

---

## 1. The core idea

The runtime already mutates four distinct trees as an app runs:

| Tree | Runtime type | Pointer identity used as node id |
| --- | --- | --- |
| **Logical** | `CUIElement*` (developer-authored subset) | `reinterpret_cast<uint64_t>(CUIElement*)` |
| **Visual** | `CUIElement*` (full tree) | same `CUIElement*` id space as Logical |
| **Composition** | `HWCompTreeNode*` | `reinterpret_cast<uint64_t>(HWCompTreeNode*)` |
| **WUC / IVisual** | `WUComp::IVisual*` | `reinterpret_cast<uint64_t>(IVisual*)` |

The framework change is to **fire a self-describing TraceLogging (ETW) event at
every point one of those trees forms or breaks an edge**, and to emit enough
metadata (labels, peer handles, template flags, visual properties) that the
profiler can render a faithful, human-readable, cross-linked view — purely from
the event stream. The events that fire on every tree change carry scalars only, so
the provider is cheap enough for retail builds; details that cost something to
compute come from separate granular events, emitted once per type or when the
detail changes (§3).

Nothing is ever read back out of the target process by the profiler; the tap
(see §9) is a separate, optional live-highlight channel.

---

## 2. The one ETW provider

All events go out on a single TraceLogging provider, declared in
`dxaml/xcp/components/base/inc/XamlProfilerTracing.h` and defined in
`dxaml/xcp/components/base/XamlProfilerTracing.cpp`:

- **Name:** `Microsoft-Windows-XAML-Profiler`
- **GUID:** `{26f28a5c-7d30-54c5-40f4-fff6d3beda7d}` — the EventSource-style hash of
  the name, so tools can also enable it as `*Microsoft-Windows-XAML-Profiler`.
- **Definition:** a plain `TRACELOGGING_DEFINE_PROVIDER` with **no provider group**,
  so only sessions that enable this provider turn the instrumentation on (Microsoft
  telemetry group sessions don't). It is registered in `InitializeDll` and
  unregistered in `DeinitializeDll` (`dllentry.cpp`).
- **Events:** one out-of-line (`noinline`) emitter per event in the
  `XamlProfilerTracing` namespace, each a `TraceLoggingWrite` at
  `WINEVENT_LEVEL_VERBOSE` with exactly one keyword. Event names, field names, field
  types and field order are the wire contract with the profiler tools.

### Keyword groups

Each marker group has its own ETW keyword. A session enables only the groups it
needs, and the work behind a group runs only while that group is enabled. A session
that passes no keywords (MatchAnyKeyword 0) receives every group.

| Keyword | Value | Events | Work behind the group |
| --- | --- | --- | --- |
| `XamlTree` | `0x01` | `ChildAdded`, `ChildInserted`, `ChildRemoved`, `ElementEnteredTree`, `ElementTypeInfo`, `ElementLeftTree`, `PeerAssociated`, `ContentChanged`, `PopupOpened`, `PopupClosed` | Labels and peer handles on the child, content and popup events; type name and core size per type, re-sent when a session starts |
| `CompTree` | `0x02` | `CompPeerLinked`, `CompPeerUnlinked`, `CompNodeChildInserted`, `CompNodeChildRemoved` | Pick-overlay marking |
| `WucTree` | `0x04` | `WucVisualCreated`, `WucVisualChildInserted`, `WucVisualChildRemoved`, `WucVisualChildrenCleared`, `WucVisualRootSet`, `WucVisualRootCleared`, `WucVisualPropertyChanged`, `WucVisualTransformChanged`, `WucVisualObjectChanged` | `IVisual` QI for identity and a QI ladder for the visual's kind; one event per property value XAML sets; a full property read for app-provided visuals |
| `Surfaces` | `0x08` | `DCompSurfaceMemoryChanged`, `VisualSurfaceChanged` | Surface size and id reads |
| `SurfaceTextures` | `0x10` | `DCompSurfaceTextureObserved` | QI to the D3D texture on every draw |
| `HeapSnapshots` | `0x20` | `XamlHeapSnapshot` | Allocator counter reads on element enter/leave and surface changes |
| `GpuSnapshots` | `0x40` | `GpuMemorySnapshot` | D3D device lock and DXGI video-memory query on surface changes |

The live profiler app enables `0x07` (the three tree groups), plus two
`Microsoft-Windows-XAML` events for element names (§3). The tree groups carry
deltas, so a session has to start before the UI it wants to see is built.

### Gates

Every instrumented site checks only its own group with the inline
`XamlProfilerTracing::IsEnabled(XamlProfilerKeywords::<Group>)`, which is
`TraceLoggingProviderEnabled(provider, WINEVENT_LEVEL_VERBOSE, keywords)` — a load,
compare and branch. Sessions below verbose level would drop every event, so they
don't turn the gates on. Argument work (labels, peer handles, type info, visual ids)
lives in out-of-line helpers next to the call sites, and the `TraceLoggingWrite`
expansion lives in the emitters, so neither lands in a hot function.

Because TraceLogging is **self-describing**, adding a field or a whole new event
never changes the GUID and never breaks an older consumer — old consumers ignore
fields they don't know, new consumers reading a missing slot get a safe default.
No `.wprp`/manifest change is required for new payload fields.

---

## 3. The complete event schema

### Logical / Visual (UIElement) tree — `XamlTree`

| Event | Params | Fired from | Meaning |
| --- | --- | --- | --- |
| `ChildAdded` | `ParentId, ChildId, Index, ChildLabel, IsTemplateChild, ChildPeerHandle` | `CUIElement::AddChild` | Child appended to a parent's children collection |
| `ChildInserted` | `ParentId, ChildId, Index, ChildLabel, IsTemplateChild, ChildPeerHandle` | `CUIElement::InsertChild` | Child inserted at an index |
| `ChildRemoved` | `ParentId, ChildId` | `CUIElement::RemoveChild` | Child removed (no label — consumer already has it) |
| `ElementEnteredTree` | `ElementId, ParentId, IsLive, IsTemplateChild, TypeIndex` | `CUIElement::EnterImpl` | Element entered the live tree (authoritative parent source). `TypeIndex` is the core object's `KnownTypeIndex`, the key into `ElementTypeInfo` |
| `ElementTypeInfo` | `TypeIndex, TypeName, CoreSize` | `CUIElement::EnterImpl`, before the first `ElementEnteredTree` of each type; re-sent by the provider's enable callback when a session starts | The core class name (what `GetDebugLabel()` falls back to without an `x:Name`) and the core object's allocation size in bytes, which is fixed per type. `CoreSize` is measured in Debug builds and is 0 (unknown) in other builds. It can repeat, and around a session start it can arrive after the first enters of its type (see below) |
| `ElementLeftTree` | `ElementId, ParentId, IsLive` | `CUIElement::LeaveImpl` | Element left the live tree |
| `PeerAssociated` | `ElementId, PeerHandle` | `DXamlCore::GetPeerPrivate` (peer creation, after the PeerTable lock is released) | A DXaml peer was just bound to a core UIElement. The only source of an `ElementEnteredTree` node's peer handle: it fires before the enter for elements whose peer exists first, and **lazily back-fills** elements whose peer is created later |
| `ContentChanged` | `ParentId, OldContentId, NewContentId, NewContentLabel, NewContentPeerHandle` | `CContentControl` content set | `ContentControl.Content` replaced |
| `PopupOpened` | `PopupId, ChildId, PopupLabel, ChildLabel, ChildPeerHandle` | `CPopup::Open` | Popup opened, child entered the tree (**two** labels) |
| `PopupClosed` | `PopupId, ChildId` | `CPopup` close | Popup closed |

`ElementEnteredTree` fires for every element, so it carries no label, peer handle or
source location. Consumers join those back by `ElementId` from granular events:

- **Class name and core size:** `ElementTypeInfo`, by `TypeIndex`. The runtime writes a
  type's row before the first enter of that type, then marks the type (one bit per
  `KnownTypeIndex`, never cleared), so later enters cost one bit test. When a session
  enables the provider, changes its level or keywords, or requests capture state, the
  provider's enable callback re-sends the row of every marked type, so a session that
  starts later still gets the rows for the types that entered before it. A UI thread
  that writes a row while a session starts checks, after it marks the type, whether a
  re-send began meanwhile, and if so writes the row again: either the re-send saw the
  mark or the thread saw the re-send. So a session gets the row of every type it sees
  enter, unless ETW drops it (a capture-state request sends it again). Rows can repeat,
  because re-sends also reach the sessions already listening and UI threads racing on a
  new type may each write one. Around a session start, the first enters of a type can
  arrive before its row, so consumers apply a late row to the nodes they already hold
  with that `TypeIndex`.
- **`x:Name`:** the `Microsoft-Windows-XAML` manifest provider
  (`{531a35ab-63ce-4bcf-aa98-f88c7a89e455}`, keyword `0x1`): `ElementSetNameInfo`
  (event 213) when a name is set, and `ElementCreatedInfo` (event 61), which clears
  any name left at a reused address. A session can enable just these two events with
  an event-ID filter. ETW applies that filter when the event is written, so the target
  still writes the keyword's other events (for example per-element measure and
  arrange) and ETW discards them.
- **Peer handle:** `PeerAssociated`.
- **XAML source location:** `Microsoft-Windows-XAML-Diagnostics`
  `ElementCreatedWithSourceInfo1`.

### Composition (HWCompTreeNode) tree — `CompTree`

| Event | Params | Meaning |
| --- | --- | --- |
| `CompPeerLinked` | `UIElementId, CompNodeId, ParentCompNodeId, ElementLabel, UIElementPeerHandle` | A UIElement got a comp node (bridges Element ↔ Composition) |
| `CompPeerUnlinked` | `UIElementId, CompNodeId` | A UIElement lost its comp node |
| `CompNodeChildInserted` | `ParentCompNodeId, ChildCompNodeId` | A comp node was parented under another |
| `CompNodeChildRemoved` | `ParentCompNodeId, ChildCompNodeId` | A comp node was removed from its parent |

### WUC (Windows.UI.Composition `IVisual`) tree — `WucTree`

| Event | Params | Meaning |
| --- | --- | --- |
| `WucVisualCreated` | `VisualId, VisualKind` | XAML created a WUC visual; resets any properties a destroyed visual left at the same address |
| `WucVisualChildInserted` | `ParentVisualId, ChildVisualId, OwnerCompNodeId, Index, VisualKind` | A WUC visual was inserted under a parent visual |
| `WucVisualChildRemoved` | `ParentVisualId, ChildVisualId` | A WUC visual was removed from its parent |
| `WucVisualChildrenCleared` | `ParentVisualId` | A parent's children were bulk-removed (`RemoveAll`) |
| `WucVisualRootSet` | `VisualId, TargetId, OwnerCompNodeId, VisualKind` | A visual became a composition-target/island root (`put_Root`) |
| `WucVisualRootCleared` | `TargetId` | A composition-target/island root was cleared |
| `WucVisualPropertyChanged` | `VisualId, Property, X, Y, Z, W` | XAML set a scalar or vector property; unused components are 0 |
| `WucVisualTransformChanged` | `VisualId, M11 … M44` | XAML set the visual's `TransformMatrix` |
| `WucVisualObjectChanged` | `VisualId, Property, ObjectId, TypeName` | XAML set (or cleared, `ObjectId` 0) an object-valued property such as `Clip`, `Brush` or `Shadow`; `TypeName` is the object's short runtime class name |

`VisualKind` and `Property` are the `WucVisualTreeProfiler::VisualKind` and
`WucVisualTreeProfiler::VisualProperty` enums in `WucVisualTreeProfiler.h`; both are
part of the contract and append-only.

**Removal/clear events carry no label/handle/properties on purpose** — the
consumer already learned those from the matching introduce event and keys
everything by pointer identity. Property events are keyed by `VisualId` alone and
can arrive before the visual is inserted, so the consumer keeps a property bag per
visual and attaches it when the visual enters the tree.

**Property events carry changes only.** The producer skips a scalar, vector or
transform value, or a clear, that repeats the last one it wrote for that visual;
object-valued sets are always written. It forgets what it wrote when the visual's
`WucVisualCreated` fires and whenever a trace session starts, stops or requests a
capture state, so a new session gets each value the next time XAML sets it. The
consumer keeps the latest value of each property, so the skipped repeats change
nothing it shows.

### Surfaces and memory — `Surfaces`, `SurfaceTextures`, `HeapSnapshots`, `GpuSnapshots`

| Event | Keyword | Params | Meaning |
| --- | --- | --- | --- |
| `DCompSurfaceMemoryChanged` | `Surfaces` | `SurfaceId, IsAllocation, EstimatedSizeBytes, Width, Height, PixelStride, IsVirtual` | A DComp surface was allocated, resized or released |
| `VisualSurfaceChanged` | `Surfaces` | `VisualId, SurfaceId, Role` | A rendered `IVisual` started or stopped using a surface for its brush (role 0) or mask (role 1) |
| `DCompSurfaceTextureObserved` | `SurfaceTextures` | `SurfaceId, ResourceId, Width, Height, MipLevels, ArraySize, Format, SampleCount, OffsetX, OffsetY` | The D3D texture DComp exposed for a draw |
| `XamlHeapSnapshot` | `HeapSnapshots` | `HeapHandle, IsPrivateHeap, OutstandingBytes, OutstandingAllocationCount, CumulativeAllocatedBytes, CumulativeAllocationCount, CumulativeDeallocationCount` | XAML allocator counters. Emitted only in builds that keep outstanding-allocation counters (`XCP_COUNT_OUTSTANDING_ALLOC`: Debug builds), never as zeros |
| `GpuMemorySnapshot` | `GpuSnapshots` | `LocalCurrentUsage, LocalBudget, LocalAvailable, NonLocalCurrentUsage, NonLocalBudget, NonLocalAvailable` | Driver-reported video memory for the process |

---

## 4. Enrichment layer 1 — Human-readable component labels

**Problem:** every node used to display as a bare `Element 0x…` address.
**Fix:** attach a short `PCWSTR` label built from the runtime's existing
`CDependencyObject::GetDebugLabel()` (returns `x:Name` if set, else the class
name — e.g. `"RefreshButton"`, `"Grid"`), zero-allocation in the common path.

- Added a trailing `PCWSTR` to **5 introduce events** (`ChildAdded`,
  `ChildInserted`, `ContentChanged`, `PopupOpened` ×2, `CompPeerLinked`).
- `ElementEnteredTree` fires for every element, so instead of a label it carries a
  `TypeIndex`: the class name arrives per type in `ElementTypeInfo`, and the
  `x:Name` from `Microsoft-Windows-XAML` `ElementSetNameInfo` (§3).
- Null-guarded call sites (`pNewContent`, `m_pChild`) pass `L""`.
- The comp node borrows its owning UIElement's label via `CompPeerLinked`.

**Detailed doc:** `resources/producer-side-component-labels.md`.

---

## 5. Enrichment layer 2 — `IsTemplateChild` (developer-only logical tree)

**Problem:** template-generated elements (Border/ContentPresenter inside a
Button template) and framework scaffolding polluted the logical tree, making it
identical to the visual tree.
**Fix:** add a `bool IsTemplateChild = (GetTemplatedParent() != nullptr)` to
`ChildAdded`, `ChildInserted`, and `ElementEnteredTree` so the consumer can route
template children to the **visual tree only**. (A second, consumer-side
infrastructure-name filter handles non-template scaffolding like
`XamlIslandRoot`/`PopupRoot`.)

**Detailed doc:** `resources/feature-is-template-child.md`.

---

## 6. Enrichment layer 3 — DXaml peer handle (live highlight bridge)

**Problem:** node ids are the core `CUIElement*`, but XAML Diagnostics (and the
tap that draws overlays) identify an element by its **DXaml peer `IInspectable`
identity** (`InstanceHandle`) — a *different* address. You can't ask a tap to
highlight a `CUIElement*`.
**Fix:** a new read-only helper

```cpp
uint64_t XamlProfilerGetPeerHandle(_In_opt_ const CDependencyObject* obj) noexcept;
```

(declared in `XamlProfilerTracing.h`, defined in `uielement.cpp`) mirrors
`HandleMap::GetHandle`: `GetDXamlPeer()` → `QueryInterface(IInspectable)` →
identity address, or `0` when there is no peer yet. A trailing `uint64_t` peer
handle was appended to the same introduce events that carry a label (§4). The
helper is **read-only** (uses non-creating `GetDXamlPeer()`), so lazy-peer elements
legitimately report `0` until their peer exists ("first non-zero wins" on the
consumer side).

### Lazy peer handles — the `PeerAssociated` event

Many elements (e.g. `ContentPresenter`, `ClientAreaPresenter`, and anything whose
DXaml peer is created on demand) have **no peer yet** when they first enter the
tree, so `ChildAdded` traces `PeerHandle = 0`. The peer is
created later, and there was no signal for that moment — the node would stay
un-highlightable forever.

**Fix:** a dedicated `PeerAssociated(ElementId, PeerHandle)` event fired from the
peer-creation path in `dxaml/xcp/dxaml/lib/DXamlCore.cpp`, **after `SetDXamlPeer`**
binds the new peer to the core object and the PeerTable lock is released. Computing
the handle QIs the peer, and for a composed peer (e.g. a C# subclass) that QI runs
app code, which mustn't run under that lock. It computes the handle
with the *same* `XamlProfilerGetPeerHandle` helper (so the value matches every
other event) and only emits for UIElements, when the handle is non-zero. The consumer's
`SetPeerHandle` ("first non-zero wins") then upgrades that element's logical /
visual / comp nodes from `0` to the real handle — so a Ctrl+Click can highlight
it live even though its peer didn't exist at enter time.

This is the authoritative "the peer just got attached" signal. The peer-handle
fields on the other introduce events remain an *eager* fast-path for elements that
already had a peer. `ElementEnteredTree` has no peer field, so for it
`PeerAssociated` is the only source, including for peers created before the element
enters the tree: the consumer holds those by `ElementId` until the enter.

**Detailed docs:** `resources/peer-handle-live-highlight-phase1.md` (emission),
`resources/peer-handle-live-highlight-phase2.md` (tap consumption).

---

## 7. Composition-tree completeness — sync comp-node events

**Problem:** the runtime has two comp-tree mutation paths, but only the async
(render-data) path emitted `CompNodeChild*` events. The **synchronous
sprite-visuals path** (`HWCompTreeNodeWinRT::InsertChildSynchronous` /
`RemoveSynchronous`, the modern default) mutated `m_children` silently, so the
profiler missed synchronously-inserted nodes and never removed stale ones.
**Fix:** emit `CompNodeChildInserted` after `InsertChildSynchronous` and
`CompNodeChildRemoved` in `RemoveSynchronous` (child reparenting is covered
automatically because each moved child re-enters via `InsertChildSynchronous`).
The remove emit is deliberately **not** in the shared `RemoveChildInternal` to
avoid double-firing with the async path.

**Detailed doc:** `resources/producer-side-sync-comp-node-events.md`.

---

## 8. The densest tree — full WUC (`IVisual`) tree + visual properties

**Goal:** expose the third/densest tree — the raw `Windows.UI.Composition`
`IVisual` tree that actually paints pixels (the same tree `DCompTreeHelper`
dumps as `<VisualTree>`) — including each visual's properties (offset, size,
opacity, transform, clip, brush, shadow, …).

### New component: `WucVisualTreeProfiler`

- `dxaml/xcp/components/comptree/inc/WucVisualTreeProfiler.h` **(new)** — the
  `VisualKind` and `VisualProperty` contract enums, the free functions the call
  sites invoke (taking visuals as `IUnknown*`), and the `XAMLPROFILER_WUC_*` macros:
  - structure: `NotifyChildInserted / NotifyChildRemoved / NotifyChildrenCleared /
    NotifyRootSet / NotifyRootCleared`, plus `NotifyHandInVisualInserted` and
    `NotifyHandOffVisualAdopted` for app-provided visuals;
  - creation and properties: `XAMLPROFILER_WUC_CREATED`, `_SCALAR`, `_VECTOR2/3/4`,
    `_TRANSFORM` and `_OBJECT`, placed right after the call that creates the visual
    or sets the value they report. They compile to nothing when the profiler is
    compiled out, so call sites include the header unconditionally, without
    `#ifdef`.
- `dxaml/xcp/components/comptree/WucVisualTreeProfiler.cpp` **(new)** — implements
  them. Every function **early-returns unless `WucVisualTreeProfiler::IsEnabled()`**
  (the inline `WucTree` keyword check), and the macros check it before evaluating
  their arguments, so none of this costs anything unless a session has that group
  enabled. Every id is normalized by QI-ing to `WUComp::IVisual*` so ids match the
  live dump; the insert and root-set events add the visual's kind from a QI ladder
  (most-derived first).
- **Properties are reported where XAML sets them**, with the value XAML already has
  in hand, so nothing is read back and an event fires only when XAML sets a value.
  Creation and property events come from `HWCompNodeWinRT.cpp` (the prepend,
  primary, content and drop-shadow visuals), `VisualContentRenderer.cpp`,
  `DCompTreeHost.cpp`, `Popup.cpp`, `hwcompnode.cpp`, `SwapChainPanel.cpp`,
  `XamlIslandRoot.cpp` and `uielement.cpp` (the hand-off visual).
- **App-provided visuals** (hand-in visuals from `SetElementChildVisual`, and
  hand-off visuals the app may have changed through `GetElementVisual`) have
  properties XAML didn't set, so `NotifyHandInVisualInserted` and
  `NotifyHandOffVisualAdopted` read every reported property once and emit them as
  property events. This is the only place the producer reads visual properties.
- Values that the app or an animation later sets directly on a XAML-owned visual
  aren't reported; the profiler shows the last value XAML set.

### Every WUC edge is instrumented

Call sites fire **after** the underlying `IVisualCollection` mutation / `put_Root`
succeeds, covering: intra-comp-node **spine** (prepend→primary→clip→content),
**inter-node** edges, **DManip/hand-in** siblings, **reparenting**,
**sprite/leaf** inserts, **bulk clears**, and **island roots** (in-proc, Xaml
island, windowed-popup).

**Detailed doc:** `resources/addingisvisualtree.md` (producer half + consumer half).

---

## 9. IVisual live-highlight support — in-process identity matching

To make a WUC node highlightable live (it has no DXaml peer and there is no
"find visual by pointer" API), the producer emits the normalized `IVisual*`
identity as the node id. The injected tap runs in the target process, so it can
DFS-walk the live tree and compare the ABI pointer of each candidate's `IVisual`
interface with that id. This value is a transient, same-process, same-interface
identity token: the profiler never dereferences it, and the producer suppresses
the event if the source object cannot be normalized to `IVisual`. An
`__xp_adorner` comment prefix marks the tap's own overlay so the producer **skips** it
(`IsProfilerAdorner`) and it never surfaces as a phantom node. App-provided
visual comments remain unchanged.

**Detailed doc:** `resources/ivisual-live-highlight.md`.

---

## 10. Pick-overlay suppression (producer-side, by comp-node identity)

The tap's "Pick from app" mode opens a XAML `Popup`+`Border` capture surface,
which XAML renders into its own comp node + WUC spine — otherwise emitted as
phantom nodes in the very tree being inspected. Suppression is producer-side:

- `HWCompNode` gained a one-bit **per-node suppression mark**
  (`MarkProfilerSuppressed` / `IsProfilerSuppressed`), packed next to
  `m_usesPlaceholderVisual` so the class size is unchanged; `WucVisualTreeProfiler`
  exposes the overlay-name marker constant.
- `CUIElement::EnsureCompositionPeer` marks the new comp node before the
  comp-node/WUC events fire when the element's `GetDebugLabel()` starts with the
  overlay name marker (`__xp_pick_overlay`, stamped by the tap) **or** its parent
  comp node is already marked — so suppression **propagates** down the subtree.
  This runs only while a session has the `CompTree` or `WucTree` group enabled.
- `CompPeerLinked` / `CompPeerUnlinked` / `CompNodeChild*` / `WucVisualChildInserted` /
  `WucVisualRootSet` all gate on `!IsProfilerSuppressed()`; `NotifyChildInserted` /
  `NotifyRootSet` take the owning `HWCompNode*` so they can check the mark. WUC
  creation and property events are keyed by the visual alone and aren't gated; the
  consumer never shows a visual that wasn't inserted. The mark lives and dies with the
  comp node, so there is no global registry, no lock, and no teardown work in
  `RemoveCompositionPeer` — a reused pointer address can never inherit it.

**Detailed doc:** `resources/pick-overlay-etl-suppression.md`.

---

## 11. Complete list of framework files changed

| File | Role in the change |
| --- | --- |
| `dxaml/Xaml.Cpp.Props` | `XamlProfilerEnabled` master switch (on for every configuration; see `activateframework.md`) |
| `dxaml/xcp/components/base/inc/XamlProfilerTracing.h` | Provider declaration, keyword groups, inline `IsEnabled(keywords)` gate, emitter declarations (**all** event schemas); `XamlProfilerGetPeerHandle` / `XamlProfilerGetCoreSize` declarations; the `ElementTypeInfo` type table (`DescribeType`, which takes the core's `TypeRowWriter`) and the enable callback that re-sends its rows when a session starts; inline `XamlProfilerTraceHeapSnapshot` (Debug builds) |
| `dxaml/xcp/components/base/XamlProfilerTracing.cpp` | **New.** Provider definition, the out-of-line emitters, the type table and its enable callback |
| `dxaml/xcp/components/base/lib/Microsoft.UI.Xaml.Base.vcxproj` | Registered `XamlProfilerTracing.cpp` in `ClCompile` |
| `dxaml/xcp/dxaml/dllsrv/winrt/dllentry.cpp` | Registers the provider (with the enable callback) and unregisters it with the DLL |
| `dxaml/xcp/components/comptree/inc/WucVisualTreeProfiler.h` | **New.** `VisualKind` / `VisualProperty` contract enums, WUC notify API, `XAMLPROFILER_WUC_*` creation and property macros, inline `WucTree` gate, pick-overlay name marker |
| `dxaml/xcp/components/comptree/WucVisualTreeProfiler.cpp` | **New.** WUC structure, creation and property emitters, `IVisual` identity normalization, visual-kind QI ladder, full property read for app-provided visuals, owner-node suppression check |
| `dxaml/xcp/components/comptree/lib/Microsoft.UI.Xaml.CompTree.vcxproj` | Registered the new `.cpp` in `ClCompile` |
| `dxaml/xcp/components/comptree/HWCompNodeWinRT.cpp` | Sync comp-node events; WUC spine/inter-node/hand-in/DManip/reparent/bulk-clear edges; creation and property events for the visuals each comp node owns |
| `dxaml/xcp/components/comptree/DCompTreeHost.cpp` | WUC in-proc island + Xaml-island root attach (`WucVisualRootSet`); creation and property events for the host's own visuals |
| `dxaml/xcp/core/hw/hwwalk.cpp` | WUC sprite/leaf visual insert chokepoints |
| `dxaml/xcp/core/hw/hwcompnode.h` | Per-node pick-overlay suppression bit (`MarkProfilerSuppressed` / `IsProfilerSuppressed`) |
| `dxaml/xcp/core/hw/hwcompnode.cpp` | Async comp-node events; render-data node bulk clear; creation and property events for media and swap-chain visuals |
| `dxaml/xcp/core/core/elements/uielement.cpp` | `ChildAdded`/`ChildInserted`/`ElementEnteredTree` (with `DescribeType` and the `ElementTypeInfo` row writer)/`CompPeerLinked` call sites and their out-of-line argument helpers; `XamlProfilerGetPeerHandle` / `XamlProfilerGetCoreSize` definitions; hand-off visual creation events; pick-overlay suppression hooks |
| `dxaml/xcp/core/core/elements/ContentControl.cpp` | `ContentChanged` call site |
| `dxaml/xcp/core/core/elements/Popup.cpp` | `PopupOpened`/`PopupClosed`; windowed-popup island WUC root tree and its creation and property events |
| `dxaml/xcp/core/core/elements/SwapChainPanel.cpp` | Creation event for the swap-chain sprite visual |
| `dxaml/xcp/core/core/elements/XamlIslandRoot.cpp` | Creation event for the island's root container visual |
| `dxaml/xcp/dxaml/lib/DXamlCore.cpp` | `PeerAssociated` emit after `SetDXamlPeer`, outside the PeerTable lock — the peer handle for `ElementEnteredTree` nodes, including lazy back-fill (§6) |
| `dxaml/xcp/core/hw/VisualContentRenderer.cpp` | `VisualSurfaceChanged` call sites; creation and property events for content visuals (sprites, shapes) |
| `dxaml/xcp/plat/win/desktop/DCompSurface.h` / `.cpp` | `DCompSurfaceMemoryChanged` / `DCompSurfaceTextureObserved` / snapshot emits behind inline per-group gates |
| `dxaml/xcp/components/allocation/inc/XcpAllocation.h`, `lib/XcpAllocation.cpp` | `XCP_COUNT_OUTSTANDING_ALLOC` and the allocator counter getters used by `XamlHeapSnapshot` |

---

## 12. Lifetime, safety & compatibility (applies to all of the above)

- **String lifetime:** `TraceLoggingWrite` copies each `PCWSTR` into the ETW
  record **synchronously**, so a value valid for the full-expression (an
  `xstring_ptr::GetBuffer()` returned by value, or a stack type-name buffer) is
  safe. No string outlives the notify call.
- **Pointer ids** are `reinterpret_cast<uint64_t>` of objects alive at the emit
  point (the mutation already succeeded).
- **Cost when idle:** each instrumented site is one inline keyword check
  (`XamlProfilerTracing::IsEnabled(<group>)`); the label / peer-handle / type-info /
  visual-id work and the `TraceLoggingWrite` itself are in out-of-line helpers and
  emitters that run only while that group is enabled. No behavioral runtime logic
  changes — only ETW events are added.
- **Cost with a session:** the events that fire on every tree change
  (`ElementEnteredTree`, `WucVisualChildInserted`, `WucVisualRootSet`) carry scalars
  only. Type details are emitted once per type, and re-sent when a session starts;
  names, peers, source locations and visual properties are emitted when they're set,
  not on every tree change.
- **Release data:** builds without allocator counters never emit `XamlHeapSnapshot`
  and report `ElementTypeInfo.CoreSize` 0 (unknown) instead of reading the heap with
  a base-class pointer.
- **Compatibility:** the provider GUID moved from the early placeholder
  `{A1B2C3D4-E5F6-4A5B-9C8D-7E6F5A4B3C2D}` to the name-hash GUID above, and each event
  now has a keyword. To make the provider cheap enough for retail builds,
  `ElementEnteredTree`, `WucVisualChildInserted` and `WucVisualRootSet` also dropped
  their label, peer handle, core size, source location, type string and property
  fields in favor of the granular events in §3, and gained `TypeIndex` /
  `VisualKind`. Every other event's name, fields and field order are unchanged.
  Self-describing TraceLogging keeps payload additions backward- and
  forward-compatible, and a consumer that reads fields by name sees a dropped field
  as missing. This matters because the runtime ships via the Windows App SDK NuGet,
  so old DLLs coexist for a long time.

---

## 13. Building & deploying the producer

```powershell
# Build the merged runtime DLL (smallest target containing all changed .cpp):
.\initrun.ps1 build.cmd mux /q
# Output: BuildOutput\bin\amd64chk\Product\Microsoft.ui.xaml.dll (amd64fre for Release)
```

Because `Microsoft.UI.Xaml.CompTree.vcxproj` and `Microsoft.UI.Xaml.Base.vcxproj`
gained **new** `ClCompile` items, a plain `bt` inner-loop build is not sufficient for
those files — use the MSBuild / `build.cmd mux` path (or build those lib projects first).

The target app loads the runtime from its NuGet-restored / MSIX-layout copy, **not**
from `BuildOutput`. To test a locally-built runtime, overwrite every
`Microsoft.ui.xaml.dll` in the sample's output (for dev-registered MSIX, the
`…\AppX\` layout folder the OS actually loads from). The profiler does **not**
need rebuilding when only the producer changes.

---

## 14. Related docs

| Doc | Covers |
| --- | --- |
| `profilerworking.md` | **How the profiler app + tap consume everything here** |
| `resources/producer-side-component-labels.md` | §4 labels |
| `resources/feature-is-template-child.md` | §5 `IsTemplateChild` |
| `resources/feature-resolved-summary-names.md` | Consumer use of labels in the history pane |
| `resources/peer-handle-live-highlight-phase1.md` | §6 peer-handle emission |
| `resources/peer-handle-live-highlight-phase2.md` | §6 tap consumption |
| `resources/producer-side-sync-comp-node-events.md` | §7 sync comp-node events |
| `resources/addingisvisualtree.md` | §8 full WUC tree (producer + consumer) |
| `resources/ivisual-live-highlight.md` | §9 in-process IVisual identity matching |
| `resources/pick-overlay-etl-suppression.md` | §10 pick-overlay suppression |
| `resources/visual-tree-cross-connection.md` | Consumer cross-tree linkage (uses these ids) |
