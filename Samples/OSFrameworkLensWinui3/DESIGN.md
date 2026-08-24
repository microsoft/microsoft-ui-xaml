# OS Framework Lens — Design Brief

A Windows developer tool that identifies which UI framework owns the component under the mouse cursor, plus a per-window breakdown of every framework rendering inside any window. Built to help engineers and leadership understand the modernization surface of the Windows shell and shipping apps.


## What it does

Two real-time modes:

1. **Live inspector** — hover any control, see its framework, confidence, evidence, and an embedded-host badge if it's a XAML island / WebView / NetUI inside a Win32 frame.
2. **Per-window analyzer** — for any window, breaks down which frameworks render inside it, with HWND count and percentage of window area per framework.

Recognises 16 frameworks: Win32, WPF, WinForms, MFC, ComCtl32, DirectUI, Office NetUI, WinUI 2/UWP, WinUI 3, WebView2, React Native for Windows, .NET MAUI, Java Swing/AWT, Qt, Avalonia, Flutter.


## Design decisions

Authoritative log. **Read before proposing or refuting a design choice.** Any change must come with HLD/LLD justification — perf data, broken invariant, security flaw, new platform requirement. Aesthetic preference is not enough. Append new `D#`; strike superseded rows with `→ superseded by D#`. Never silently delete.

### D1. Port to WinUI 3

Why: tracks the modernization story; uses the stack we inspect.

Trade-off: ~70 MB self-contained; `SetWindowSubclass` for WndProc hook; `DataPackage` for clipboard; no owner/child window concept.

### D2. Unpackaged, self-contained

Why: xcopy-deployable; no MSIX install to demo on a dev machine.

Trade-off: ~70 MB output, no AppContainer sandbox, no auto-update.

### D3. x64 only

Why: most modern targets are 64-bit; 32-bit targets are surfaced as `"32-bit, can't enumerate"`.

Trade-off: can't introspect itself in 32-bit form. Explicitly de-scoped.

### D4. Real UIA via direct COM (`CUIAutomation`), no FlaUI

Why: wrappers add a dependency for zero benefit; UIA's COM surface is small and stable.

Trade-off: slightly more interop; we own the COM lifetime (released in `SnapshotService.Dispose`).

### D5. 30 Hz tick on `DispatcherQueueTimer`, snapshot on `Task.Run`, single-flight gate

Why: UI stays responsive even when UIA / module-enum takes 15–30 ms. The `_pendingSnapshot.IsCompleted` gate prevents thread-pool pile-up if a target hangs.

Trade-off: late results from a now-changed window still get applied — real past state; next tick overwrites within 33 ms.

### D6. `CommunityToolkit.Mvvm` `[ObservableProperty]` over hand-rolled INPC

Why: removes ~80 lines of `Set<T>` boilerplate; idiomatic for WinUI 3; partial-method change hooks keep coupled state in one place.

Trade-off: one small NuGet dependency.

### D7. Field-based `[ObservableProperty]`, not partial-property syntax

Why: partial properties are the AOT-safe recommendation; we don't `PublishAot`. The 8.4.x generator on our Roslyn version also produced `CS9248` for partial properties.

Trade-off: one `MVVMTK0045` suppression in csproj. Revisit if we add `<PublishAot>true</PublishAot>`.

### D8. Flat VM property surface (no nested models bound by `x:Bind`)

Why: nested-path `x:Bind` needs a `FrameworkElement` root for converter scope and adds complexity for zero gain — every binding is a primitive.

Trade-off: a 24-property VM.

### D9. `async void OnClosedHandler` awaits `_pendingSnapshot` before disposing

Why: otherwise an in-flight snapshot touches a disposed `IUIAutomation`. `async void` is correct only in event handlers; this is one.

Trade-off: close may take up to one snapshot duration to drain.

### D10. No XAML value-converters; VM pre-builds `Brush` and `Visibility`

Why: converters force a `FrameworkElement` root and add a code-path per binding.

Trade-off: VM knows about presentation types — acceptable, it's WinUI-specific anyway.

### D11. No classifier unit tests in this PR (deferred)

Why: keep the PR focused on the two reviewer-requested behaviour changes (off-thread snapshot + MVVM migration).

Plan: `tests/OSFrameworkLens.Tests/` (xUnit, `net9.0`, `[InternalsVisibleTo]`).

### D12. Default `internal`; promote to `public` only when XAML forces it

Why: zero external consumers, so every `public` is a false API contract.

