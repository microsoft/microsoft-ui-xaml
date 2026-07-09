# DComp Tree Viewer — How It Works & How We Built It

This document explains the **DComp (DirectComposition) tree viewer** we added to
WinUI: how the composition tree exists in the engine, how it is walked/built, the
exact functions we hook into to notify our helper, the helper itself, and how to
use the live viewer with this sample app.

---

## 1. Background: Two Trees

A WinUI app actually has **two** trees stacked on top of each other.

```
  XAML Visual Tree  (what you write)        DComp Tree  (what actually renders)
  ──────────────────────────────────        ────────────────────────────────────
  CUIElement                                 HWCompNode
    Grid                                       HWCompTreeNode (root)
      Button   ← animated                        +-- HWCompTreeNode (Button)
        TextBlock                                      ...
      StackPanel
        Image

  Read by: VisualTreeHelper (public API)     Read by: DCompTreeHelper (our new helper)
```

- **Visual tree** = every `CUIElement` you declared in XAML. Walked by `VisualTreeHelper`.
- **DComp tree** = a **sparse** tree of `HWCompNode` objects that the renderer uses
  to push pixels to the screen via Windows.UI.Composition (WUC) visuals.

> **Key idea:** The DComp tree is *not* one-node-per-XAML-element. Only elements
> that need **independent composition** get a comp node. Everyone else just paints
> into an ancestor's visual.

---

## 2. How the DComp Tree Exists (data structures)

The comp nodes live in the **core** layer (`dxaml/xcp/core/hw/`).

### Class hierarchy (`hwcompnode.h`)
```
CDependencyObject                     ← base; provides m_pParent + GetParentInternal()
  └── HWCompNode                      ← base comp node; GetWUCVisual()
        ├── HWCompLeafNode            ← leaves (render data, media, swapchain)
        └── HWCompTreeNode            ← has children (m_children) + a UIElement back-ptr
              └── HWCompTreeNodeWinRT ← the concrete WUC implementation
```

### Where the parent/child links are stored
| Link | Field / method | Defined in |
| ---- | -------------- | ---------- |
| Parent | `m_pParent` (via `GetParentInternal()`) | `CDependencyObject` (inherited!) |
| Children | `m_children` (a `CompNodeCollection`) | `HWCompTreeNode` |
| Child iteration | `GetChildrenBegin()` / `GetChildrenEnd()` | `HWCompTreeNode` |
| Owning XAML element | `m_pUIElementNoRef` / `GetUIElementPeer()` | `HWCompTreeNode` |
| The actual rendered visual | `GetWUCVisual()` | `HWCompNode` |

Because `HWCompNode` inherits from `CDependencyObject`, **parent tracking already
exists for free** — exactly like the visual tree. We did **not** build any of this;
it predates our work.

---

## 3. How the Tree Gets Built (the render walk)

The tree is (re)built **every frame** by the render walk, `HWWalk` (`hwwalk.cpp`).

### Step A — decide which elements need a comp node
For each `CUIElement`, the walk asks (`uielement.cpp`):
```cpp
bool CUIElement::RequiresCompositionNode() const
{
    return HasIndependentAnimationsOrManipulations()  // animated on render thread
        || IsManipulatable()                          // touch pan/zoom
        || IsRenderWalkRoot()                         // the root
        || HasComplexTransformation()                 // rotation / projection
        || HasSwapChainContent()                      // SwapChainPanel
        || IsUsingHandOffVisual()                     // app grabbed the Visual
        || DCompTreeHost::IsFullCompNodeTree()        // debug: force ALL
        || ... many more conditions ...;
}
```
If **any** condition is true → this element gets a comp node. Otherwise it is
skipped and just paints into the nearest ancestor's visual (that is why the tree
is sparse).

### Step B — create & link the comp node
In `hwwalk.cpp`:
```cpp
if (requiresCompositionNode)                       // = RequiresCompositionNode()
{
    EnsureCompositionPeer(pUIElement, ...);        // create HWCompTreeNode (cached)
    myRP.pHWWalk->SetParentCompNode(pElementCompNode); // become parent for descendants
}
```
The walk tracks a "current parent comp node" as it descends. A new comp node is
linked to **the nearest ancestor that also has a comp node** — skipping all the
elements that don't. The physical linking happens in
`HWCompTreeNode::InsertChild` (see next section).

