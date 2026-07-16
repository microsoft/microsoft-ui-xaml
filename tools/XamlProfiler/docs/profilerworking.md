# How the XAML Profiler & Tap Actually Work

This is the consumer-side companion to `frameworksidechanges.md`. That doc covers
what the **runtime (producer)** emits; this one covers **how the profiler app and
the tap turn that stream into four cross-linked, interactive trees** and how the
optional live cross-app highlight works end-to-end.

---

## 1. The big picture

```
 ┌──────────────────────────────────────────┐         ┌──────────────────────────────┐
 │  Target app (e.g. FolderTreeApp)         │         │  XamlProfiler.exe (WinUI 3)  │
 │                                          │         │                              │
 │  Microsoft.ui.xaml.dll  ── ETW events ──►│──ETW───►│  EtwListenerService          │
 │  (producer)                              │         │        │                     │
 │                                          │         │        ▼                     │
 │  ┌────────────────────────┐  named pipes │         │  ProfilerTreeStore           │
 │  │ XamlProfilerTap.dll     │◄────────────┼─────────┤   (4 trees + cross-links)    │
 │  │ (injected, optional)    │─────────────┼────────►│        │                     │
 │  └────────────────────────┘  tap↔profiler│         │        ▼                     │
 │   draws live highlight overlays          │         │  MainWindow: 4 TreeViews     │
 └──────────────────────────────────────────┘         └──────────────────────────────┘
```

Two **independent** channels:

1. **ETW (always on while listening)** — one-way, target → profiler. Rebuilds the
   four trees. Needs no injection; works even against a released runtime.
2. **The tap (optional, on-demand)** — a native DLL injected into the target via
   XAML Diagnostics, driven by two named pipes. Powers *live* highlight and
   "pick from app". Degrades gracefully when absent.

The whole four-tree model is a **pure function of the ETW event stream**, so it
rebuilds deterministically on every capture, regardless of event order.

---

## 2. Two projects in the solution

`XamlProfiler.sln` (solution platform **x64**) contains:

| Project | Language | Role |
| --- | --- | --- |
| `XamlProfilerApp` (`XamlProfiler.csproj`) | C# / WinUI 3 | The profiler UI + ETW listener + tap injector |
| `XamlProfilerTap` (`XamlProfilerTap.vcxproj`) | native C++ | The XAML Diagnostics tap injected into the target |

The C# project `ProjectReference`s the C++ project and copies `XamlProfilerTap.dll`
next to `XamlProfiler.exe` after build — **but only under full MSBuild**
(`'$(MSBuildRuntimeType)' == 'Full'`), because the .NET SDK's `dotnet build` runs
on MSBuild Core which can't compile a vcxproj. So:

- **Full feature (tap + app):** `MSBuild XamlProfiler.sln /restore /t:Build
  /p:Configuration=Debug /p:Platform=x64` (Visual Studio / full MSBuild).
- **App only (no tap):** `dotnet build .\XamlProfilerApp\XamlProfiler.csproj -c Debug`
  — live highlight degrades to "Tap DLL not found", everything else works.

---

## 3. Consumer file map

| File | Responsibility |
| --- | --- |
| `Services/EtwListenerService.cs` | Subscribe to the provider, snapshot payloads on the ETW thread, batch, dispatch to the store on the UI thread |
| `Models/ProfilerTreeStore.cs` | The heart — 4 tree models, get-or-create nodes, cross-tree links, highlight passes |
| `Models/TreeNode.cs` | One node in any tree: id, label, kind, link ids, `PeerHandle`, event history |
| `Models/WucVisualProperties.cs` | Strongly-typed parse of the `Properties` attribute string from WUC events |
| `Converters/*` | XAML value converters (badges, link-highlight brushes, etc.) |
| `MainWindow.xaml{,.cs}` | The 4 TreeViews, launch/listen, tap wiring, click gestures, exports |
| `Services/TapChannel.cs` | Managed injector: creates pipes, injects the tap, sends commands |
| `XamlProfilerTap/*` | The native tap (see §8) |
| `XamlProfiler.wprp` | WPR profile to record an `.etl` offline against the same provider |

---

## 4. The ETW listener — `EtwListenerService`