Public exceptions: XAML hard-requires `public` for `x:Class` types (`InspectorWindow`, `WindowAnalysisWindow`, `App`) and for `x:DataType` targets (`FrameworkSliceVm`). Everything else stays `internal`.


## Flow diagrams

Every architectural choice falls out of one constraint: **deliver a fresh classification and a coloured outline at 30 fps while inspecting an arbitrary stranger process that may be hung, protected, or windowless.**

### 1. Startup flow — what runs when you launch the EXE

```
        +-----------+
        | EXE start |
        +-----+-----+
              |
              v
   +--------------------------+
   |     App.xaml.cs          |
   |--------------------------|
   |  ctor()                  |
   |  OnLaunched()  --------------+
   +--------------------------+   |
                                  v
   +--------------------------------------------+
   |       InspectorWindow.xaml.cs ctor         |
   |--------------------------------------------|
   |  InitializeComponent       (loads .xaml)   |
   |  GetWindowHandle           -> our HWND     |
   |  ConfigureAppWindow        top-right       |
   |  new HighlightOverlay      layered HWND    |
   |  DispatcherQueueTimer      33 ms tick      |
   |  SetWindowSubclass         our WndProc     |
   |  RegisterHotKey            Ctrl+Shift+F    |
   +-----------------+--------------------------+
                     |
                     v
              +-------------+
              |  Activate() |  ====> 30 Hz loop begins
              +-------------+
```

Every Win32 dependency is installed before `Activate()` returns, so the very first tick already has the overlay, hotkey, and subclass in place.


### 2. Live inspector — 30 Hz hot path

```
   [UI thread]                           [worker thread]
   ============                          ===============

   +------------------------+
   | DispatcherQueueTimer   |  every 33 ms
   |       .Tick            |
   +-----------+------------+
               |
               v
   +------------------------+
   |  InspectorWindow       |
   |     .OnTick()          |
   |  GetCursorPos          |
   +-----------+------------+
               |
               v
       +---------------+    no    +------------------+
       | _pendingSnap  |--------->| skip tick (D5)   |
       |   done?       |          +------------------+
       +-------+-------+
               | yes
               v
       +---------------+
       |   Task.Run    |================================+
       +---------------+                                |
                                                        v
                                       +----------------------------+
                                       |   SnapshotService          |
                                       |     .Snapshot(x, y)        |
                                       |       orchestrator         |
                                       +-+--------+--------+--------+
                                         |        |        |
                              +----------+        |        +-----------+
                              v                   v                    v
                  +-------------------+ +-------------------+ +-------------------+
                  |  HwndInspector    | |  UiaInspector     | |  ModuleInspector  |
                  |-------------------| |-------------------| |-------------------|
                  | WindowFromPoint   | | ElementFromPoint  | | OpenProcess       |
                  | GetClassNameW     | |  (COM)            | | EnumProcessModules|
                  | GetWindowTitleSafe| | FrameworkId,      | | per-PID 2s cache  |
                  | GetWindowRect,PID | | ClassName, Name   | | -> [dll, dll,...] |
                  +---------+---------+ +---------+---------+ +---------+---------+
                            |                     |                     |
                            +---------------------+---------------------+
                                                  |
                                                  v
                                  +-------------------------------+
                                  |   FrameworkClassifier         |
                                  |     .Classify                 |
                                  |  +5 UIA  +4 HWND  +1/module   |
                                  |  -> Best, Score, Confidence,  |
                                  |     Evidence                  |
                                  +---------------+---------------+
                                                  |
                                                  v
                                  +-------------------------------+
                                  |   IslandDetector.Detect       |
                                  |   (embedded host? e.g.        |
                                  |    WinUI island in Win32)     |
                                  +---------------+---------------+
                                                  |
                                                  v
                                  +-------------------------------+
                                  |  ElementSnapshot (immutable)  |
                                  +---------------+---------------+
                                                  |
                                                  v
   +-----------------------+    +-----------------------------+
   |  back on UI thread    |<===|  DispatcherQueue.TryEnqueue |
   +-----------+-----------+    +-----------------------------+
               |
       +-------+-------+
       |               |
       v               v
   +---------+   +---------------------+
   | VM      |   |  HighlightOverlay   |
   | .Apply  |   |   .ShowRect         |
   | x:Bind  |   |  SetWindowPos       |
   | redraws |   |  GDI rectangle      |
   +---------+   +---------------------+
               |
               v
        (next tick)
```