---

## 4. The Functions We Hook Into (to notify our helper)

We made **observation-only** additions at the exact points where the tree changes
or finishes updating. These are all in `hwcompnode.cpp`.

| Hooked function | Line (approx) | What it does | Our call |
| --------------- | ------------- | ------------ | -------- |
| `HWCompTreeNode::InsertChild` | ~653 | Links a child comp node into a parent | `DCompTreeHelper::LogCompNodeInserted(this, pChild)` |
| `HWCompTreeNode::InsertChildAtBeginning` | ~699 | Links a child as the first child | `DCompTreeHelper::LogCompNodeInserted(this, pChild)` |
| `HWCompTreeNode::RemoveChild` | ~735 | Unlinks a child comp node | `DCompTreeHelper::LogCompNodeRemoved(this, pChild)` |
| `HWCompTreeNode::UpdateTreeRoot` | ~528 | Runs once per frame on the **root** after the tree is updated | `DCompTreeHelper::MaybeDumpTreeSnapshot(this)` |

> These are the **only** behavioral changes to existing engine code — 4 call sites.
> Everything else is pure reading. The tree is still built entirely by `HWWalk`
> exactly as before; we just observe it.

`InsertChild` itself (where the link is physically made):
```cpp
HWCompTreeNode::InsertChild(parent=this, pChild, ...)
{
    m_children.insert(index, pChild);   // parent now knows the child
    AddRefInterface(pChild);
    pChild->AddParent(this, ...);       // child now knows the parent (m_pParent)
    InsertChildInternal(...);           // insert the real WUC visual
    DCompTreeHelper::LogCompNodeInserted(this, pChild);   // ← our hook
}
```

---

## 5. The Helper: `DCompTreeHelper`

New files: `dxaml/xcp/core/hw/DCompTreeHelper.h` and `DCompTreeHelper.cpp`.

> It lives in **core/hw** (not the DXAML layer like `VisualTreeHelper`) because the
> `HWCompNode` types are **core-internal** and not visible outside core. A helper
> must live in the layer where the types it touches are defined.

### Public functions
| Function | Purpose |
| -------- | ------- |
| `GetParent(node)` | Parent comp node (via `GetParentInternal`, cast to `HWCompTreeNode`). |
| `GetChild(node, i)` | i-th child (iterates `m_children`). |
| `GetChildrenCount(node)` | Number of children. |
| `GetUIElement(node)` | The `CUIElement` that owns this comp node. |
| `GetCompNode(uielement)` | The comp node for a given `CUIElement` (`GetCompositionPeer`). |
| `DumpMainTree(pCore)` | Print the whole tree from the main visual tree's root. |
| `DumpMainTree()` | Zero-arg overload — grabs the core itself via `DXamlServices`. |
| `DumpTree(root, maxDepth)` | Print the tree from a given comp node. |
| `DumpTreeForElement(uielement)` | Print the subtree under a UIElement's comp node. |
| `MaybeDumpTreeSnapshot(root)` | **Throttled** full-tree dump; the auto-snapshot. |
| `EnableAutoSnapshot(bool)` / `IsAutoSnapshotEnabled()` | Toggle the auto-snapshot (on by default in DBG). |
| `EnableLiveLogging(bool)` / `IsLiveLoggingEnabled()` | Toggle the per-change stream (off by default). |
| `LogCompNodeInserted/Removed(parent, child)` | Called by the hooks to print one change line. |