**Subscription.** `Start()` creates a real-time `TraceEventSession`
(`Microsoft.Diagnostics.Tracing`, TraceEvent library), enables the provider GUID
`{A1B2C3D4-…}` at `Verbose`, and pumps events on a dedicated background thread via
`_source.Process()`. Requires **Administrator** (ETW real-time sessions do).

**The recycled-buffer hazard (critical).** The `TraceEvent` instance and its
payload buffer are **reused** by the parser and become invalid the moment the
callback returns. So `OnEvent` **snapshots synchronously on the ETW thread**:
copies `EventName` and every `PayloadValue(i)` into an `object?[]`, then enqueues
`(name, payload)`. Reading the live `TraceEvent` from a later UI-thread lambda
would dereference recycled memory.

**Batching.** Events arrive far faster than the UI can absorb one-by-one. A
50 ms timer drains up to 500 queued events per hop (`FlushPending`), so many
mutations apply under one layout pass. At most one drain is queued on the
dispatcher at a time (`_flushScheduled` interlock).

**Process filter.** When a target PID is known, events from other processes are
dropped in `OnEvent`.

**Dispatch.** `Dispatch(name, payload)` switches on the event name to ~18
handlers. Payload slots are read with typed helpers that tolerate missing/null
slots (forward/backward compat): `U64` (note: TraceLogging `uint64_t` arrives as
`Int64`, so it's `unchecked((ulong)Convert.ToInt64(v))`), `Bool`, `I32`, `Str`.
Each handler mutates the store and records the event onto the node's history.

Example — `HandleChildAdded` reads
`parentId, childId, [index], childLabel, isTemplateChild, childPeerHandle`, then:
always `AddVisualChild`; if **not** template and **not** infrastructure, route to
the logical tree via `FindNearestLogicalAncestor`; finally `SetPeerHandle`.

**Lazy peer handles.** Elements with a lazily-created DXaml peer trace
`PeerHandle = 0` at enter time. The producer's `PeerAssociated(elementId,
peerHandle)` event fires later, when the peer is actually bound; its handler
(`HandlePeerAssociated`) simply calls `_store.SetPeerHandle(elementId,
peerHandle)`. Because `SetPeerHandle` is **first-non-zero-wins**, this upgrades
the element's logical/visual/comp nodes from `0` to the real handle, so it becomes
live-highlightable even though its peer didn't exist when it first appeared.

---

## 5. The tree store — `ProfilerTreeStore`

Holds four node dictionaries keyed by pointer id, four `ObservableCollection`
root lists the TreeViews bind to, plus reverse indexes for the cross-links.

### Get-or-create everywhere → event-order independence
Every mutator uses `GetOrCreate*Node(id, label)`. Because a node can be created
by whichever event mentions it first, a capture that starts mid-flight still
converges to the correct tree.

### The developer-only logical tree (three-layer filter)
1. **Template children** (`IsTemplateChild`, producer-side) → visual tree only.
2. **Infrastructure labels** (`IsInfrastructureLabel`, a static consumer-side set:
   `XamlIslandRoot`, `RootScrollViewer`, `PopupRoot`, `ClientAreaPresenter`, …) →
   visual tree only.
3. **Logical parent fix:** a developer element whose visual parent is
   template/infra is reparented to the nearest logical ancestor via
   `FindNearestLogicalAncestor`; if none, it becomes a logical root.

### Labels → readable names
`GetDebugLabel()` labels are stored per node and surfaced everywhere:
tree rows show `Button "RefreshButton" (0x…)`, and the detail pane's history
Summary column resolves ids to `Label (0xADDR)` via `ResolveName(id)` (looks up
logical → visual → comp). See `resources/feature-resolved-summary-names.md`.

### Display-name toggles (readability)
Two **static** `TreeNode` flags let the user declutter every row on the fly; each
node composes its `DisplayName` in `ComposeDisplayName()` from its label, id,
suffix, and these flags, so flipping a flag + calling
`ProfilerTreeStore.RefreshAllDisplayNames()` recomputes all four trees at once:

- `TreeNode.IncludeMemoryAddress` (default `true`) — when off, drops the `0x…`
  address, so rows read `Button` / `Element` instead of `Button (0x1D2F…)`.
- `TreeNode.ShowResponsibleElementAddress` (Composition-only, default `true`) —
  when off, drops the `→ Element 0x…` annotation on comp rows. The comp node
  stores the linked element in `ResponsibleElementId` (set by `LinkCompPeer`,
  cleared by `UnlinkCompPeer`) so the annotation can be toggled without re-linking.

The two `…Check_Click` handlers in `MainWindow.xaml.cs` wire the checkboxes to
these flags. (The old `[Live]` suffix on Visual/Logical enter rows was dropped in
the same UI pass.)

### The four-tree cross-connection (the key design)
Every cross-tree relationship reduces to two bridge ids:

- `elementId` (`CUIElement*`) — **shared** by Logical and Visual (trivial link).
- `compId` (`HWCompTreeNode*`) — Composition tree; bridged to the element by the
  explicit `CompPeerLinked` event, stored in the `_compByElement` reverse index.
- The **WUC** tree hangs off Composition via each visual's `OwnerCompNodeId`
  (`_wucByOwnerComp` reverse index).

Links are written **symmetrically the moment both sides exist** (whichever appears
second back-fills), in `GetOrCreateLogicalNode` / `GetOrCreateVisualNode` /
`LinkCompPeer` / `AddWucChild`. Details in `resources/visual-tree-cross-connection.md` and
the consumer half of `resources/addingisvisualtree.md`.

### WUC properties
`WucVisualProperties.Parse(propertiesString)` turns the producer's `key="value"`
attribute string (byte-identical to `DCompTreeHelper`) into an immutable model
(offset/size/transform/opacity/clip/brush/shadow/child-count/…). Every field is
best-effort with a default fallback, so producer schema drift never crashes the
consumer.

---

## 6. Nodes — `TreeNode`

One class for all four trees, distinguished by `TreeNodeKind`
(`Logical` / `Visual` / `Composition` / `WucVisual`). Carries:

- `Id` (pointer identity — `CUIElement*` / `HWCompTreeNode*` / `IVisual*`),
  `Label`, `DisplayName`, `Children`.
- Cross-link ids: `LinkedLogicalNodeId`, `LinkedVisualNodeId`, `LinkedCompNodeId`.
- `PeerHandle` — the DXaml peer `IInspectable` identity (0 = none). What the tap
  needs to highlight an element. **Not** the same as `Id`.
- `LinkHighlight` (`LinkHighlightKind`: `None` / `Peer` / `Path`) — the transient
  glow state the TreeView templates bind to during a Ctrl+Click.
- WUC-only: `VisualProperties`, `VisualType`, `IslandTargetId`.
- Composition-only: `ResponsibleElementId` (the linked UIElement, for the
  `→ Element 0x…` annotation). Plus the static toggles `IncludeMemoryAddress` /
  `ShowResponsibleElementAddress` and `ComposeDisplayName()` / `RefreshDisplayName()`.
- `RecordEvent(...)` history for the detail pane.

---

## 7. The UI — `MainWindow`

- **Four side-by-side TreeViews** (Logical, Visual, Composition purple-ish, IVisual
  purple `#9C27B0`), each bound to a store root collection, sharing the same item
  template and `OnTreeItemInvoked` gesture handler.
