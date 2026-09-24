# Legacy DComp Tree Walker (backup)

This folder is a **backup / archive** of the original prototype that walked the
DirectComposition (DComp) tree and dumped it as XML via `OutputDebugString`.
It was the first iteration of the XamlProfiler's tree-reconstruction feature and
has since been **superseded** by the live ETW pipeline
(`XamlProfilerTracing` + `WucVisualTreeProfiler`) that the XamlProfiler app now
consumes out-of-process.

Nothing here is compiled into the framework anymore. The helper's hooks were
removed from the framework and the source files moved here so the approach stays
documented and re-enable-able, without paying its cost (an unconditional
per-frame tree walk + `OutputDebugString`) in the shipping/dev DLL.

## What's in this folder

| File | What it is |
| --- | --- |
| `DCompTreeHelper.h` / `.cpp` | The producer-side helper. Walks the comp-node tree and the dense WUC `IVisual` tree, serializes both to XML (`<CompTree>` + `<VisualTree>` inside `<DCompTreeSnapshot>`), and prints via `OutputDebugString`. Chk-only diagnostic code. |
| `LiveDCompTree.ps1` | Capture script. Reads `OutputDebugString` output, grabs each complete `<DCompTreeSnapshot>...</DCompTreeSnapshot>` and overwrites an `.xml` file so it always holds the latest tree. |
| `DCOMP_TREE_VIEWER.md` | The original deep-dive design doc: how the comp/visual trees relate, where the hooks live, the XML schema, and the throttling model. |

> **Prefer the ETW path.** For normal profiling use the XamlProfiler app — it
> reconstructs all four trees live, out-of-process, and only when a session is
> attached. Use this walker only for low-level debugging when you can't attach
> the profiler (e.g. a raw `OutputDebugString`/DebugView workflow).

---

## Part A — How to re-enable the helper in the framework

The helper needs to be (1) placed back into the `hw` component, (2) registered
in the project, and (3) re-hooked into `HWCompTreeNode`. All hooks were in a
single file: `dxaml/xcp/core/hw/hwcompnode.cpp`.

### 1. Copy the source back

Copy both files back into the `hw` folder:

```
tools/XamlProfiler/legacy-dcomptree-walker/DCompTreeHelper.h   ->  dxaml/xcp/core/hw/DCompTreeHelper.h
tools/XamlProfiler/legacy-dcomptree-walker/DCompTreeHelper.cpp ->  dxaml/xcp/core/hw/DCompTreeHelper.cpp
```

### 2. Register the files in `dxaml/xcp/core/hw/hw.vcxproj`

Add back the two entries (next to the other `hw` files):

```xml
<ClInclude Include="DCompTreeHelper.h"/>
```
```xml
<ClCompile Include="DCompTreeHelper.cpp"/>
```

### 3. Re-add the hooks in `dxaml/xcp/core/hw/hwcompnode.cpp`

**Include** (with the other profiler includes near the top):

```cpp
#include "DCompTreeHelper.h"
```

**Per-frame snapshot** — in `HWCompTreeNode::UpdateTreeRoot`, right after the
`IFC_RETURN(UpdateTree(...))` call, before `return S_OK;`:

```cpp
// Throttled full-tree snapshot to OutputDebugString (chk / DebugView).
DCompTreeHelper::MaybeDumpTreeSnapshot(this);
```

**Insert / remove edges** — three call sites:

* `HWCompTreeNode::InsertChild` — after the `CompNodeChildInserted` ETW block,
  before `return S_OK;`:
  ```cpp
  DCompTreeHelper::LogCompNodeInserted(this, pChild);
  ```
* `HWCompTreeNode::InsertChildAtBeginning` — same, after its
  `CompNodeChildInserted` block:
  ```cpp
  DCompTreeHelper::LogCompNodeInserted(this, pChild);
  ```
* `HWCompTreeNode::RemoveChild` — after the `CompNodeChildRemoved` ETW block,
  before `IFC_RETURN(RemoveChildInternal(pChild));`:
  ```cpp
  DCompTreeHelper::LogCompNodeRemoved(this, pChild);
  ```

> `MaybeDumpTreeSnapshot` is internally throttled (~once every 2.5s) and only
> prints while the app is actively rendering. `LogCompNodeInserted` /
> `LogCompNodeRemoved` only print when live logging is enabled — see below.

### 4. Build the framework

```powershell
.\initrun.ps1 build.cmd mux /q
```

Output: `BuildOutput\bin\amd64chk\Product\Microsoft.ui.xaml.dll`. Deploy that
chk DLL into the target app's install location (see the profiler deployment
notes) so it loads the freshly-built helper.

---

## Part B — How to walk the trees (capture a live view)

The helper emits to `OutputDebugString`. You can watch it two ways.

### Option 1 — Self-refreshing XML file (recommended)

`LiveDCompTree.ps1` captures each snapshot and overwrites a single `.xml` file,
so opening it in an auto-reloading editor (VS Code) gives a live view.

```powershell
powershell -ExecutionPolicy Bypass -File .\LiveDCompTree.ps1
# optional: -OutFile C:\temp\live_dcomp_tree.xml  -Seconds 600
```

Then:
1. Launch the app so it loads the **freshly-built chk** `Microsoft.UI.Xaml.dll`.
2. **Close DebugView and detach the VS native debugger** — only one process can
   read `OutputDebugString` at a time.
3. Interact with the app (scroll / expand / click) to keep it rendering; the
   `.xml` refreshes at most ~every 2.5s.
4. Open the output `.xml` (default `live_dcomp_tree.xml` next to the script) in
   VS Code and watch it reload.

Each snapshot contains two sibling trees:
* `<CompTree>` — the sparse comp-node tree (`DumpTreeRecursiveXml`).
* `<VisualTree>` — the dense WUC `IVisual` tree (`DumpVisualTreeXml`) with
  per-visual type + render attributes.

### Option 2 — DebugView (raw)

Run [DebugView](https://learn.microsoft.com/sysinternals/downloads/debugview)
as admin with "Capture Win32" enabled, launch the app, and read the
`<DCompTreeSnapshot>` blocks directly. (Don't run the script at the same time.)

### On-demand dumps from the debugger

With a native debugger attached, call from the Immediate/Watch window:

```
DCompTreeHelper::DumpMainTree()                 // whole main tree
DCompTreeHelper::DumpTreeForElement(pUIElement) // subtree for one element
DCompTreeHelper::EnableLiveLogging(true)        // stream insert/remove edges
```

See `DCOMP_TREE_VIEWER.md` for the full design, XML schema, and attribute list.