### Private helpers
| Function | Purpose |
| -------- | ------- |
| `DumpTreeRecursive(node, depth, maxDepth, indent)` | Depth-first walk that prints each node with indentation (text format). |
| `DumpTreeRecursiveXml(node, depth, maxDepth, indent)` | Depth-first walk that emits nested `<CompNode>` tags for the **sparse comp-node tree** (`<CompTree>`); QIs each node's WUC visual to discover its concrete type and reads its render + type-specific properties. Recurses into **child comp nodes only** (no WUC nesting — the WUC visuals now live in the sibling `<VisualTree>`). |
| `DumpVisualTreeXml(visual, indentLevel, depth, maxDepth)` (file-scope) | Depth-first walk of the **pure WUC visual tree** (`<VisualTree>`), rooted at the root comp node's container visual. Emits one `<WucVisual type="…" wucVisual="0x…" …>` per visual (full property set via `AppendVisualPropertyAttributes`), then recurses through each visual's `Children` (`get_Children` → `IIterable` → `IIterator`), capped at depth 256. |
| `GetNodeDescription(node, buf, size)` | Builds a label like `TreeNode [Button] (children:2) [WUC]`. |
| `CompositeModeName` / `BorderModeName` / `BackfaceVisibilityName` (file-scope) | Map the WUC enum values to readable strings for the XML. |
| `GetWucClassName` (file-scope) | Asks a WUC object for its runtime class name (`MetadataAPI::GetRuntimeClassName`) and strips the namespace prefix. |
| `GetWucVisualTypeName` (file-scope) | Returns a visual's concrete WUC type via a QI ladder (most-derived first), used to label `<WucVisual>` elements. |
| `GetWucVisualChildCount` (file-scope) | Returns a visual's direct WUC child count (or `-1` if it isn't a container); used to decide whether a `<WucVisual>` self-closes or recurses. |
| `AppendVisualPropertyAttributes` (file-scope) | Appends the full render + type-specific attribute set for a visual; shared by both `<CompNode>` and `<WucVisual>` so the two tags describe a visual identically. |

### How printing works
1. `DumpTreeRecursive` visits a node, prints it, then recurses into each child.
2. The **indent level** (2 spaces per depth, plus `+--`) is what makes the text
   look like a tree:
   ```
   TreeNode [Grid] (children:2) [WUC]
     +-- TreeNode [Button] (children:1) [WUC]
       +-- TreeNode [TextBlock] (children:0) [WUC]
   ```
3. Each line is sent to the Windows debug stream via `OutputDebugStringW`.

### The throttle (auto-snapshot)
```cpp
void DCompTreeHelper::MaybeDumpTreeSnapshot(root)
{
    if (!g_dcompAutoSnapshotEnabled || root == nullptr) return;
    static ULONGLONG s_lastDumpTime = 0;
    ULONGLONG now = GetTickCount64();
    if (s_lastDumpTime != 0 && now - s_lastDumpTime < 2500 /*ms*/) return; // skip
    s_lastDumpTime = now;

    // Emit ONE XML snapshot containing TWO sibling trees:
    OutputDebugStringW(L"<DCompTreeSnapshot>\n");
      OutputDebugStringW(L"<CompTree>\n");
      DumpTreeRecursiveXml(root, 0, 50, 2);          // sparse HWCompNode tree
      OutputDebugStringW(L"</CompTree>\n");
      OutputDebugStringW(L"<VisualTree>\n");
      DumpVisualTreeXml(root->GetWUCVisual(), 2, 0, 256); // pure WUC IVisual tree
      OutputDebugStringW(L"</VisualTree>\n");
    OutputDebugStringW(L"</DCompTreeSnapshot>\n");
}
```
Because `MaybeDumpTreeSnapshot` is called from `UpdateTreeRoot` (once per frame),
a snapshot prints **only when both are true**:
1. a render happened this frame, **and**
2. ≥ 2.5 s elapsed since the last printed snapshot.

So an **idle** app prints nothing; interact with it to trigger renders. The
`<DCompTreeSnapshot>` / `</DCompTreeSnapshot>` markers are what the capture script
uses to detect a complete tree and write it to the `.xml` file.

### The XML snapshot attributes (`DumpTreeRecursiveXml`)

The auto-snapshot is emitted as XML by `DumpTreeRecursiveXml`. Each `<CompNode>`
carries far more than the comp-node identity — it also reflects the **concrete
WUC visual** behind the node and that visual's live render properties.

**How the concrete type is found.** `GetWUCVisual()` hands back a base
`WUComp::IVisual`, but the real object is one of the derived visuals. We probe it
with `QueryInterface` **most-derived first** (the four leaf types all derive from
`ContainerVisual`, so they must be checked before it):

