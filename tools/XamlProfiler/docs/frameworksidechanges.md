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
every point one of those trees forms or breaks an edge**, and to **enrich** each
"an element/visual just appeared" event with enough metadata (label, peer
handle, template flag, full property string) that the profiler can render a
faithful, human-readable, cross-linked view — purely from the event stream.

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
| `XamlTree` | `0x01` | `ChildAdded`, `ChildInserted`, `ChildRemoved`, `ElementEnteredTree`, `ElementLeftTree`, `PeerAssociated`, `ContentChanged`, `PopupOpened`, `PopupClosed` | Labels, peer handles, source info |
| `CompTree` | `0x02` | `CompPeerLinked`, `CompPeerUnlinked`, `CompNodeChildInserted`, `CompNodeChildRemoved` | Pick-overlay marking |
| `WucTree` | `0x04` | `WucVisualChildInserted`, `WucVisualChildRemoved`, `WucVisualChildrenCleared`, `WucVisualRootSet`, `WucVisualRootCleared` | `IVisual` QI and property serialization |
| `Surfaces` | `0x08` | `DCompSurfaceMemoryChanged`, `VisualSurfaceChanged` | Surface size and id reads |
| `SurfaceTextures` | `0x10` | `DCompSurfaceTextureObserved` | QI to the D3D texture on every draw |
| `HeapSnapshots` | `0x20` | `XamlHeapSnapshot` | Allocator counter reads on element enter/leave and surface changes |
| `GpuSnapshots` | `0x40` | `GpuMemorySnapshot` | D3D device lock and DXGI video-memory query on surface changes |

The live profiler app enables `0x07` (the three tree groups). The tree groups carry
deltas, so a session has to start before the UI it wants to see is built.

### Gates

Every instrumented site checks only its own group with the inline
`XamlProfilerTracing::IsEnabled(XamlProfilerKeywords::<Group>)`, which is
`TraceLoggingProviderEnabled(provider, WINEVENT_LEVEL_VERBOSE, keywords)` — a load,
compare and branch. Sessions below verbose level would drop every event, so they
don't turn the gates on. Argument work (labels, peer handles, core size, source info)
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
| `ElementEnteredTree` | `ElementId, ParentId, IsLive, Label, IsTemplateChild, PeerHandle, CoreSize, SourceFileUri, SourceLine, SourceColumn` | `CUIElement::EnterImpl` | Element entered the live tree (authoritative parent source). `CoreSize` is the core object's allocation size in Debug builds and 0 (unknown) in other builds |
| `ElementLeftTree` | `ElementId, ParentId, IsLive` | `CUIElement::LeaveImpl` | Element left the live tree |
| `PeerAssociated` | `ElementId, PeerHandle` | `DXamlCore::GetPeerPrivate` (peer creation) | A DXaml peer was just bound to a core element — **lazily back-fills** the peer handle for nodes that traced `PeerHandle=0` before the peer existed |
| `ContentChanged` | `ParentId, OldContentId, NewContentId, NewContentLabel, NewContentPeerHandle` | `CContentControl` content set | `ContentControl.Content` replaced |
| `PopupOpened` | `PopupId, ChildId, PopupLabel, ChildLabel, ChildPeerHandle` | `CPopup::Open` | Popup opened, child entered the tree (**two** labels) |
| `PopupClosed` | `PopupId, ChildId` | `CPopup` close | Popup closed |

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
| `WucVisualChildInserted` | `ParentVisualId, ChildVisualId, OwnerCompNodeId, Index, VisualType, Properties` | A WUC visual was inserted under a parent visual |
| `WucVisualChildRemoved` | `ParentVisualId, ChildVisualId` | A WUC visual was removed from its parent |
| `WucVisualChildrenCleared` | `ParentVisualId` | A parent's children were bulk-removed (`RemoveAll`) |
| `WucVisualRootSet` | `VisualId, TargetId, OwnerCompNodeId, VisualType, Properties` | A visual became a composition-target/island root (`put_Root`) |
| `WucVisualRootCleared` | `TargetId` | A composition-target/island root was cleared |

**Removal/clear events carry no label/handle/properties on purpose** — the
consumer already learned those from the matching introduce event and keys
everything by pointer identity.

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

