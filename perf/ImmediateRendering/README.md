# RenderBench: comparative measurement kit for immediate-rendering backends

Measures the backend modes from
[the proposal](../../docs/design-notes/immediate-rendering-backends-proposal.md) on the target scenes from
[the test plan](../../docs/design-notes/immediate-rendering-backends-test-plan.md):

| Label | `WINUI_RENDERING_BACKEND` | Path |
|---|---|---|
| COMP | *(unset)* or `composition` | Existing retained Composition path (baseline) |
| GDIP | `gdiplus` | Immediate, GDI+ on the CPU |
| D2D-HW | `d2d-hw` | Immediate, Direct2D on a hardware D3D11 device |
| D2D-SW | `d2d-sw` | Immediate, Direct2D on WARP |

Until a backend sets its bit in `RenderingBackend::GetAvailableModes()`, a request for it falls back to Composition with
`FallbackReason=NotImplemented`. The gate marks those runs red, so they never count as backend data.

## Pieces

| Path | What it is |
|---|---|
| `dxaml\xcp\components\base\RenderingBackend.cpp` | Native seam: mode selection and the `Microsoft-WinUI-RenderingBackend` ETW provider (`{d18ce99c-dd66-5390-18da-068546739801}`). Events: BackendSelected, DeviceInfo, FrameBegin/End, RenderWalkStats, PaintBegin/End, ResourceInventory, UnsupportedContent |
| `RenderBench\` | WinUI harness app. Scenes S1–S16 and S19–S23 (see [Scenes](#scenes)); phases Warmup → Steady → Interaction → Recovery; `Microsoft-WinUI-RenderBench` markers; memory, GPU, GDI and CPU sampling; SendInput or programmatic input; screenshot |
| `RenderBench.Core\` | Shared options, mode names, EventSource, and the metrics model and calculator |
| `RenderBench.Analyzer\` | `analyze <etl>` → `metrics.json` + gate, `gate <json>...`, `report <dir>` → `report.md`, `records <etl>` (debug dump) |
| `RenderBench.Tests\` | Console test runner: analysis, options, WPR profile, and ETW round-trip tests; `--acceptance <dir>` |
| `profiles\ImmediateRendering.wprp` | WPR profiles `Timing`, `Memory` (adds the XAML profiler provider for DComp surface memory), `Gpu` |
| `scripts\Run-RenderBench.ps1` | Runs the matrix under WPR (modes in random order per repetition), then analyzes and reports |
| `scripts\Stage-LocalRuntime.ps1` | Copies `BuildOutput\packaging\Release\runtimes\win10-x64\native\*` next to `RenderBench.exe` |

## Use

```powershell
cd perf\ImmediateRendering
dotnet build RenderBench.Tests\RenderBench.Tests.csproj -c Release
dotnet run --project RenderBench.Tests\RenderBench.Tests.csproj -c Release --no-build   # elevated: also ETW tests
$env:RENDERBENCH_WPR_SMOKE = '1'   # optional: start/stop each WPR profile (cancels a running WPR session)