```
ISpriteVisual → "SpriteVisual"
ILayerVisual → "LayerVisual"
IShapeVisual → "ShapeVisual"
IRedirectVisual → "RedirectVisual"
IContainerVisual → "ContainerVisual"
else → "Visual"
```

The first interface the object answers `S_OK` to wins and is written as
`visualType="…"`.

**Attributes emitted per node:**

| Group | Attributes |
| ----- | ---------- |
| Identity / tree | `type`, `element`, `name`, `children`, `hasVisual`, `visualType`, `compNode`, `uiElement`, `wucVisual` |
| Base transform (`IVisual`) | `offsetX/Y/Z`, `width`/`height`, `anchorPointX/Y`, `centerPointX/Y/Z`, `scaleX/Y/Z`, `rotationAngleInDegrees`, `rotationAxisX/Y/Z`, `orientationX/Y/Z/W`, `transformMatrix` (16 values) |
| Render / modes | `opacity`, `isVisible`, `compositeMode`, `borderMode`, `backfaceVisibility`, `hasClip`, `clipType` |
| Versioned base | `relativeOffsetAdjustmentX/Y/Z`, `relativeSizeAdjustmentX/Y` (`IVisual2`), `isHitTestVisible` (`IVisual3`), `isPixelSnappingEnabled` (`IVisual4`) |
| Type-specific | `wucChildCount` (container-derived), `shapeCount`+`hasViewBox` (ShapeVisual), `hasBrush`+`brushType`+`hasShadow`+`shadowType` (SpriteVisual), `hasEffect`+`hasShadow` (LayerVisual), `sourceVisual` (RedirectVisual) |

**Two formatting conventions:**

- **Enums → readable names.** `compositeMode`, `borderMode`, and
  `backfaceVisibility` are mapped from their integer values to strings (e.g.
  `borderMode="Soft"`) by small `switch` helpers (`CompositeModeName`,
  `BorderModeName`, `BackfaceVisibilityName`).
- **Object properties → class name.** For object-valued properties (Brush,
  Shadow, Clip) the helper `GetWucClassName` calls
  `MetadataAPI::GetRuntimeClassName` on the object and strips the
  `Microsoft.UI.Composition.` namespace, so you get e.g.
  `brushType="CompositionColorBrush"` alongside a `has*="true/false"` flag.

The whole opening tag is built into one `WCHAR line[4096]` buffer (enlarged from
1536 to fit the expanded attribute set) via successive `swprintf_s` appends, then
emitted with a single `OutputDebugStringW`.

### The two sibling trees inside a snapshot

Each `<DCompTreeSnapshot>` now contains **two independent trees**, so the same
render can be inspected at two granularities:

```xml
<DCompTreeSnapshot>
  <CompTree>
    <CompNode element="Grid" visualType="ContainerVisual" wucVisual="0x…" …>
      <CompNode element="Button" …> … </CompNode>
    </CompNode>
  </CompTree>
  <VisualTree>
    <WucVisual type="ContainerVisual" wucVisual="0x…" …>
      <WucVisual type="SpriteVisual" wucVisual="0x…" … />
      <WucVisual type="ContainerVisual" wucVisual="0x…" …>
        <WucVisual type="ShapeVisual" wucVisual="0x…" … />
      </WucVisual>
    </WucVisual>
  </VisualTree>
</DCompTreeSnapshot>
```

- **`<CompTree>`** (`DumpTreeRecursiveXml`) — the **sparse `HWCompNode` tree**.
  Each `<CompNode>` recurses into its **child comp nodes only** (`willRecurse =
  isTreeNode && childCount > 0`). It carries the identity/tree attributes plus the
  full render attributes of the node's *own* spine visual, but it no longer nests
  the visual's WUC children — those moved to `<VisualTree>`.
- **`<VisualTree>`** (`DumpVisualTreeXml`) — the **dense, pure WUC `IVisual`
  tree**, rooted at the root comp node's container visual
  (`root->GetWUCVisual()`) and walked recursively through each visual's
  `Children`. This is where the actual `SpriteVisual`/`ShapeVisual`/`LayerVisual`
  content and every child comp node's connection visual appear.