**Why this shape**

- Timer on **UI thread** — result handlers (`Apply`, `ShowRect`) need no marshalling.
- Snapshot on **worker thread** — a 50 ms `WM_GETTEXT` to a hung target can't freeze us.
- Single-flight `_pendingSnapshot` **gate** — slow targets can never queue more than one snapshot (D5).
- Three inspectors run **independently** — a UIA timeout doesn't block the HWND read.
- **Immutable** `ElementSnapshot` crosses the thread boundary once — no locks, no races.


### 3. Analyze button — per-window framework breakdown

```
   [UI thread]                  [worker thread]
   ============                 ===============

   +--------------------+
   | Click "Analyze"    |
   +---------+----------+
             |
             v
   +-------------------------+
   | InspectorWindow         |
   |   .OnAnalyzeClicked     |
   |   pick root HWND        |
   +-----------+-------------+
               |
               v
       +---------------+
       |   Task.Run    |=======================+
       +---------------+                       |
                                               v
                              +----------------------------------+
                              |  SnapshotService                 |
                              |    .AnalyzeWindow(rootHwnd)      |
                              +----------------+-----------------+
                                               |
                                               v
                              +----------------------------------+
                              |  WindowFrameworkAnalyzer         |
                              |    EnumChildWindows (<= 4000)    |
                              |    for each visible child:       |
                              |      ClassifyByClassOnly         |
                              |      (no UIA -- 4000 COM = sec)  |
                              +----------------+-----------------+
                                               |
                                               v
                              +----------------------------------+
                              |  aggregate per framework:        |
                              |    count, pixel area, samples    |
                              |  + "Loaded but not seen" tail    |
                              +----------------+-----------------+
                                               |
                                               v
                              +----------------------------------+
                              |  WindowAnalysis (DTO)            |
                              +----------------+-----------------+
                                               |
   +-----------------------+    +--------------+---------------+
   |  back on UI thread    |<===|  DispatcherQueue.TryEnqueue  |
   +-----------+-----------+    +------------------------------+
               |
               v
   +-----------------------------+
   | new WindowAnalysisWindow    |  independent top-level
   |     .Activate()             |  (no owner concept in WinUI)
   +-----------+-----------------+
               |
               v
   +-----------------------------+
   | WindowAnalysisWindow.Render |
   |  FrameworkSliceVm per slice |
   |  BarWidth scaled vs largest |
   +-----------------------------+
```

**Why this shape:** tree-walks cost ~100 ms on shell surfaces — never run on the 30 Hz tick. UIA deliberately skipped (4000 cross-process COM calls = seconds). "Loaded but not seen" is honest reporting so reviewers can audit the gap between *loaded runtimes* and *rendered HWNDs*.


### 4. Shutdown — strict order or COM faults

```
   User closes the inspector window
        |
        v
   InspectorWindow.OnClosedHandler  (async void -- correct for event handler)
        |
        v   1. Stop timer FIRST           -- no new ticks fire
        v   2. await _pendingSnapshot     -- drain in-flight worker
        v   3. UnregisterHotKey + RemoveWindowSubclass
        v   4. HighlightOverlay.Close (DestroyWindow)
        v   5. SnapshotService.Dispose    -- Marshal.FinalReleaseComObject IUIAutomation
```

Skipping any of these races crashes the EXE on close: the timer fires once more → worker touches a disposed `IUIAutomation` → COM fault. Strict reverse order of startup (D9).


### Observed latency

- ~5–15 ms typical hover (warm module cache).
- ~30–50 ms first frame in a new process (cold `EnumProcessModulesEx`).
- 33 ms tick is comfortably above both → smooth 30 Hz; single-flight gate caps attempted work at 30 Hz even when target is slow.


## How the framework is determined

Three signals, weighted, scored, highest score wins. Multiple signals are deliberate so no single misleading classname or DLL can fool us.

| Signal | Source | Weight |
|---|---|---|
| UIA `FrameworkId` | `IUIAutomation::ElementFromPoint` then `CurrentFrameworkId` (prop 30024) | **+5** |
| HWND class-name regex | `GetClassNameW` (and UIA `ClassName` as fallback) | **+4** |
| Loaded module match | `EnumProcessModulesEx` | **+1 per match** |

```
total_score(fw) = UIA_match*5 + class_regex_match*4 + count(matched_modules)
```

Confidence bands: **High** ≥ 5, **Medium** ≥ 4, **Low** ≥ 1. Below Low → **Unknown** — we never guess.