dotnet build RenderBench\RenderBench.csproj -c Release
.\scripts\Stage-LocalRuntime.ps1          # after: .\initrun.ps1 -Flavor amd64fre .\build.cmd /q product  (local perf: init /nopgo)
.\scripts\Run-RenderBench.ps1 -Repetitions 5                 # elevated; full matrix, Timing profile
.\scripts\Run-RenderBench.ps1 -Profile Memory -Repetitions 3 # memory attribution runs
dotnet run --project RenderBench.Tests\RenderBench.Tests.csproj -c Release --no-build -- --acceptance ..\results\<stamp>
```

Every run directory contains `trace.etl`, `metrics.json`, `screenshot.bmp`, and `exitcode.txt`. Timing ETLs grow by about
15 MB/s; pass `-DeleteTraces` for large matrices.

## Scenes

`--scene <name>` (or the plan id, e.g. `--scene S3`). Each scene is deterministic for `--seed`. Defaults apply unless the
flag is given explicitly; variant flags are rejected for other scenes. `Run-RenderBench.ps1 -Scenes` takes the matrix
entry names in the last column, plan-id aliases (`s1`..`s23`, `s11-nointerop`, `s12-noplayer`, `s16-nomedia`) or the
groups `full-backend` and `all` (default: S1–S10 and their variants, S1/S2 first). Window titles are
`RenderBench <PlanId> <label> [<backend>]`, where the label includes the variant (e.g. `Lottie-NoPlayer`). Details,
hypotheses and gate rules per scene are in the test plan, "Extended scenes" and "Full-backend scenes".

| Id | `--scene` | Content | Interaction | Defaults (`--interaction`, `--update-hz`, `--input-hz`, `--scroll-speed`, `--items`) | Variants (matrix entry) |
|---|---|---|---|---|---|
| S1 | `settings` | Settings page: cards, toggles, radios, slider | clicks on toggles/radios | 30 s, –, 2 Hz | – (`settings`) |
| S2 | `dashboard` | 320 tiles + chart, updated | tile clicks + 10 Hz updates | 30 s, 10 Hz, 2 Hz | `--tiles`, `--update-fraction` (`dashboard`) |
| S3 | `inbox` | Virtualized `ListView` of 10,000 seeded mails (avatar ellipse + initials, sender, trimmed subject, time) | continuous scroll, reversing at ends | 20 s, –, 10 Hz (wheel), 1500 DIP/s, 10000 | – (`inbox`) |
| S4 | `reader` | `RichTextBlock`, 25 paragraphs of mixed fonts/weights/italics/sizes + Hyperlink, in a `ScrollViewer` | slow scroll + selection changes | 20 s, –, 2 Hz, 60 DIP/s, 25 | – (`reader`) |
| S5 | `gallery` | `GridView` of 500 seeded 256×256 PNGs shown at 128×128, full-size decode | scroll + hover zoom (1.1×) | 20 s, –, 2 Hz, 800 DIP/s, 500 | `--decode-to-size` (`gallery-decoded`) |
| S6 | `animated` | ProgressRing (Lottie), indeterminate ProgressBar, fading InfoBar, Expander toggling every 2 s, ToggleSwitches | toggles + InfoBar text updates | 20 s, 0.5 Hz, 1 Hz | `--no-progressring` (`animated-noring`) |
| S7 | `layered` | Mica backdrop, acrylic card, ThemeShadow vs. no-shadow cards, nested opacity 0.8/0.6, translucent overlays and rounded border over text | overlay visibility toggles + opacity cycling | 20 s, 2 Hz, 1 Hz | – (`layered`) |
| S8 | `datagrid` | `ItemsRepeater` rows, 50 columns × 1,000 rows of bordered cells, frozen header | 2% of visible cells at 10 Hz + alternating V/H scrolls | 30 s, 10 Hz, 0.5 Hz, –, 1000 | – (`datagrid`) |
| S9 | `multiwindow` | N windows of S1 (`--windows 1\|5\|10`, default 10) | lifecycle steps: baseline, open-1..N, minimize-all, restore-all, close-all, settled; one per `--step-seconds` (2) | 30 s | `--tabs`: one TabView of 20 S1 tabs (`multiwindow-tabs`) |
| S10 | `form` | 3 TextBoxes, ComboBox, CheckBox, ToggleSwitch, Slider, Button+Flyout, Button+ToolTip, Submit | typing, Tab focus, hover, ComboBox/Flyout/ToolTip open/close | 30 s, –, 10 Hz | – (`form`) |
| S11 | `interop` | `SetElementChildVisual` SpriteVisual with an expression animation, `GetElementVisual` + implicit Offset animation, `ConnectedAnimation` between two views | view swaps + card moves | 20 s, 2 Hz, 1 Hz | `--no-interop`: same layout without composition interop, must stay immediate (`interop-nointerop`) |
| S12 | `lottie` | ProgressRings, `AnimatedIcon` buttons, CheckBox, `AnimatedVisualPlayer` with a hand-coded `IAnimatedVisualSource` (rotating ShapeVisual) | pointer-state changes on AnimatedIcons | 20 s, –, 1 Hz | `--no-player` (`lottie-noplayer`) |
| S13 | `materials` | Mica window backdrop, `AcrylicBrush` panes, ThemeShadow cards (Translation Z), elevation borders, `LinearGradientBrush` fills | card elevation + gradient changes | 20 s, 2 Hz, 1 Hz | – (`materials`) |
| S14 | `popups` | 560×400 window; Flyout, MenuFlyout, ComboBox, TeachingTip, ToolTip and Popup with `ShouldConstrainToRootBounds=false` placed partly outside the window | open-1..6 / closed steps, then open/close cycles | 30 s, –, 1 Hz | – (`popups`) |
| S15 | `independent` | 2000-item list scrolled with animated `ChangeView`, looping opacity/translate Storyboard, ProgressRing; UI thread blocked 1000 ms every 4 s | animated scroll | 20 s, –, –, 600 DIP/s, 2000 | – (`independent`) |
| S16 | `swapchain` | `SwapChainPanel` with a raw-COM D3D11 swap chain cleared to an animated color + `MediaPlayerElement` playing a generated clip | continuous presents | 20 s | `--no-media`: SwapChainPanel only, still falls back (`swapchain-nomedia`) |
| S19 | `text` | RTL Arabic/Hebrew, Devanagari/Thai, color emoji, a CFF (.otf) font if installed, Segoe UI Variable weights, ClearType vs grayscale areas, RichTextBlock mixed runs, 72 pt text, 1000 TextBlocks in an ItemsRepeater | scroll | 20 s, –, –, 300 DIP/s, 1000 | – (`text`) |
| S20 | `dpitheme` | Settings-like page; per cycle an `AppWindow.Resize`, every 5th a `RequestedTheme` toggle, every 25th a move to a monitor with another DPI (if any) | `--items` cycles | 60 s, –, –, –, 100 | – (`dpitheme`) |
| S21 | `devicelost` | S2 dashboard with `WINUI_IMMEDIATE_SIMULATE_DEVICE_LOST` set (`--device-lost <frames\|every:N>`, default `every:10`; immediate modes only) | 30 Hz updates | 45 s, 30 Hz | – (`devicelost`) |
| S22 | `accessibility` | 50 controls; keyboard focus visuals; every 10th move `RenderTargetBitmap` of a subtree vs the on-screen capture (`WINUI_IMMEDIATE_CAPTURE` BMP in immediate modes) | `Focus(Keyboard)` moves | 30 s, –, 4 Hz, –, 50 | – (`accessibility`) |
| S23 | `appshell` | NavigationView shell: settings pages (cards, toggles, expanders), Explorer-like ListView (5 columns × 2000 rows), gallery grid of control samples | navigate through 10 pages | 30 s, –, 0.5 Hz, –, 2000 | – (`appshell`) |

- S5 writes its images once per seed to `%TEMP%\RenderBench\images\seed<N>` (a `complete.txt` marker holds the count).
- S6 animates during Steady by design: `SteadyStateIsIdle` is waived and Steady CPU is reported as the *animation tax*
  (E1 animation CPU ms/s).
- S9's Interaction is its lifecycle: it lasts max(`--interaction`, steps × `--step-seconds`).
- S11, S12 (with the player) and S16 expect the native `WindowFallback` event (RenderingBackend provider; Mode, RootId,
  Reason) in immediate modes: the window switches to Composition. `interop-nointerop` and `lottie-noplayer` must not
  fall back; `swapchain-nomedia` still has the SwapChainPanel and falls back.
- S16 generates its clip once with `MediaComposition` into `%LOCALAPPDATA%\RenderBench\media\colors.mp4`; if that or
  the D3D11 swap chain fails, the `MediaAvailable`/`SwapChainAvailable` scene values are 0 and that part is skipped.
- S19 reports `CffFontPresent` (0 when no system `.otf` with CFF outlines is found; that variant is skipped).
- S20's DPI move needs two monitors with different DPI (`DpiChangeAvailable`); otherwise DPI change is manual.
- S21/S22 hooks are environment variables set by RenderBench at startup for immediate modes (written to
  `stderr.txt` as `startup:` lines); captures go to `%LOCALAPPDATA%\RenderBench`. High contrast is manual-only.
- S17 (WebView2) and S18 (InkCanvas) are out of scope.

## Input modes

`--input-mode auto|sendinput|programmatic` (default `auto`). In a disconnected or remote (RDP) session `SendInput` doesn't
reach the app, so `auto` uses `sendinput` only when the process is in the active console session *and* a probe mouse
move changes `GetLastInputInfo`; otherwise `programmatic`. The choice and reason are in the `InputModeSelected` event
and `metrics.json` (`InputMode`, `InputModeReason`).

- **sendinput:** real clicks, wheel notches, mouse moves, drags and keystrokes (`KEYEVENTF_UNICODE`, Tab, Escape).
- **programmatic:** the same state changes through the XAML API on the UI thread at the same rate (toggle `IsOn` /
  `IsChecked`, `ScrollViewer.ChangeView`, `RichTextBlock.Select`, `TextBox.SelectedText`, `Focus(Keyboard)`,
  `VisualStateManager.GoToState`, `IsDropDownOpen`, `Flyout.ShowAt`, `ToolTip.IsOpen`).

Both emit the same `InputInjected` markers; programmatic ones get a `programmatic:` kind prefix. **R1 counts real input
only** (sendinput runs); programmatic changes are reported separately as "programmatic change → frame". Use a console
session with `-InputMode sendinput` for latency numbers (R1, S10 typing latency).

## Screenshots

`--screenshot-method auto|screen|printwindow|xaml` (default `auto`). `screen` copies the client area from the screen DC;
that is black (or white) when the session has no visible desktop (disconnected/remote/locked). `auto` falls back to
`PrintWindow(PW_RENDERFULLCONTENT)` when the screen copy is one uniform color, and then to XAML `RenderTargetBitmap` of
the scene root when that is uniform too (a disconnected RDP session gives uniform results for both: DWM isn't
composing). `xaml` uses `RenderTargetBitmap` directly. The method used is recorded in `ScreenshotSaved`.
`RenderTargetBitmap` is the framework's own render: it has no window backdrop (S7's Mica area is black), no popups or
other windows, and isn't what DWM shows, so use a console session (`screen`) for C1 fidelity comparisons.

## Composition engine and XamlOptionalChanges

`Run-RenderBench.ps1 -CompositionEngine Lifted|System -OptionalChanges None|All` (RenderBench:
`--composition-engine lifted|system --optional-changes none|all`). `Program.cs` owns `Main` (`DISABLE_XAML_GENERATED_MAIN`)
so both are applied before `Application.Start`, following the wiki "Opting Into WinUI 3 Composition Engine &
XamlOptionalChanges Performance APIs". `System` applies to composition mode only: the script sets the internal
`HKLM\Software\Microsoft\WinUI\RenderingEngine\Switcher\ProcessOverrides\RenderBench.exe=1` override for the run (restoring
any previous value afterwards; needs a supporting internal OS build), and RenderBench calls
`CompositionEngine.TrySetProcessEngine(System)`. If that returns false, the run exits 4 instead of measuring Lifted.
`All` enables every `XamlChangeId`. Results of both calls are written to the run's `stderr.txt` as `startup:` lines.
Private memory excludes DWM, so System-engine numbers understate composition's total cost.

## Extended metrics

Added to `metrics.json` and the report (test plan metric ids in parentheses):

- Per phase: frame interval percentiles between consecutive drawn frames, dropped frames (interval > 1.5 × vsync,
  from `DisplayInfo` refresh rate, 60 Hz assumed), long frames (FrameBegin → FrameEnd UI work > 1.5 × vsync), longest
  gap (R2/R3 during scroll), and
  scene counters (`SceneValue`: realized items/rows, image load ms, estimated decoded MB, open windows, typed chars).
  For non-scrolling scenes gaps > 250 ms count as idle, not dropped.
- Animation CPU per second during Steady for scenes that animate while idle (S6; E1).
- S9 steps: private/working-set MB per lifecycle step, marginal private MB per window (slope over baseline + open-*),
  and fraction reclaimed after close (M1).
- R1 by input kind (`click`, `wheel`, `key`, `tab`, `drag`, ...) and popup open time (input → first frame after
  `PopupOpened`; S10).
- Unsupported content by type and detail, split into expected and unexpected per scene (S6 ProgressRing /
  AnimatedVisualPlayer, S7 acrylic / mica / shadow); `NoUnsupportedContent` fails only on unexpected content. Details
  ending in `:approximated` (e.g. Mica's fallback color) are always allowed, and content in a scene that expects a
  window fallback is allowed once the fallback was observed.
- Full-backend scenes: `WindowFallbackCount`/`WindowFallbackReasons`/`ExpectsWindowFallback`; S15 `UiBlocks`
  (frames drawn and max frame gap per deliberate UI-thread block), `FramesDuringUiBlock`,
  `MaxFrameGapDuringUiBlockMs` and the screen probe values `BlockScreenChanges`/`BlockMaxScreenGapMs`; S20/S23
  `ChangeToFrameMsByKind` (resize, theme, dpi, navigate) and `UnmatchedChanges`; S20 `CycleLeakPrivateMB`
  (cycle-100 minus cycle-10); S21 `DeviceLostRecoveryMs` and `DeviceLostLeakPrivateMB`; S14 `PopupOpenMsByKind` and
  `PrivateMBPerOpenPopup`; S22 `RtbSuccessRate` and `CaptureMeanAbsDiff` (mean absolute BGRA difference, 0..255).

## Gate (per run)

`HarnessRunStarted`, `HarnessCompleted`, `BackendSelectedPresent`, `RequestReachedRuntime`, `EffectiveMatchesRequested`,
`DeviceMatchesMode` (D2D-SW must be WARP, D2D-HW must not be), `FramesObserved`, `SteadyStateIsIdle` (at most 6 drawn
frames/min; waived with "animation tax" for S6), `MemorySampled`, `NoUnsupportedContent` (expected content per scene is
allowed; see Extended metrics), `WindowFallbackAsExpected` (immediate modes: S11/S12/S16 pass only if a
`WindowFallback` was observed, every other scene fails if one was; n/a for composition). Scene gates: S15
`UiBlockObserved`; S20 `RecoveryWithinLimit` (p95 change-to-frame per kind ≤ 1000 ms, no unmatched changes) and
`NoCycleLeak` (< 2 MB); S21 immediate `DeviceLostObserved`, `RecoveryWithinLimit` (≤ 1000 ms) and `NoCycleLeak`
(n/a below 100 losses); S22 `RenderTargetBitmapOk` (every render succeeded) and `CaptureWithinTolerance` (mean abs
diff ≤ 8). Reports compute medians from gated runs only.

## Notes

- R1* is input-injected → next drawn UI-thread frame end. It's a proxy; visible latency needs DWM present correlation (Gpu profile).
- GPU memory is `IDXGIAdapter3::QueryVideoMemoryInfo` for this process. `GpuLocalBytes`/`GpuNonLocalBytes` are
  summed over all adapters, as before. Each sample also records per-adapter usage (`GpuAdapterMemorySample`, keyed by
  LUID), and the report's "M3 ... render adapter" row uses the adapter whose LUID is in `DeviceInfo`. On a machine
  without a hardware GPU (for example a Hyper-V VM), GPU memory reads 0.
- Measure fre builds. chk builds capture an allocation stack per allocation.
- Harness limitations seen on a disconnected Hyper-V/RDP session: SendInput doesn't reach the app, so input mode is
  programmatic and there are no R1/typing-latency numbers. Screen copy and PrintWindow return a uniform color, so
  screenshots come from RenderTargetBitmap, which fails under immediate backends (logged to stderr; no screenshot). WARP
  Composition presents at about 30 fps, so "dropped frames" against a 60 Hz vsync are inflated for every mode. Compare
  backends against each other on the same machine, not against the absolute threshold.

## GDI+ readiness

- **ResourceChurn** (Resources keyword 0x8; fields Mode, FrameNumber, Created, Destroyed) is emitted with `RenderingBackend::TraceResourceChurn`
  once per painted frame. The analyzer turns it into W6 created/destroyed per second per phase. A phase with no ResourceChurn events
  reports null, not 0, so a backend that never emits it is visibly unmeasured.
- M7: the report shows GDI objects (steady, interaction max, and interaction growth, which is last minus first sample) and USER objects.
  GDI+ creates GDI objects only for text interop and the presentable copy, so growth should be 0.
- The `Memory` profile adds heap allocation events with stacks for `RenderBench.exe`. `Run-RenderBench.ps1 -Profile Memory`
  runs `wpr -heaptracingconfig RenderBench.exe enable/disable` around the matrix. Use WPA *Heap Allocations* to attribute GDI+
  bitmap, path, and glyph-cache memory. `VirtualAlloc` covers large DIB sections.
- The native GDI+-specific tests are in `dxaml\xcp\components\base\unittests\GdiPlusPainterTests.cpp`: runtime refcount, PARGB,
  antialiasing, half-pixel offset, grayscale text, the 200-frame GDI handle/cache leak check, churn, and damage-only upload. They stay
  red (E_NOTIMPL) until `CreateGdiPlusPainter` renders. The upload/copy accounting contract is documented in `GdiPlusPainter.cpp`.

## D2D-HW readiness

D2D-HW numbers are meaningful only on a machine with a real GPU.

- **Adapter policy** (native `D2DDeviceFactory.cpp`, mirrored in `RenderBench.Core\Adapters.cs`):
  - Adapters come from `EnumAdapterByGpuPreference(HIGH_PERFORMANCE)`, falling back to `EnumAdapters1`.
  - Software adapters and all Microsoft (`0x1414`) adapters are rejected: Basic Render, and Hyper-V Video / Remote
    Display, which DXGI exposes as Basic Render.
  - `WINUI_RENDERING_ADAPTER_LUID=<hex>` pins one adapter. The pin is strict: if that adapter doesn't qualify, nothing
    is chosen.
  - With no qualifying adapter, device creation returns `DXGI_ERROR_NOT_FOUND`; it never falls back to WARP.
- **`RenderBench.Analyzer adapters`** lists the adapters and their verdicts. Exit codes: 0 means a qualifying adapter
  exists, 1 means none, 2 means a bad override.
- **Runs without a qualifying adapter:**
  - `Run-RenderBench.ps1` runs the `adapters` check. When there's no qualifying adapter, it writes each d2d-hw run
    with `RenderBench.Analyzer skip <scene> d2d-hw "no hardware adapter" --out <metrics.json>` instead of launching it.
  - Skipped runs are not gate failures. The report shows `skipped: <reason>` in the gate row and `n/a (no GPU)` in the
    value cells.
- **W3 GPU time:**
  - `GpuTimestampProfiler` wraps a paint in D3D11 timestamp/disjoint queries and reads them back a frame or two later.
    It emits `GpuPaintTime` (FrameNumber, GpuMicroseconds) on the RenderingBackend provider (Paint keyword).
  - The report row is "W3 GPU paint p50/p95, interaction". Readings are attributed to a phase by their frame's end time.
- **Gpu WPR profile:**
  - DxgKrnl with keywords `0x2F7`: base, profiler, references, CDD, resource, memory, and power. Rundown runs at start
    and save, for the WPA GPU Utilization and GPU memory tables.
  - Microsoft-Windows-DXGI, Microsoft-Windows-Direct3D11, and the RenderingBackend provider.

## D2D-SW readiness: CPU split and WARP golden reference

- **Per-thread CPU split (W1/W2).** With D2D-SW and GDI+, raster work moves onto CPU threads, so the harness's
  `ThreadCpuSampler` enumerates the process's threads (Toolhelp32 + `GetThreadTimes`) at each memory sample. It
  classifies each thread once, by the module containing its Win32 start address (`ThreadCategories.Classify`):
  - `Ui`: the dispatcher thread.
  - `WarpRasterizer`: `d3d10warp.dll`.
  - `D2DDWrite`: `d2d1`, `dwrite`, or `DWriteCore`.
  - `Composition`: `dcomp*`, `Microsoft.UI.Composition*`, `CoreMessaging*`, or `dwmcorei.dll` (the in-process
    compositor).
  - `Other`.

  `ThreadCpuSample` (event 20) carries cumulative milliseconds. `metrics.json` has per-phase `ThreadCpu`, and the
  report has `W2 CPU <category>` rows. At run end, `ThreadModuleSummary` (event 21) lists what actually ran, so the
  rules can be checked.
- **Caveat.** In smoke runs, WARP and thread-pool work start in `ntdll.dll`, and .NET runtime threads start in
  `coreclr.dll`. Both land in `Other`, so `WarpRasterizer` can read 0 even when WARP is busy. Use the Timing profile
  (CPU samples, CSwitch and ReadyThread, all with stacks) in WPA to attribute `Other` by stack.
- **WARP golden reference (C1).** WARP is deterministic for a given binary.
  `D2DSoftwarePainterTests::TwoWarpDevicesProduceIdenticalPixels` checks this across devices, so D2D-SW output can
  serve as the C1 reference image. Goldens are only valid for the same `d3d10warp`/`d2d1`/`dwrite` versions. Each run
  records them in `metrics.json` `RasterModuleVersions` (event 22).
- **W5 copies for D2D-SW.** `uploadBytes` is `damagedPixels × 4` for frames presented through a swap chain: it is the
  system-memory WARP target copied into the back buffer. It is 0 for offscreen frames. See the contract in
  `dxaml\xcp\components\base\D2DPainter.cpp`.
- **Device loss.** Native `DeviceLost` events are counted in `metrics.json` `DeviceLostCount` and in the report.