The `wucVisual="0x…"` address is the bridge between the two: a `<WucVisual>` in
`<VisualTree>` whose address matches a `<CompNode wucVisual="0x…">` in `<CompTree>`
is that comp node's own spine visual.

**How `<VisualTree>` is walked** (`DumpVisualTreeXml`, depth-capped at 256):

- Emit `<WucVisual type="…" wucVisual="0x…" …>` for the current visual — `type`
  via `GetWucVisualTypeName`'s QI ladder, the full property set via the shared
  `AppendVisualPropertyAttributes` (same attributes a `<CompNode>` emits).
- If it has children (`GetWucVisualChildCount > 0`), QI to `IContainerVisual` →
  `get_Children()` → `IVisualCollection` → `IIterable<Visual*>` →
  `IIterator<Visual*>`, then iterate `get_HasCurrent` / `get_Current` / `MoveNext`
  and recurse into each child. Otherwise the tag self-closes (`… />`).

---

## 6. Where the Output Goes

Everything is written with **`OutputDebugStringW`** — Windows' debug output
stream — as **XML** wrapped in `<DCompTreeSnapshot>` … `</DCompTreeSnapshot>`
markers. There is **no on-screen window**; the text is invisible unless a tool
captures that stream:
- **Visual Studio** Output window (when debugging), or
- **DebugView** (Sysinternals), or
- our **`LiveDCompTree.ps1`** capture script (below), which turns it into
  `live_dcomp_tree.xml`.

Only **one** tool can capture the stream at a time.

---

## 7. The Live Viewer Script

File: `Samples/FolderTreeApp/LiveDCompTree.ps1`
Output: `Samples/FolderTreeApp/live_dcomp_tree.xml` (default; override with `-OutFile`)
Duration: `-Seconds` (default `600` = 10 min, then auto-stops)

The script is a thin **capturer** — all the tree-walking happens inside the
checked `Microsoft.UI.Xaml.dll` (sections 3–5). It compiles a small inline C#
helper (`LiveDCompXml`) via `Add-Type` and:

1. **Attaches to the debug stream.** It creates the `DBWIN_BUFFER_READY` /
   `DBWIN_DATA_READY` events and the `DBWIN_BUFFER` shared-memory section — the
   standard `OutputDebugString` (`DBWIN_*`) protocol — so it receives every line
   the DLL emits. (Only one process can own that stream at a time, so DebugView /
   the VS native debugger must be closed.)
2. **Reassembles a snapshot.** It buffers lines between the `<DCompTreeSnapshot>`
   and `</DCompTreeSnapshot>` markers. Everything in between — the `<CompTree>` and
   `<VisualTree>` bodies — is captured verbatim (the DLL already emits well-formed
   XML, so the script does no formatting).
3. **Writes the `.xml` atomically.** On each complete snapshot it wraps the body in
   a root `<DCompTreeSnapshot capturedAt="…" snapshot="N">`, writes to a `.tmp`
   file, then `File.Replace`s the target — so a live reader never sees a
   half-written document — and also echoes the tree to the console.

Open `live_dcomp_tree.xml` in an editor that auto-reloads on change (e.g. VS Code)
to watch the comp tree + IVisual tree refresh live. (An earlier standalone
`DCompTreeViewer.html` visualiser was removed; the `.xml` is now the single
output — view it directly, or feed it to any XML tool.)

### How the comp tree is walked (producer → script)

The script never walks the tree itself — it just relays what the DLL produced.
The actual walk is:

1. `HWWalk` (re)builds the sparse comp tree every frame and calls
   `HWCompTreeNode::UpdateTreeRoot` on the root when done (section 3).
2. That hook calls `DCompTreeHelper::MaybeDumpTreeSnapshot(root)` (section 4),
   which — at most once per 2.5 s and only if a render happened — emits one
   `<DCompTreeSnapshot>` containing:
   - `<CompTree>` via `DumpTreeRecursiveXml` — a DFS of `m_children` from the root
     comp node, one `<CompNode>` per node, recursing into child comp nodes.
   - `<VisualTree>` via `DumpVisualTreeXml` — a DFS from `root->GetWUCVisual()`
     through each visual's `Children`, one `<WucVisual>` per IVisual.
3. The script captures those lines off the debug stream and writes the `.xml`.