- **Launch and Listen** (`StartButton_Click`): choose an **.exe** (launched with
  `Process.Start`) or an **app package** (launched via
  `shell:AppsFolder\<AUMID>`, then the window-owner process is resolved). Then it
  starts an `EtwListenerService` filtered to that PID.
- **Spotlight new nodes:** the first settled batch after connect is the initial
  tree flood and is consumed silently as a baseline; only *subsequent* structural
  changes glow + auto-scroll. Composition tree auto-follows to the bottom.
- **Readability toggles:** an "include memory address" checkbox and a
  "responsible element address" checkbox (comp tree) flip the static `TreeNode`
  flags and call `Store.RefreshAllDisplayNames()` to recompute every row (§5).
- **Click gestures** in `OnTreeItemInvoked`:
  - **Plain click** — expand/collapse; clears any live overlay.
  - **Shift+click** — open the detail pane only.
  - **Ctrl+click** — the cross-tree highlight: `Store.HighlightLinkedFor(node)`
    glows the peer (bright) + ancestor breadcrumb (dim) in the **other three**
    trees; **and**, if a tap is connected, the live in-app highlight (§8/§9).
- **Downloads:** each tree can be exported (JSON) via its Download button.
- **Deep-node reveal:** because the TreeViews nest a `TreeViewItem`
  (`ItemsSource=Children`) inside the item template, containers only exist for
  root nodes; revealing a deep node walks the root→node chain, `StartBringIntoView`
  per level with a deferred layout pass.