**Disambiguation built into `frameworks.json`**

- WinUI 2 vs WinUI 3 — both report UIA `XAML`; host class (`CoreWindow` vs `DesktopChildSiteBridge`) and modules (`Windows.UI.Xaml.dll` vs `Microsoft.UI.Xaml.dll`) disambiguate.
- Office NetUI vs plain Win32 — both report UIA `Win32`; `^NetUI` class regex + `mso*.dll` modules win.
- WebView2 vs Chrome — both have `Chrome_WidgetWin_*`; parent process + `EmbeddedBrowserWebView.dll` disambiguate.
- React Native for Windows vs WinUI 3 — RNW *is* WinUI 3 underneath; `Microsoft.ReactNative.dll` + `hermes.dll` disambiguate.

**Embedded-host detection.** When the leaf component is one framework but the root window's class belongs to a different framework family, we flag it as embedded (e.g. WinUI 3 island in a Win32 host). If the root class isn't fingerprinted, we make a best guess from the root process's loaded modules — labelled "Best guess:" with matched DLLs shown as evidence, never as a verdict.

**Honesty principles**

- Per-HWND signals decide; per-process signals corroborate. A loaded DLL alone is never enough.
- "Loaded but not seen" lists frameworks whose runtimes are in the process but produced no visible HWND — so users can audit the gap.
- Unknown is a first-class answer.


## Code map

Two layers. **`Detection/` is pure logic** (no WinUI types, headlessly testable). **`UI/` is wiring** (timers, dispatch, painting).

```
src/OSFrameworkLens/
  App.xaml(.cs)                          WinUI bootstrap; OnLaunched -> new InspectorWindow().Activate()
  Detection/
    Native.cs                            All [DllImport] declarations; called only from the 4 files below
    HwndInspector.cs                     HWND tree, class name, safe title (SendMessageTimeout/SMTO_ABORTIFHUNG)
    UiaInspector.cs                      IUIAutomation COM; FrameworkId/ControlType/Name/AutomationId/ClassName
    ModuleInspector.cs                   OpenProcess + EnumProcessModulesEx, per-PID 2s cache
    FrameworkClassifier.cs               Score-based, evidence-emitting classifier
    Fingerprints/frameworks.json         Data-driven framework definitions (edit JSON to add a framework)
    IslandDetector.cs                    Cross-references leaf vs root frameworks; emits host badge
    ClassNameDescriptions.cs             Class-name -> human label ("Shell_TrayWnd" -> "Windows taskbar")
    WindowFrameworkAnalyzer.cs           Analyze button: tree-walk + aggregate by framework
    SnapshotService.cs                   Orchestrator; only place that composes the above
    ElementSnapshot.cs                   Immutable DTO returned by Snapshot()
  UI/
    InspectorWindow.xaml(.cs)            Hosts the 33 ms DispatcherQueueTimer, hotkey, subclass, buttons
    InspectorViewModel.cs                ObservableObject; Apply(snapshot) -> x:Bind updates
    HighlightOverlay.cs                  Raw Win32 layered click-through window for the coloured outline
    WindowAnalysisWindow.xaml(.cs)       Per-window breakdown popup
    FrameworkSliceVm.cs                  Display-shape adapter for one row of the analyzer ItemsControl
    HexColor.cs                          "#RRGGBB" -> WinUI Color / Win32 COLORREF / cached SolidColorBrush
publish.ps1                              One-shot publisher -> dist/OSFrameworkLens/
```

Per-method documentation lives in the code itself (principle: "the best documentation is the method"). DESIGN.md only owns *why* — file purpose, layering rules, and the design decisions above.


## Future enhancements

1. Per-process framework rollup — aggregate across every top-level window of a process for a single headline number.
2. Persistent history / CSV export — log snapshots to SQLite so engineers can query their own questions.
3. Bulk scan mode — launch a list of executables, capture classifications, emit a fleet report.
4. Headless mode — `--json --process=X.exe` to stdout, for CI regression checks.
5. Distinguish Electron from generic Chromium — `app.asar` + `electron.exe` signature.
6. WebView2 URL context — DevTools-protocol query for navigated URL.
7. Diff over time — re-classify the same window before/after a Windows update.
8. Accessibility tree dump — full UIA tree to JSON for offline analysis.
9. Symbol-based DLL fingerprinting — catch renamed/repackaged framework runtimes.
10. Ship as Microsoft Store / WinGet package.