So "interact with the app → the DLL re-walks and re-emits → the script rewrites the
file". The two trees are correlated by the shared `wucVisual="0x…"` address.

### Usage
1. **Close DebugView** / detach the VS native debugger (only one capturer allowed).
2. Start the script first:
   ```powershell
   cd Samples\FolderTreeApp
   powershell -ExecutionPolicy Bypass -File .\LiveDCompTree.ps1
   ```
3. Launch the app:
   ```powershell
   Start-Process "shell:AppsFolder\FolderTreeApp_6f07fta6qpts2!App"
   ```
4. **Interact** with the app (scroll, expand folders) to trigger renders. The tree
   refreshes (≤ once per 2.5 s).

> The script needs **no rebuild** — a checked build of `Microsoft.UI.Xaml.dll`
> already emits the XML snapshots; the script only captures them.

---

## 8. Deployment Note (why you might see an old tree)

This sample runs as a **registered MSIX package**. The DLL it actually loads is:
```
BuildOutput\obj\amd64chk\Samples\FolderTreeApp\FolderTreeApp\AppX\Microsoft.UI.Xaml.dll
```
After rebuilding mux, copy the fresh DLL there (the app won't pick it up otherwise):
```powershell
Copy-Item `
  BuildOutput\bin\amd64chk\Product\Microsoft.ui.xaml.dll `
  BuildOutput\obj\amd64chk\Samples\FolderTreeApp\FolderTreeApp\AppX\Microsoft.UI.Xaml.dll -Force
```

---

## 9. Summary of Files Changed / Added

| File | Change |
| ---- | ------ |
| `dxaml/xcp/core/hw/DCompTreeHelper.h` | **New** — helper class declaration. |
| `dxaml/xcp/core/hw/DCompTreeHelper.cpp` | **New** — navigation, dump, throttled snapshot, live-log impl. Emits each snapshot as XML with **two sibling trees**: `<CompTree>` (`DumpTreeRecursiveXml`, sparse comp-node tree) and `<VisualTree>` (`DumpVisualTreeXml`, dense WUC IVisual tree). QIs each WUC visual for its concrete type (`visualType`/`type`) and an expanded set of render + type-specific attributes (enums as readable names, object props as runtime class names via `GetWucClassName`), shared through `AppendVisualPropertyAttributes`. Adds `#include <MetadataAPI.h>`. |
| `dxaml/xcp/core/hw/hwcompnode.cpp` | **Modified** — 4 hook calls (`InsertChild`, `InsertChildAtBeginning`, `RemoveChild`, `UpdateTreeRoot`) + include. |
| `dxaml/xcp/core/hw/hw.vcxproj` | **Modified** — registered `DCompTreeHelper.cpp` (`ClCompile`) and `.h` (`ClInclude`). |
| `Samples/FolderTreeApp/LiveDCompTree.ps1` | **New** — live capture script (DBWIN → `.xml`). |
| `Samples/FolderTreeApp/live_dcomp_tree.xml` | **Generated** — latest snapshot, overwritten by the script each capture (a transient artifact, not source). |

---

## 10. One-paragraph Recap

The DComp tree already existed and was built every frame by `HWWalk`, which gives a
comp node only to elements where `RequiresCompositionNode()` is true and links each
one (via `HWCompTreeNode::InsertChild`) to its nearest ancestor comp node, storing
parent/child in `m_pParent` / `m_children`. We added `DCompTreeHelper` (in
core/hw, because `HWCompNode` is core-internal) to **read** that tree, and hooked
four existing functions — `InsertChild`, `InsertChildAtBeginning`, `RemoveChild`
(for per-change logging) and `UpdateTreeRoot` (for a throttled full snapshot) — to
emit an **XML `<DCompTreeSnapshot>`** via `OutputDebugString`, carrying two sibling
trees: `<CompTree>` (the sparse comp-node tree) and `<VisualTree>` (the dense WUC
IVisual tree), correlated by each visual's `wucVisual` address. The
`LiveDCompTree.ps1` script captures that debug-stream output and rewrites
`live_dcomp_tree.xml` on every snapshot, so opening it in an auto-reloading editor
gives a live view of both trees.