`HighlightLinkedFor` resolves `elementId` + `compId` from the clicked node's kind,
then for **every tree except the source's own** marks the peer `Peer` and walks up
marking ancestors `Path` (with a `visited` cycle guard); for WUC it glows every
`IVisual` owned by that comp node (`_wucByOwnerComp`). The source tree is never
self-glowed (native selection already shows it).

---

## 8. The tap — live cross-app highlight

The tap (`XamlProfilerTap.dll`, CLSID `c9f2ef77-1b6a-4810-a490-6ea6a06bf7cb`) is a
**XAML Diagnostics tap**: a COM object (`XamlSnoopTap` implementing
`IObjectWithSite` + `IVisualTreeServiceCallback2`) injected into the target
process. It is a port of the WinUISnoop tap, built from source as part of the
solution.

### Injection & channel — `TapChannel.cs`
On the **Tap** button (deliberately on-demand, so the target window and its
`WinUIVisualDiagConnection1` diagnostics port are up — injecting too early fails
with `ERROR_NOT_FOUND 0x80070490`):

1. Create two anonymous **named pipes** (tap→profiler, profiler→tap).
2. `DuplicateHandle` the relevant ends into the target process.
3. `InitializeXamlDiagnosticsEx("WinUIVisualDiagConnection1", pid,
   "Microsoft.UI.Xaml.dll", <tap dll path>, <CLSID>, "{write:x} {read:x}")`
   (P/Invoked from `Microsoft.Internal.FrameworkUDK.dll`).
4. Read the tap's `{"TapType":"Connected",…}` line → `IsConnected = true`.

> **Version compatibility gotcha:** the injector's FrameworkUDK must be
> version-compatible with the target's mux, or injection fails `0x80070490`.

### What the tap does on `SetSite`
- Reads the two pipe handles from `GetInitializationData` (`"%p %p"`).
- Sends `Connected` back with its module handle.
- Calls `AdviseVisualTreeChange` on a background thread — this **enumerates the
  live visual tree and populates the diagnostics RuntimeObjectCache**, so the
  producer-emitted peer handles resolve to live elements with no extra command.
- Starts a reader thread parsing commands.

### Command protocol (profiler → tap, over the pipe)
| Command | Effect |
| --- | --- |
| `HIGHLIGHT: %p` | Resolve `InstanceHandle` → `FrameworkElement`, draw overlay `SpriteVisual`s over its bounds (`TransformToVisual(xamlRoot.Content)` + `ElementCompositionPreview.SetElementChildVisual`) |
| `HIGHLIGHT-VISUAL: %llx` | IVisual path (§9) — DFS the live comp tree, match `Comment == "xpid:<id>"`, add an in-place translucent child adorner |
| `CLEAR-HIGHLIGHT` | Remove element overlay + in-place adorner |
| `START-PICK` / `STOP-PICK` | Enter / leave app→profiler pick mode (§10) |
| `CLOSE` | Tear down; profiler then ejects the DLL via remote `CreateRemoteThread(FreeLibrary)` |

`TapChannel.Highlight(handle)` sends `handle.ToString("X16")` so the tap's
`swscanf_s(L"%p")` round-trips to the identical pointer. (Earlier diagnostic
commands — `DUMP-VISUAL-TREE`, `DUMP-XAML-ROOTS`, `TEST-GETELEMENTVISUAL`,
`GET-PROPERTIES`, `GET-SUBTREE-PROPERTIES` — and the property-retrieval plumbing
were removed in the UI-cleanup pass; the tap is now just highlight + pick.)

### Element vs Composition vs WUC highlight
- **Logical / Visual** node → `PeerHandle` → `HIGHLIGHT` (element overlay).
- **Composition** node → borrows its owning element's `PeerHandle` (highlights the
  element's box; the comp node has no peer of its own).
- **WUC IVisual** node → `HIGHLIGHT-VISUAL: node.Id` (§9). It has no peer.
- `PeerHandle == 0` and not WUC → just `ClearHighlight`.

---

## 9. IVisual live highlight — the `Comment` trick