- Added a trailing `PCWSTR` to **6 introduce events** (`ChildAdded`,
  `ChildInserted`, `ElementEnteredTree`, `ContentChanged`, `PopupOpened` ×2,
  `CompPeerLinked`).
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
handle was appended to the same 6 introduce events. The helper is **read-only**
(uses non-creating `GetDXamlPeer()`), so lazy-peer elements legitimately report
`0` until their peer exists ("first non-zero wins" on the consumer side).

### Lazy peer handles — the `PeerAssociated` event

Many elements (e.g. `ContentPresenter`, `ClientAreaPresenter`, and anything whose
DXaml peer is created on demand) have **no peer yet** when they first enter the
tree, so `ElementEnteredTree`/`ChildAdded` trace `PeerHandle = 0`. The peer is
created later, and there was no signal for that moment — the node would stay
un-highlightable forever.

**Fix:** a dedicated `PeerAssociated(ElementId, PeerHandle)` event fired from the
peer-creation path in `dxaml/xcp/dxaml/lib/DXamlCore.cpp`, **immediately after
`SetDXamlPeer`** binds the new peer to the core object. It computes the handle
with the *same* `XamlProfilerGetPeerHandle` helper (so the value matches every
other event) and only emits when the handle is non-zero. The consumer's
`SetPeerHandle` ("first non-zero wins") then upgrades that element's logical /
visual / comp nodes from `0` to the real handle — so a Ctrl+Click can highlight
it live even though its peer didn't exist at enter time.

This is the authoritative "the peer just got attached" signal; the peer-handle
fields on the six introduce events remain an *eager* fast-path for elements that
already had a peer.

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

## 8. The densest tree — full WUC (`IVisual`) tree + property payloads

**Goal:** expose the third/densest tree — the raw `Windows.UI.Composition`
`IVisual` tree that actually paints pixels (the same tree `DCompTreeHelper`
dumps as `<VisualTree>`) — including the **full ~45-property payload** of every
inserted visual.

### New component: `WucVisualTreeProfiler`

- `dxaml/xcp/components/comptree/inc/WucVisualTreeProfiler.h` **(new)** — a
  namespace of five free functions the call sites invoke, taking visuals as
  `IUnknown*`:
  `NotifyChildInserted / NotifyChildRemoved / NotifyChildrenCleared /
  NotifyRootSet / NotifyRootCleared`.
- `dxaml/xcp/components/comptree/WucVisualTreeProfiler.cpp` **(new)** — implements
  them. Every function **early-returns unless `WucVisualTreeProfiler::IsEnabled()`**
  (the inline `WucTree` keyword check), so building the property string costs nothing
  unless a session has that group enabled.
  Internally it ports, byte-for-byte, `DCompTreeHelper`'s serialization
  (`BuildPropertiesString`, enum-name helpers, `GetWucVisualTypeName`) and
  normalizes every id by QI-ing to `WUComp::IVisual*` so ids match the live dump.
- The property string is a single `PCWSTR` in the **exact XML-attribute format
  `DCompTreeHelper` uses**, keeping the events within `PARAM6`.

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
- `CompPeerLinked` / `CompPeerUnlinked` / `CompNodeChild*` / `WucVisual*` all gate
  on `!IsProfilerSuppressed()`; `NotifyChildInserted` / `NotifyRootSet` take the
  owning `HWCompNode*` so they can check the mark. The mark lives and dies with the
  comp node, so there is no global registry, no lock, and no teardown work in
  `RemoveCompositionPeer` — a reused pointer address can never inherit it.

**Detailed doc:** `resources/pick-overlay-etl-suppression.md`.

---

## 11. Complete list of framework files changed