A WUC visual has no DXaml peer and there is no "find visual by pointer" API, and
its `IVisual*` id belongs to the **target** process (unsafe to deref from the
profiler). So identity is carried in the visual's writable **`Comment`**: the
producer stamps every live visual `Comment = "xpid:<hex IVisual*>"` — the *same
hex* the profiler shows as `node.Id`. The tap's `HighlightVisual(id)` DFS-walks
the live composition tree (from
`ElementCompositionPreview.GetElementVisual(xamlRoot.Content)`), matches
`Comment == "xpid:<id>"`, and adds a translucent child `SpriteVisual` stamped
`Comment = "__xp_adorner"` with `RelativeSizeAdjustment {1,1}` (no coordinate
math). The `__xp_adorner` sentinel makes the producer **skip** the overlay so it
never appears as a phantom node. Full detail: `resources/ivisual-live-highlight.md`.

---

## 10. Pick from app (app → profiler)

The tap also supports the reverse direction: **`StartPick`** opens a full-window
transparent `Popup`+`Border` capture overlay on **every** live window (multi-window
aware), intercepting all pointer input so the app "freezes" for interaction.
Hover previews the element under the cursor; the mouse wheel drills through the
overlapping hit-test stack (`m_pickCandidates`); a click commits the element and
sends its handle back up the pipe. The profiler (`OnElementPicked` /
`OnVisualPicked`) then selects/scrolls to the matching node — deferring if the
element's `PeerHandle` (or picked `IVisual`) only arrives over ETW a frame later
(`PendingPickResolved` / `VisualSubtreeHighlightApplied`). The overlay's comp node
+ WUC spine are suppressed producer-side (§10 of `frameworksidechanges.md` /
`resources/pick-overlay-etl-suppression.md`) so it never pollutes the tree.

---

## 11. End-to-end walkthrough (Ctrl+Click a Button)

1. App runs → runtime emits `ElementEnteredTree`/`ChildAdded`/`CompPeerLinked`/
   `WucVisualChildInserted`… over ETW, each carrying label + peer handle +
   (for WUC) full properties.
2. `EtwListenerService` snapshots + batches them; `ProfilerTreeStore` builds the
   four trees and cross-links them; `MainWindow` renders four TreeViews with
   readable names.
3. User clicks **Tap** → `TapChannel` injects `XamlProfilerTap.dll`, which
   enumerates the live tree (peer handles now resolvable).
4. User **Ctrl+Clicks** the `Button` in the Visual tree:
   - `Store.HighlightLinkedFor` glows the same Button in the Logical, Composition
     and IVisual trees (peer bright, ancestors dim).
   - `_tap.Highlight(node.PeerHandle)` → `HIGHLIGHT: <X16>` → the tap resolves the
     handle and draws an overlay on the **live Button in the app**.

---

## 12. Offline captures & requirements

- **Live:** run `XamlProfiler.exe` **as Administrator** (ETW real-time session),
  then Launch and Listen. Tap is optional and same-integrity-level only.
- **Offline:** `XamlProfiler.wprp` records an `.etl` against the same provider via
  WPR for later analysis; the same reconstruction logic applies to a recorded
  stream.
- **Runtime match:** to see labels/handles/WUC data, the target must load a
  runtime built with the framework-side changes (see `frameworksidechanges.md`
  §13). Against a stock runtime the trees still build but nodes fall back to bare
  addresses / no live highlight.

---

## 13. Related docs

| Doc | Covers |
| --- | --- |
| `frameworksidechanges.md` | **The producer/runtime half — what all these events are** |
| `resources/visual-tree-cross-connection.md` | The four-tree cross-linkage & in-profiler glow |
| `resources/addingisvisualtree.md` | The WUC tree, producer + consumer |
| `resources/feature-is-template-child.md` | Developer-only logical tree filter |
| `resources/feature-resolved-summary-names.md` | Resolved names in the history pane |
| `resources/peer-handle-live-highlight-phase1.md` / `…phase2.md` | Peer-handle emission + element tap |
| `resources/ivisual-live-highlight.md` | WUC `Comment` highlight |
| `resources/pick-overlay-etl-suppression.md` | Pick-overlay suppression |
| `resources/producer-side-component-labels.md`, `resources/producer-side-sync-comp-node-events.md` | Individual producer changes |