| File | Role in the change |
| --- | --- |
| `dxaml/Xaml.Cpp.Props` | `XamlProfilerEnabled` master switch (on for every configuration; see `activateframework.md`) |
| `dxaml/xcp/components/base/inc/XamlProfilerTracing.h` | Provider declaration, keyword groups, inline `IsEnabled(keywords)` gate, emitter declarations (**all** event schemas); `XamlProfilerGetPeerHandle` / `XamlProfilerGetCoreSize` declarations; inline `XamlProfilerTraceHeapSnapshot` (Debug builds) |
| `dxaml/xcp/components/base/XamlProfilerTracing.cpp` | **New.** Provider definition and the out-of-line emitters |
| `dxaml/xcp/components/base/lib/Microsoft.UI.Xaml.Base.vcxproj` | Registered `XamlProfilerTracing.cpp` in `ClCompile` |
| `dxaml/xcp/dxaml/dllsrv/winrt/dllentry.cpp` | Registers / unregisters the provider with the DLL |
| `dxaml/xcp/components/comptree/inc/WucVisualTreeProfiler.h` | **New.** WUC notify API, inline `WucTree` gate, pick-overlay name marker |
| `dxaml/xcp/components/comptree/WucVisualTreeProfiler.cpp` | **New.** WUC serialization, `IVisual` identity normalization, owner-node suppression check |
| `dxaml/xcp/components/comptree/lib/Microsoft.UI.Xaml.CompTree.vcxproj` | Registered the new `.cpp` in `ClCompile` |
| `dxaml/xcp/components/comptree/HWCompNodeWinRT.cpp` | Sync comp-node events; WUC spine/inter-node/hand-in/DManip/reparent/bulk-clear edges |
| `dxaml/xcp/components/comptree/DCompTreeHost.cpp` | WUC in-proc island + Xaml-island root attach (`WucVisualRootSet`) |
| `dxaml/xcp/core/hw/hwwalk.cpp` | WUC sprite/leaf visual insert chokepoints |
| `dxaml/xcp/core/hw/hwcompnode.h` | Per-node pick-overlay suppression bit (`MarkProfilerSuppressed` / `IsProfilerSuppressed`) |
| `dxaml/xcp/core/hw/hwcompnode.cpp` | Async comp-node events; render-data node bulk clear |
| `dxaml/xcp/core/core/elements/uielement.cpp` | `ChildAdded`/`ChildInserted`/`ElementEnteredTree`/`CompPeerLinked` call sites and their out-of-line argument helpers; `XamlProfilerGetPeerHandle` / `XamlProfilerGetCoreSize` definitions; pick-overlay suppression hooks |
| `dxaml/xcp/core/inc/uielement.h` | Declaration of the out-of-line `TraceProfilerElementEnteredTree` member helper |
| `dxaml/xcp/core/core/elements/ContentControl.cpp` | `ContentChanged` call site |
| `dxaml/xcp/core/core/elements/Popup.cpp` | `PopupOpened`/`PopupClosed`; windowed-popup island WUC root tree |
| `dxaml/xcp/dxaml/lib/DXamlCore.cpp` | `PeerAssociated` emit after `SetDXamlPeer` — lazy peer-handle back-fill (§6) |
| `dxaml/xcp/core/hw/VisualContentRenderer.cpp` | `VisualSurfaceChanged` call sites |
| `dxaml/xcp/plat/win/desktop/DCompSurface.h` / `.cpp` | `DCompSurfaceMemoryChanged` / `DCompSurfaceTextureObserved` / snapshot emits behind inline per-group gates |
| `dxaml/xcp/components/allocation/inc/XcpAllocation.h`, `lib/XcpAllocation.cpp` | `XCP_COUNT_OUTSTANDING_ALLOC` and the allocator counter getters used by `XamlHeapSnapshot` |

---

## 12. Lifetime, safety & compatibility (applies to all of the above)

- **String lifetime:** `TraceLoggingWrite` copies each `PCWSTR` into the ETW
  record **synchronously**, so a value valid for the full-expression (an
  `xstring_ptr::GetBuffer()` returned by value, or a stack property buffer) is
  safe. No string outlives the notify call.
- **Pointer ids** are `reinterpret_cast<uint64_t>` of objects alive at the emit
  point (the mutation already succeeded).
- **Cost when idle:** each instrumented site is one inline keyword check
  (`XamlProfilerTracing::IsEnabled(<group>)`); the label / peer-handle / core-size /
  source-info reads and the `TraceLoggingWrite` itself are in out-of-line helpers and
  emitters that run only while that group is enabled. No behavioral runtime logic
  changes — only the ETW payload is enriched.
- **Release data:** builds without allocator counters never emit `XamlHeapSnapshot`
  and report `CoreSize` 0 (unknown) instead of reading the heap with a base-class
  pointer.
- **Compatibility:** the provider GUID moved from the early placeholder
  `{A1B2C3D4-E5F6-4A5B-9C8D-7E6F5A4B3C2D}` to the name-hash GUID above; event names,
  fields and field order are unchanged and only the keyword is new. Self-describing
  TraceLogging keeps payload additions backward- and forward-compatible. This matters
  because the runtime ships via the Windows App SDK NuGet, so old DLLs coexist for a
  long time.

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
