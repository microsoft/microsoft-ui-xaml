# Comparative test plan for immediate-rendering backends

Status: proposed test plan for the
[retained XAML with selectable immediate-rendering backends](immediate-rendering-backends-proposal.md) experiment.
No harness, instrumentation event, or result described here exists yet unless stated otherwise.

## Table of Contents

- [Purpose](#purpose)
- [Backends under comparison](#backends-under-comparison)
- [Run protocol](#run-protocol)
- [Metric catalog](#metric-catalog)
- [Test cases](#test-cases)
- [Extended scenes](#extended-scenes)
- [Full-backend scenes](#full-backend-scenes)
- [Required WinUI instrumentation](#required-winui-instrumentation)
- [Comparative report format](#comparative-report-format)
- [Measurement pitfalls](#measurement-pitfalls)
- [Backend decision scenarios (S24-S32)](#backend-decision-scenarios-s24-s32)
- [GDI+ bounded layers](#gdi-bounded-layers)
- [S33 scroll physics (DManip ticking)](#s33-scroll-physics-dmanip-ticking)

## Purpose

Produce a comparative view of the four backend choices on identical content and interactions. The aim is not one
overall winner; it is to show, per workload, what each backend saves and what it costs in memory, responsiveness,
CPU/GPU work, energy, startup, correctness, and stability.

Every test case below lists the metrics to record (by ID from the [metric catalog](#metric-catalog)) and the
signal that matters when comparing backends.

## Backends under comparison

| Mode ID | Backend |
|---|---|
| `COMP` | Existing Composition path (baseline) |
| `GDIP` | Immediate rendering, GDI+ painter (CPU), including GDI+ text |
| `D2D-HW` | Immediate rendering, Direct2D on a hardware device |
| `D2D-SW` | Immediate rendering, Direct2D on WARP |

Mode IDs are report labels, not API names.

## Run protocol

These rules apply to every test unless a test explicitly varies one of them.

- **One harness app.** Identical XAML, data, and scripted interactions for all modes. Select the mode at process
  startup. Do not build per-mode variants of a scene.
- **Verify identity first.** Each run logs requested mode, effective mode, adapter/device type (hardware or WARP),
  fallback, and retained-layer promotions. Discard runs where the effective mode differs from the requested one.
- **Check correctness before timing.** A run whose screenshot or behavior check fails is reported as a failure,
  not as a performance number.
- **Build and machine.** Optimized (fre) build, no debugger attached. Fixed display resolution, DPI, and refresh
  rate. AC power, fixed power plan, background activity minimized. Record OS build, GPU, driver, and CPU.
- **Phases.** Emit ETW phase markers (`Warmup`, `Steady`, `Interaction`, `Recovery`). Steady-state memory is
  sampled only after warmup and after an explicit idle settle period (for example 10 s).
- **Repetition.** At least 5 runs per mode per test (more for latency and energy). Randomize mode order across
  runs. Report median and spread (for example p25 to p75), not a single run.
- **Cold versus warm.** Report process-cold startup (first launch after a reboot or file-cache flush) separately
  from warm startup.
- **Machines.** Start with one integrated-GPU and one discrete-GPU machine. Add a CPU-constrained device and a
  remote session once the core hypothesis holds.

## Metric catalog

### Memory

| ID | Metric | How to measure |
|---|---|---|
| M1 | App private commit (steady, peak) | `GetProcessMemoryInfo` with `PROCESS_MEMORY_COUNTERS_EX2` (`PrivateUsage`) sampled by the harness; peak over the interaction phase. Cross-check with WPR `VirtualAllocation` profile in WPA |
| M2 | App working set (steady, peak) | Same API (`WorkingSetSize`, `PeakWorkingSetSize`). Report separately from M1; the two are not additive |
| M3 | App GPU memory: local and non-local | `IDXGIAdapter3::QueryVideoMemoryInfo` (`CurrentUsage`) for both segment groups, sampled in-process; cross-check with WPR `GPU` profile (GPU memory views in WPA) |
| M4 | Compositor (DWM) attributable delta | Measure `dwm.exe` commit and GPU memory with the scene shown versus a matched baseline (same window, blank content) in the same session. Label as approximate; DWM memory is shared |
| M5 | Heap breakdown | WPR `Heap` profile for the harness process; group allocations by WinUI module/stack to separate XAML tree, render data, caches |
| M6 | Rendering resource inventory | New WinUI counters (see [instrumentation](#required-winui-instrumentation)): Composition visuals, retained layers, surface count and bytes, cache bytes, temporary surface bytes |
| M7 | GDI and USER objects | `GetGuiResources` (`GR_GDIOBJECTS`, `GR_USEROBJECTS`) |
| M8 | Memory growth trend | Slope of M1 and M3 across repeated cycles (MB per 100 cycles) after warmup |

### Responsiveness and frames

| ID | Metric | How to measure |
|---|---|---|
| R1 | Input-to-visible latency p50/p95/p99 | Harness injects input (`SendInput` or `InjectSyntheticPointerInput`) and logs an ETW timestamp; a `Windows.Graphics.Capture` session detects the first captured frame whose pixels reflect the change and records its `SystemRelativeTime`. Validate periodically against a high-speed camera or photodiode for ground truth |
| R2 | Frame-deadline misses | Expected updates per second (for example during scroll) versus distinct visible updates observed in the capture stream, plus WinUI frame ETW events aligned with DWM frame timing (`DwmGetCompositionTimingInfo` or DWM ETW events) |
| R3 | Longest visible stall | Maximum gap between consecutive visible updates during an interaction that should update every frame |
| R4 | Independent-motion continuity | During a deliberate UI-thread stall, count visible updates of compositor-eligible motion; flag stale or unpainted exposed regions |

App-side `Present` events alone (for example from PresentMon) are not a reliable frame signal here, because
Composition content updates are committed to DWM rather than presented through an app swap chain. Use the capture
stream and WinUI/DWM events, and say which signal each result uses.

### Work performed

| ID | Metric | How to measure |
|---|---|---|
| W1 | UI-thread CPU time per frame, split by phase | WPR `CPU` profile plus WinUI ETW regions (layout, render walk, command generation, rasterization, commit or present). Analyze in WPA by region |
| W2 | Total process CPU time | `GetProcessTimes` delta over the phase; ETW CPU usage (precise) for per-thread breakdown |
| W3 | GPU time | WPR `GPU` profile, GPU utilization per process in WPA or GPUView. For `D2D-HW`, optionally D3D timestamp queries around the painter |
| W4 | Requested damage versus repainted area | New WinUI counters: pixels invalidated, pixels actually repainted, number of damage rectangles per frame |
| W5 | Upload and copy bytes | New WinUI counters for CPU-to-GPU uploads and surface copies (important for `GDIP` and `D2D-SW`); cross-check with WPR `GPU` profile DMA packets |
| W6 | Resource churn | New WinUI counters: surfaces, textures, and brushes created or destroyed per second |

### Startup, idle, and energy

| ID | Metric | How to measure |
|---|---|---|
| S1 | Time to first correct frame | Process creation (ETW `Microsoft-Windows-Kernel-Process`) to first captured frame matching the reference image |
| S2 | Time to first interactive frame | Process creation to the first successful input round trip (R1 method) |
| E1 | Idle wakeups and redraws | WinUI frame/tick ETW events and CPU context switches per minute while idle (WPR `CPU` profile) |
| E2 | Energy per scripted sequence | External power meter or on-board energy counters where available, minus a matched idle baseline. `powercfg /srumutil` is suitable for coarse trends only |
| E3 | Average package power during interaction | Same source as E2, reported over the interaction phase |

### Correctness and stability

| ID | Metric | How to measure |
|---|---|---|
| C1 | Visual difference versus reference | Capture via `Windows.Graphics.Capture` (not `RenderTargetBitmap`, which has its own capture path). Compare against `COMP` and a reviewed golden image. Use per-region tolerances: tight for geometry and fills, defined antialiasing tolerance for text edges |
| C2 | Stale-pixel detection | After each scripted change, compare the capture against a full-redraw reference of the same state. Any mismatch outside tolerance is a damage bug |
| C3 | Behavioral equivalence | Hit-testing probes, focus order, keyboard navigation, selection, caret position, UI Automation tree and bounding rectangles compared with `COMP` |
| C4 | Failures and recovery time | Crashes, failed frames, device-lost recovery duration (event to first correct frame) |

## Test cases

Each case runs in all four modes. "Compare" names the signal that differentiates backends.

### Scaling experiments

These explain why a backend wins or loses.

| ID | Test | Setup and steps | Variations | Metrics | Compare |
|---|---|---|---|---|---|
| T1 | Element-count scaling | Grid of simple bordered text cells; fixed window size; painted area held roughly constant by shrinking cells | 100, 1,000, 10,000 realized elements | M1, M3, M4, M6, W1, S1 | Memory per element and traversal cost per element; slope matters more than absolute values |
| T2 | Pixel-area scaling | Same tree; vary physical pixels | Window sizes 1280x720, 1920x1080, 3840x2160; DPI 100%, 150%, 200% at fixed logical size | M1, M3, M6, W1, W3, W5 | Where surface memory and fill/upload cost overtake object savings |
| T3 | Damage scaling | Dashboard grid; update a set of cells each tick at 30 Hz | 1%, 10%, 100% of viewport; contiguous versus scattered | W1, W3, W4, W5, R2, E3 | Whether work tracks damaged area; cost of scattered small damage |
| T4 | Primitive mix | One scene per primitive type at the same coverage | Text-only, images, paths, gradients, nested clips | W1, W3, M3, M6, C1 | Per-primitive cost and fidelity for each painter |
| T5 | Overlap and transparency | Stacked translucent panels; group opacity on overlapping children | Overlap depth 1, 4, 16 | W1, W3, M6 (temporary surfaces), C1 | Overdraw cost and correctness of group-opacity semantics |
| T6 | Retained-boundary scaling | Fixed ordinary content plus N independently animated elements | N = 0, 4, 16, 64 | M3, M6 (layers, promotions), W1, R2 | Point where layer promotion erases immediate-mode memory savings |

### Representative workloads

These show whether savings matter in realistic apps.

| ID | Test | Script | Metrics | Compare |
|---|---|---|---|---|
| T7 | Static settings page | Launch, idle 60 s, hover 10 controls, toggle one switch | S1, S2, M1-M4, E1, E2, R1 | Steady memory and idle cost for the most common app shape |
| T8 | Dense dashboard | 200 live indicators; update 10% at 10 Hz for 5 minutes | M1-M4, W1, W3, W4, E3 | Memory and energy under sustained small updates |
| T9 | Virtualized text list | 10,000 items; slow scroll, fling, reverse, jump to end, append items | R1, R2, R3, M1, M3, W1, W5, C2 | Scroll smoothness, exposure correctness, raster churn |
| T10 | Text editing | Type a 500-character paragraph at a fixed rate; select, scroll, use IME composition, RTL and mixed scripts | R1 (keystroke to glyph), W1, C1, C3 | Typing latency and text fidelity per painter |
| T11 | Image gallery | 500 images; scroll and zoom; run once with preloaded decoded images and once with normal decoding | M1, M3 (peak), W5, R2 | Upload cost and peak memory; isolate decode from rendering |
| T12 | Animation mix | Run separately: (a) opacity and translate animations only; (b) animations that change content each frame (color, size, text) | R2, R3, W1, W3, M6 | Retained-boundary benefit for (a); repaint cost for (b) |
| T13 | Multiwindow | Open 1, 5, then 10 windows of T7; minimize all, restore all, close all | M1-M4 per step, M8, S1 per window | Marginal per-window cost and reclamation after close |
| T14 | Interop scene | XAML plus hosted Composition visual, `SwapChainPanel`, and media playback | C1, C3, R2, M3, M6 (promotions) | Supported/promoted/unsupported classification and ordering correctness |

### Responsiveness under stress

| ID | Test | Script | Metrics | Compare |
|---|---|---|---|---|
| T15 | UI-thread stall during motion | During T9 fling and T12(a) animation, block the UI thread for 200 ms via a harness hook | R4, R3, C2 | Whether motion continues and newly exposed content is valid. Do not expect UI-thread-bound work (typing) to continue |
| T16 | Input burst | 120 pointer moves per second over hoverable items for 10 s | R1, R2, W1 | Tail latency under event pressure |
| T17 | Constrained CPU | Repeat T8, T9, T10 with CPU affinity limited to 2 cores and a fixed background CPU load | R1, R2, W2, E3 | Sensitivity of `D2D-SW` and `GDIP` to CPU contention |

### Lifecycle, recovery, and stability

| ID | Test | Script | Metrics | Compare |
|---|---|---|---|---|
| T18 | Continuous resize | Resize window between minimum and maximum size for 30 s | R3, M3 (peak), W6, C2 | Surface reallocation cost and correctness after each size |
| T19 | DPI and monitor change | Move window between 100% and 200% monitors; change scale while open | C1, S1-style time to correct frame, M3 | Re-rasterization cost and text/geometry fidelity |
| T20 | Minimize, occlude, restore | Minimize 60 s, restore; fully occlude 60 s, reveal | E1 while hidden, M1, M3 while hidden, time to correct frame after restore | Resource trimming and recovery cost |
| T21 | Device loss | Trigger device removal (for example `dxcap -forcetdr` or a driver reset) during T9 | C4, C2, M8 | Recovery time and absence of stale or missing content; `D2D-SW` and `GDIP` paths must be tested even if unaffected |
| T22 | Navigation soak | Navigate between 5 pages 1,000 times | M1, M3, M7, M8, W6 | Leaks and growth slope |
| T23 | Long run | T8 plus periodic T9 scrolls for 30 minutes | M8, R2 over time, C2 sampled | Stability of memory and frame behavior over time |

### Correctness gate suite

Run before any performance comparison for a scene is accepted.

| ID | Test | Steps | Metrics |
|---|---|---|---|
| T24 | Damage correctness set | Move, remove, and resize overlapping elements; shrink text; change clip; reveal covered content; scroll both directions | C2 |
| T25 | Fidelity set | Fractional positions, rounded corners, gradients, image stretch modes, RTL text, emoji, ClearType versus grayscale text settings | C1 |
| T26 | Behavior set | Hit testing at element edges, tab order, focus visuals, selection, caret, UIA tree and bounds | C3 |

A backend that fails T24 to T26 for a scene is reported as "unsupported" or "incorrect" for that scene, and its
performance numbers for that scene are excluded from comparisons.

## Extended scenes

Implemented in `perf\ImmediateRendering` (RenderBench) as scenes S3 to S10, next to S1 *settings* (T7) and S2
*dashboard* (T8). Each is deterministic for `--seed`, runs the standard phases, and is selected with `--scene <name>`;
per-scene defaults and variants are in the kit README ("Scenes"). The kit metrics referenced below map to the catalog:

| Kit metric | Catalog id | Source |
|---|---|---|
| Frame interval p50/p95/p99, dropped frames (interval > 1.5 × vsync), longest gap, per phase | R2, R3 | Drawn `FrameEnd` events in the phase; refresh rate from `DisplayInfo` |
| Long frames (`FrameBegin` → `FrameEnd` > 1.5 × vsync) | W1 | Same |
| R1 by input kind (`click`, `wheel`, `key`, `tab`, `drag`) | R1 | `InputInjected` → next drawn frame end; SendInput runs only |
| Programmatic change → frame | (R1 proxy) | Same, for `programmatic:` inputs; not a latency number |
| Popup open time | R1 | Input → first drawn frame after `PopupOpened` |
| Animation CPU ms/s during Steady | E1, W2 | Process CPU over Steady for scenes that animate while idle |
| Memory per lifecycle step, marginal MB per window, fraction reclaimed | M1, M2 | `MemorySample` closing each `SceneStep` |
| Scene counters (realized items/rows, image load ms, estimated decoded MB) | M1 context | `SceneValue` |
| Unsupported content by type/detail, expected vs. unexpected | C3 | Native `UnsupportedContent` |

**Input.** `--input-mode sendinput` injects real input; `programmatic` performs the same state changes through the XAML
API at the same rate (`auto` picks programmatic outside an active console session). Programmatic runs are valid for
frame, memory and CPU comparisons but not for R1. **Gate.** All extended scenes use the standard per-run gate, with two
scene-specific rules: `SteadyStateIsIdle` is waived for S6, and `NoUnsupportedContent` fails only on content that isn't
listed as expected for the scene (below). Expected unsupported content is still counted and shown in the report's
unsupported-content row as expected, so fidelity comparisons for that scene must account for it.

### S3 inbox (`inbox`, T9)

- **Purpose:** scroll smoothness and virtualization churn with text-heavy list items.
- **Content:** `ListView`, 10,000 seeded items; template: 40 px `Ellipse` avatar (solid color) with initials, sender
  (SemiBold), subject (`CharacterEllipsis`), timestamp. Containers are filled in `ContainerContentChanging`.
- **Interaction:** continuous scroll at 1,500 DIP/s reversing at the ends: `ChangeView` once per frame (programmatic) or
  wheel notches at 10 Hz (SendInput).
- **Metrics:** R2, R3 and frame interval percentiles during Interaction; W1 long frames; M1/M2/M3 (Interaction peak);
  realized item count; W5 uploads for immediate modes.
- **Hypothesis:** COMP keeps per-item visuals and scrolls by offsetting retained content, so its UI-thread cost is
  realization only. Immediate modes repaint the exposed strip (or the viewport) every frame: GDIP's CPU text and
  ellipse antialiasing is the likely bottleneck (dropped frames), D2D-HW should track COMP, D2D-SW sits between them
  and is CPU-bound. Immediate modes should use less memory per realized item.
- **Required features:** scroll offset (render-transform) handling with damage limited to the exposed area or a cached
  scroll layer; text with trimming; antialiased ellipses; recycling without stale pixels (C2).
- **Gate:** no expected unsupported content.

### S4 reader (`reader`, T10, T25)

- **Purpose:** rich text fidelity and the cost of selection highlight and slow scrolling.
- **Content:** `RichTextBlock` in a `ScrollViewer`: a title and 25 seeded paragraphs mixing Segoe UI, Georgia and
  Consolas, Normal/SemiBold/Bold weights, italics, 14–18 px sizes, and a `Hyperlink`.
- **Interaction:** slow scroll (60 DIP/s, every frame) plus a selection change at 2 Hz (`Select` over visible text, or a
  SendInput drag).
- **Metrics:** R2, R3, frame interval percentiles; W1; R1 (`drag`) in SendInput runs; M1, M3; C1 on the final screenshot.
- **Hypothesis:** glyph-run rendering dominates. GDIP's text path (GDI+ text, not DirectWrite) differs most in fidelity
  and cost; D2D modes share DirectWrite with COMP and should match glyph shapes. Selection repaints only the selected
  lines in immediate modes if damage is tracked per line.
- **Required features:** multi-run text layout with mixed fonts, selection highlight, hyperlink underline/foreground,
  vertical scrolling.
- **Gate:** no expected unsupported content.

### S5 gallery (`gallery`, variant `--decode-to-size`, T11)

- **Purpose:** image decode and upload memory, and image scaling quality.
- **Content:** `GridView` of 500 seeded 256×256 PNGs (gradients and shapes, generated once per seed under
  `%TEMP%\RenderBench\images`) shown at 128×128. Default decodes at full size (`DecodePixelWidth` unset);
  `--decode-to-size` sets `DecodePixelWidth = 128`.
- **Interaction:** scroll at 800 DIP/s, plus hover zoom (`ScaleTransform` 1.1) on three visible items at 2 Hz
  (programmatic) or real mouse moves (SendInput).
- **Metrics:** M1, M3 peak and Recovery; estimated decoded MB; image load ms (`ImageOpened`); R2, R3; W5 uploads.
- **Hypothesis:** COMP holds decoded images in GPU surfaces; immediate modes hold CPU bitmaps (GDIP, D2D-SW) or device
  bitmaps (D2D-HW) and draw them scaled each repaint. Full-size decode quadruples image memory in every mode; the
  difference between the variants isolates decode memory from rendering. GDIP scaling is the slowest and lowest quality.
- **Required features:** `ImageBrush`/`Image` drawing with scale and the decode-size path, scale transforms on
  realized items, image cache eviction when items recycle.
- **Gate:** no expected unsupported content.

### S6 animated (`animated`, variant `--no-progressring`, T12)

- **Purpose:** the "animation tax": what an always-animating page costs when nothing else changes.
- **Content:** `ProgressRing` (`IsActive`; drawn by `AnimatedVisualPlayer`, a Lottie Composition visual), indeterminate
  `ProgressBar`, `InfoBar` whose `Opacity` fades forever (Storyboard, auto-reverse), an `Expander` toggled every 2 s,
  and four `ToggleSwitch`es (knob animation). The scene animates during Steady by design.
- **Interaction:** toggles at 1 Hz and InfoBar text updates at 0.5 Hz on top of the running animations.
- **Metrics:** E1 animation CPU ms/s during Steady (the tax), drawn frames per second in Steady and Interaction, R2,
  W1, M1, M3.
- **Hypothesis:** COMP runs the ring, bar and opacity animations as independent Composition animations, so the UI
  thread barely draws and the tax is mostly DWM/compositor work. Immediate modes must repaint each animated region on
  the UI thread every frame (or promote it to a retained layer), so their tax is much higher; GDIP and D2D-SW pay it in
  CPU. `--no-progressring` removes the Lottie content that immediate modes can't paint.
- **Required features:** render-loop ticking for independent animations (opacity, transform), damage per animated
  element, retained-layer promotion or an unsupported report for Composition-only visuals.
- **Gate:** `SteadyStateIsIdle` waived (detail notes the animation tax). Expected unsupported content: types containing
  `AnimatedVisualPlayer` or `ProgressRing` (default variant only). In `--no-progressring` nothing is expected.

### S7 layered (`layered`, T5, T14)

- **Purpose:** group-opacity correctness and how immediate modes treat Composition-only effects.
- **Content:** `MicaBackdrop` window backdrop with a transparent root; an `AcrylicBrush` card over colored ellipses;
  two `ThemeShadow` cards (`Translation` Z = 32, receiver set) beside identical cards without shadows; nested opacity
  (0.8 container over a 0.6 child with overlapping rectangles); a translucent overlay over text; a translucent rounded
  `Border` over text.
- **Interaction:** overlay visibility toggles (ToggleButton, CheckBox) at 1 Hz; overlay opacity cycles at 2 Hz.
- **Metrics:** C1 on the final screenshot (group opacity: the overlap of the two rectangles must not show through), M3,
  M6 temporary surfaces, W1, unsupported content by type.
- **Hypothesis:** immediate modes need an offscreen layer for group opacity (memory and fill cost); the rounded
  translucent border is ordinary content and must render. Acrylic, mica and shadows are Composition effects that
  immediate modes report unsupported (and draw the fallback color or nothing).
- **Required features:** opacity layers with correct group semantics, rounded-rectangle fill/stroke with alpha,
  visibility and opacity damage.
- **Gate:** expected unsupported content: detail `Brush` (the acrylic brush) and anything whose type or detail mentions
  `Shadow`, `Acrylic`, `Mica` or `Backdrop`. Unsupported opacity, rounded corners or text is a failure.

### S8 datagrid (`datagrid`, T1, T3)

- **Purpose:** many small text elements with scattered small updates and two-axis scrolling.
- **Content:** `ItemsRepeater` of 1,000 rows, each 50 bordered `TextBlock` cells (96×28), inside a two-axis
  `ScrollViewer`; a frozen header row in a separate `ScrollViewer` that follows horizontally only.
- **Interaction:** 2% of the visible cells change at 10 Hz; every 2 s an animated scroll alternates vertical and
  horizontal (programmatic `ChangeView`, or three wheel / horizontal-wheel notches).
- **Metrics:** W1, W4 (damaged vs. painted pixels), R2, R3 (during the scroll animations), M1, M3, realized rows/cells.
- **Hypothesis:** scattered small damage favors immediate modes only if damage is tracked per cell; a full-viewport
  repaint at 10 Hz makes GDIP and D2D-SW CPU-bound. COMP pays per-element visual memory for thousands of cells.
- **Required features:** many-rect damage union, clipping to the viewport, frozen-header composition (two scroll
  viewers in one window), border strokes at pixel boundaries.
- **Gate:** no expected unsupported content.

### S9 multiwindow (`multiwindow`, `--windows 1|5|10`, variant `--tabs`, T13)

- **Purpose:** marginal per-window cost and reclamation.
- **Content:** N windows (default 10), each a new `Window` hosting an S1 Settings instance (seed + i). `--tabs`: one
  window with a `TabView` of 20 S1 tabs, content realized on first visit.
- **Interaction:** one lifecycle step every `--step-seconds` (2 s) with a `SceneStep` marker and memory sample: baseline,
  open-1..N, minimize-all, restore-all, close-all, settled (after a full GC). `--tabs`: baseline, open-tab-2..20,
  switch, close-tabs, settled.
- **Metrics:** M1/M2 per step, marginal private MB per window (slope over baseline and open steps), fraction reclaimed
  after close, M3, M7, drawn frames per step.
- **Hypothesis:** each COMP window carries a DComp target, visual tree and surfaces; immediate windows carry a swap
  chain/bitmap per window (GDIP, D2D-SW: CPU backing store sized to the window). The marginal cost difference is the
  headline; minimize should release immediate-mode backing stores; reclamation should be similar.
- **Required features:** per-window render loop and device sharing, backing-store release on minimize/close, resize on
  restore.
- **Gate:** no expected unsupported content.

### S10 form (`form`, T10, T16, T26)

- **Purpose:** text input latency, focus visuals, hover states and light-dismiss popups.
- **Content:** three `TextBox`es, `ComboBox`, `CheckBox`, `ToggleSwitch`, `Slider`, a `Button` with a `Flyout`, a
  `Button` with a `ToolTip`, and an accent Submit button.
- **Interaction:** a fixed 21-step script at 10 Hz: typing (SendInput `KEYEVENTF_UNICODE` keystrokes, or
  `TextBox.SelectedText`), Tab focus moves (`VK_TAB` or `Focus(Keyboard)`), hover on/off (mouse moves or
  `VisualStateManager`), `ComboBox` open/close, `Flyout` open/close (click/Escape or API), `ToolTip` open/close.
  Steady and Recovery start parked: popups closed, programmatic focus (no caret).
- **Metrics:** R1 by kind (`key`, `tab`, `click`) in SendInput runs, popup open time, W1, R2, M1, M3.
- **Hypothesis:** keystroke-to-frame latency is dominated by the UI-thread render of the TextBox and caret; immediate
  modes should be comparable to COMP when damage is limited to the caret/line. Popups are separate windowed or
  island content: they test the render loop's handling of additional roots and their first-frame time.
- **Required features:** caret blink and focus visuals, popup/flyout roots (windowed popups), tooltip timing, visual
  state transitions.
- **Gate:** no expected unsupported content; unsupported popups, carets or focus visuals are failures.

### First extended-scene results (quick smoke, one run, GPU-less VM)

`Run-RenderBench.ps1 -Modes composition,gdiplus,d2d-sw -Scenes <S3-S10 entries> -InputMode programmatic
-Repetitions 1 -Quick` on the integration branch (local runtime staged). Single quick runs on a VM with WARP
composition: directional only, not a result. In-process private commit only; COMP's DWM-side and GPU memory are not
included, which favours COMP here (the S2 dashboard numbers include the comp surface pool in process).

| Scene | Steady private MB (COMP / GDI+ / D2D-SW) | Peak private MB | Interaction frame p95 ms | Interaction CPU ms/s |
|---|---|---|---|---|
| inbox | 53 / 60 / 59 | 74 / 63 / 66 | 4.7 / 13.7 / 5.3 | 310 / 517 / 455 |
| gallery | 73 / 98 / 105 | 106 / 140 / 126 | 16.9 / 126 / 19.2 | 615 / 1386 / 1367 |
| gallery-decoded | 77 / 95 / 103 | 108 / 132 / 123 | 16.7 / 135 / 16.5 | 565 / 1374 / 1422 |
| animated | 33 / 43 / 42 | 41 / 44 / 50 | 1.3 / 6.5 / 2.1 | 188 / 310 / 299 |
| animated-noring | 33 / 43 / 42 | 44 / 43 / 42 | 1.5 / 6.0 / 2.0 | 236 / 299 / 275 |
| reader | 45 / 39 / 47 | 56 / 41 / 49 | 10.3 / 13.0 / 4.8 | 686 / 554 / 448 |
| layered | 33 / 38 / 43 | 51 / 38 / 44 | 2.0 / 9.8 / 2.0 | 180 / 196 / 190 |
| datagrid | 147 / 117 / 125 | 202 / 143 / 151 | 38.5 / 25.9 / 22.0 | 402 / 375 / 399 |
| multiwindow (5) | 46 / 33 / 44 | 78 / 84 / 90 | 100 / 76 / 65 | 384 / 347 / 343 |
| multiwindow-tabs | 51 / 49 / 51 | 144 / 110 / 116 | 94 / 87 / 67 | 542 / 492 / 549 |
| form | 34 / 45 / 38 | 47 / 45 / 39 | 3.6 / 4.9 / 2.6 | 270 / 244 / 288 |

Gate: 25/33 pass. Expected failures: `animated` in every mode (scene is never idle; immediate modes draw ~1,850
frames/min vs ~87 for COMP because every animation tick is a UI-thread paint), `layered` and `form` in immediate modes
(acrylic fallback, ThemeShadow, approximated caret invert, windowed popup painted in the island).

Observations to drive the next iteration:

- Immediate modes win where element/visual count dominates (datagrid -15 to -20% steady, -25 to -29% peak; tabs peak
  -19 to -24%) and lose where the fixed per-root swap chain + painter target dominates small scenes (animated, layered,
  form: +4 to +11 MB). Next: drop the painter's own full-size target where possible (render straight into the back
  buffer with a damage-preserving swap chain) and measure DWM/GPU memory so COMP is charged for its out-of-process cost.
- Gallery is worse in immediate modes (+18 to +32 MB steady, 2.2-2.5x CPU, GDI+ p95 126 ms): decoded images are
  held twice (XAML software surface + painter bitmap cache) and GDI+ scales with `DrawImage` on the CPU. Next: share the decoded surface
  memory with the painter instead of copying, and cache scaled bitmaps for GDI+.
- GDI+ text and scrolling are slower (inbox p95 13.7 ms vs 4.7); D2D-SW is at parity or better.
- Animations cost a full UI-thread paint per tick in immediate modes; independent-animation offload is the open
  design question.

## Full-backend scenes

S11-S23 cover the content a full immediate backend must handle, following the decisions in the proposal
("Full-backend decisions"): composition interop, `SwapChainPanel`/`MediaPlayerElement` and arbitrary
`AnimatedVisualPlayer` content switch *that window* to Composition (per-window fallback, no hybrid). The native side
reports it with a `WindowFallback` event from the RenderingBackend provider (`Mode`, `RootId`, `Reason`); the analyzer
matches it by name and tolerates it being absent from older runtimes. Every scene is deterministic for `--seed`, uses
the usual phases (Warmup → Steady → Interaction → Recovery) and supports programmatic input. Window titles follow
`RenderBench <PlanId> <label> [<backend>]`. Run them with `Run-RenderBench.ps1 -Scenes full-backend` (or `s11`..`s23`).

**Out of scope:** S17 (WebView2) and S18 (InkCanvas). Both depend on content that renders outside XAML and are not
part of this prototype; the plan ids stay reserved and `Run-RenderBench.ps1` rejects them.

| Id | `--scene` (variants) | Expected in immediate modes |
|---|---|---|
| S11 | `interop` (`--no-interop`) | window falls back; the variant stays immediate |
| S12 | `lottie` (`--no-player`) | window falls back because of the player; ProgressRing/AnimatedIcon (the variant) stay immediate |
| S13 | `materials` | immediate; only Mica approximated |
| S14 | `popups` | immediate, including windowed popups |
| S15 | `independent` | immediate; frames during UI-thread blocks measured |
| S16 | `swapchain` (`--no-media`) | window falls back (both variants) |
| S19 | `text` | immediate |
| S20 | `dpitheme` | immediate; recovery and leak gates |
| S21 | `devicelost` | immediate; recovery and leak gates |
| S22 | `accessibility` | immediate; RTB and capture gates |
| S23 | `appshell` | immediate |

### S11 composition interop (`interop`, variant `--no-interop`)

- **Content:** a `SpriteVisual` attached with `ElementCompositionPreview.SetElementChildVisual` and rotated by an
  expression animation; cards whose handoff visuals (`GetElementVisual`) have an implicit `Offset` animation; a
  `ConnectedAnimation` between a thumbnail view and a detail view.
- **Interaction:** 1 Hz view swaps (connected animation) and card reorders.
- **Expectation:** immediate modes emit `WindowFallback`, the window renders through Composition, frames are still
  presented and nothing crashes. `S11-nointerop` has the same layout and state changes without interop and must stay
  immediate (a `WindowFallback` there fails the gate).
- **Gate:** `WindowFallbackAsExpected`, `FramesObserved`, `HarnessCompleted`.

### S12 Lottie (`lottie`, variant `--no-player`)

- **Content:** built-in `ProgressRing`s, `AnimatedIcon`s in buttons and a CheckBox, and an `AnimatedVisualPlayer`
  playing a hand-coded `IAnimatedVisualSource` (a `ShapeVisual` rotated by an expression on the player's `Progress`).
- **Expectation:** the player makes the window fall back; with `--no-player` (a static square in its place) the window
  stays immediate and ProgressRing/AnimatedIcon render without unsupported content.
- **Gate:** `WindowFallbackAsExpected`, `NoUnsupportedContent` (content in a window that fell back as expected is
  allowed).

### S13 materials (`materials`)

- **Content:** `MicaBackdrop` window backdrop, in-app `AcrylicBrush` panes over colorful shapes, `ThemeShadow` cards and
  a flyout-like panel (`Translation` Z), DropShadow-like elevation from stacked translucent borders, `LinearGradientBrush`
  fills (also as text foreground).
- **Interaction:** elevation and acrylic toggles (1 Hz) and gradient-stop moves (2 Hz).
- **Expectation:** no `UnsupportedContent` for gradients, ThemeShadow or acrylic. Mica may be approximated by its
  fallback color and reported with a detail ending in `:approximated`, which is allowed.
- **Gate:** `NoUnsupportedContent`, `WindowFallbackAsExpected` (no fallback).

### S14 windowed popups (`popups`)

- **Content:** a 560×400 window with controls in its bottom-right corner, so every popup crosses the window edge:
  `Flyout`, `MenuFlyout`, `ComboBox` drop-down, `TeachingTip`, `ToolTip` (`ShouldConstrainToRootBounds=false` where the
  API has it) and plain windowed `Popup`s.
- **Interaction:** steps `baseline`, `popups-1`..`popups-6` (cumulatively open plain popups), `closed`; then open/close
  cycles over every popup kind at 1 Hz.
- **Metrics:** popup open-to-first-frame by kind (`PopupOpenMsByKind`), private MB per open popup
  (`PrivateMBPerOpenPopup`, slope over the `popups-*` steps).
- **Gate:** `NoUnsupportedContent` (a popup painted into the island instead of its own window is a failure).

### S15 independent animation and scroll (`independent`)

- **Content:** a 2000-item list scrolled with animated `ScrollViewer.ChangeView`, a looping opacity/translate
  `Storyboard` and a `ProgressRing`.
- **Interaction:** every 4 s the UI thread is blocked for 1000 ms (`Thread.Sleep`) between harness
  `UiBlockBegin`/`UiBlockEnd` events.
- **Metrics:** per block (`UiBlocks`), frames drawn during the block and the longest drawn-frame gap around it
  (`FramesDuringUiBlock`, `MaxFrameGapDuringUiBlockMs`). A background screen probe of the animated lane adds
  `BlockScreenChanges` and `BlockMaxScreenGapMs` (valid only when `BlockScreenProbeValid` = 1, i.e. a visible desktop),
  which also covers Composition, whose DWM-side frames have no WinUI frame events.
- **Expectation:** Composition keeps presenting independent animations during the block; immediate modes are measured
  (off-thread ticking is in progress). Target: no dropped frames.
- **Gate:** `UiBlockObserved`.

### S16 SwapChainPanel and MediaPlayerElement (`swapchain`, variant `--no-media`)

- **Content:** a `SwapChainPanel` with a D3D11 composition swap chain created through raw COM (no new packages),
  cleared to an animated color on every `CompositionTarget.Rendering` (next to XAML status text updated at 2 Hz, so the
  window also draws XAML frames), and a `MediaPlayerElement` looping a clip
  generated once with `MediaComposition` (`%LOCALAPPDATA%\RenderBench\media\colors.mp4`). `SwapChainAvailable` and
  `MediaAvailable` scene values report whether each part ran; a failure skips that part only.
- **Expectation:** `WindowFallback` in immediate modes for both variants.
- **Gate:** `WindowFallbackAsExpected`, `FramesObserved`.

### S19 text stress (`text`)

- **Content:** RTL Arabic and Hebrew paragraphs, Devanagari and Thai, color emoji (Segoe UI Emoji), a CFF-outline
  OpenType font if one is installed (`CffFontPresent` = 0 skips that variant), Segoe UI Variable weights, ClearType
  (opaque background) vs grayscale (transparent or translucent background) text, a `RichTextBlock` with mixed runs,
  72 pt text and 1000 small `TextBlock`s in an `ItemsRepeater`.
- **Interaction:** scroll at 300 DIP/s.
- **Gate:** `NoUnsupportedContent`; fidelity compared with Comp(Lifted) screenshots.

### S20 DPI, theme and resize (`dpitheme`)

- **Content:** the S1 settings page.
- **Interaction:** 100 cycles (`--items`): an `AppWindow.Resize` from a deterministic size list each cycle, a
  `RequestedTheme` toggle every 5th (20 toggles), and a move to a monitor with a different DPI every 25th when one exists
  (`DpiChangeAvailable`; otherwise DPI change is a manual test). All cycles run regardless of the phase length. A full GC
  precedes steps `cycle-10` and `cycle-100`.
- **Metrics:** change-to-frame by kind (`ChangeToFrameMsByKind`: `resize`, `theme`, `dpi`), `UnmatchedChanges` (no frame
  within 5 s), `CycleLeakPrivateMB` (cycle-100 minus cycle-10).
- **Gate:** `RecoveryWithinLimit` (p95 per kind ≤ 1000 ms, no unmatched changes), `NoCycleLeak` (< 2 MB; n/a for
  runs shorter than 100 cycles, such as `-Quick`).

### S21 device lost (`devicelost`, `--device-lost <frames|every:N>`)

- **Content:** the S2 dashboard updated at 30 Hz.
- **Hook:** for immediate modes RenderBench sets `WINUI_IMMEDIATE_SIMULATE_DEVICE_LOST` (default `every:10`, or a frame
  number list) before `Application.Start`; the native painter simulates a device loss on those frames. Composition has
  no equivalent hook and runs the scene as a baseline (gates n/a).
- **Metrics:** `DeviceLostRecoveryMs` (DeviceLost to the next drawn frame), `DeviceLostLeakPrivateMB` (after the 100th
  loss minus after the 10th).
- **Gate:** immediate modes: `DeviceLostObserved`, `RecoveryWithinLimit` (≤ 1000 ms), `NoCycleLeak` (< 2 MB; n/a with
  fewer than 100 losses).

### S22 accessibility and visual capture (`accessibility`)

- **Content:** 50 focusable controls.
- **Interaction:** keyboard focus moves across them at 4 Hz (Tab or `Focus(Keyboard)`), showing focus rectangles. Every
  10th move, `RenderTargetBitmap.RenderAsync` renders a static subtree and is compared with the same rectangle on
  screen: the `WINUI_IMMEDIATE_CAPTURE` BMP written by the immediate painter, or a screen/PrintWindow capture for
  Composition.
- **Metrics:** `RtbSuccessRate`, `CaptureMeanAbsDiff` (mean absolute BGRA difference, 0..255).
- **Gate:** `RenderTargetBitmapOk` (every render succeeded), `CaptureWithinTolerance` (≤ 8; skipped without values).
- **Manual:** high contrast (switch the OS theme and check focus visuals and system colors); not automated.

### S23 real app shell (`appshell`)

- **Content:** a `NavigationView` shell with 10 pages: settings-like pages (cards, toggles, expanders), an
  Explorer-like details `ListView` (5 columns × 2000 rows) and a Gallery-like grid of control samples. Each page is
  built on navigation.
- **Interaction:** navigate to the next page at 0.5 Hz.
- **Metrics:** change-to-frame for `navigate`, M1-M3, R2.
- **Gate:** `NoUnsupportedContent`, `WindowFallbackAsExpected` (no fallback).

### Full-backend exit criteria

An immediate backend is complete for S11-S23 when, on a gated run set:

1. **Content:** no unexpected `UnsupportedContent`; only details ending in `:approximated` (e.g. Mica) are allowed.
   S11, S12 with the player and S16 fall back per window as designed; nothing else falls back.
2. **Fidelity:** pixel parity with Comp(Lifted) screenshots within the C1 tolerance; S22 capture mean abs diff ≤ 8.
3. **Responsiveness:** input-to-frame p95 ≤ 1.2× Composition for the same scene and input mode.
4. **Independent animation:** S15 has no dropped frames during UI-thread blocks (target; Composition is the reference).
5. **Memory:** steady and peak private bytes ≤ Comp(System) for the same scene.
6. **Recovery:** S20 and S21 recover within 1 s (p95) and leak < 2 MB over 100 cycles.
## Required WinUI instrumentation

The comparison depends on counters that do not exist today. Add them as TraceLogging/ETW events in the WinUI
rendering code, emitted in all four modes so results are directly comparable:

- **Backend identity**: requested mode, effective mode, adapter LUID and type, fallback reason.
- **Frame regions**: start and stop for layout, render walk or immediate traversal, command generation,
  rasterization, commit or present.
- **Damage**: invalidated pixels, repainted pixels, rectangle count per frame.
- **Resources**: Composition visual count, retained layer count and promotion reason, surface count and bytes,
  cache bytes, temporary surface bytes, uploads and copies in bytes, resources created and destroyed.
- **Harness markers**: phase changes and injected-input identifiers so input and visible frames can be correlated.

Counter overhead must be measured (run with counters off versus on in `COMP`) and kept negligible, or sampled
counters must be used.

Status (GDI+ branch): resources created and destroyed per frame are emitted as `ResourceChurn` (Resources keyword) and
reported as W6 per second. GDI+ uploads are defined as the bytes copied from the CPU bitmap to the presentable surface:
damaged pixels times 4, with no full-frame copy except the first frame after a swap chain is attached or resized.
Offscreen frames report 0. The `Memory` WPR profile adds heap tracing so GDI+ heap growth (M2, M8)
can be attributed.

**GDI+ readiness (prototype).** The GDI+ painter is implemented:

- It draws into a 32bpp PARGB `Gdiplus::Bitmap` over a painter-owned buffer, clipped to damage.
- Text is drawn with GDI+ `DrawString` (grayscale antialiasing). Fonts, brushes, pens and opacity layers are cached,
  so steady-state frames have no resource churn.
- Presentation uses one composition swap chain per root on any D3D11 device. Each damage rect is copied with
  `UpdateSubresource`, then `Present1` is called with those rects as dirty rects.
- `gdiplus.dll` is delay-loaded by `Microsoft.ui.xaml.dll`, so Composition and Direct2D processes do not load it.

Prototype status for `D2D-HW` (see `perf\ImmediateRendering\README.md`):

- W3 uses `GpuPaintTime` (frame number, GPU microseconds) from `GpuTimestampProfiler`, which wraps each paint in
  non-blocking D3D11 timestamp queries.
- M3 is also reported for the render adapter, matched by the `DeviceInfo` LUID.
- Without a qualifying hardware GPU, `D2D-HW` runs are recorded as skipped, not failed.
- D2D-HW numbers must come from a real GPU machine.

**D2D-SW readiness (prototype).** The following are in place for D2D-SW:

- **Per-thread CPU split for W1/W2.** The harness emits `ThreadCpuSample`. It classifies threads by start module into
  UI, WARP rasterizer, D2D/DWrite, composition, and other. `ntdll` thread-pool threads count as other, so attribute
  those with Timing-profile stacks in WPA.
- **`DeviceLost` event** on the native provider.
- **Documented W5 copy contract.** For frames presented through a swap chain, upload bytes are damaged pixels × 4.
  Offscreen frames report 0.
- **WARP determinism test.** WARP output is pixel-identical across devices, so D2D-SW renders can serve as the C1
  golden reference. The reference is pinned to the recorded `d3d10warp`/`d2d1`/`dwrite` versions.
- **D2D painter.** `D2DPainter.cpp` serves both D2D modes; the D3D device decides hardware versus WARP. It draws into a
  persistent premultiplied BGRA target, clips to the damage, and presents through the root's composition swap chain
  by copying only the damage rects and calling `Present1` with matching dirty rects. Steady-state frames create no
  brushes, text formats, geometries, layers, or targets (`ResourceChurn` stays silent). Device removal fails the frame,
  emits `DeviceLost` once, and leaves the painter failed until the caller recreates it. On a hardware device each frame
  is bracketed by `GpuTimestampProfiler` (W3, `GpuPaintTime`); on WARP it is not, because the raster cost is CPU time.
  W5 for D2D-HW reports 0 upload bytes for the prototype primitives; the GPU-local back-buffer copy is exposed
  separately. The hardware path shares all code with WARP but has only been validated without a GPU, so `D2D-HW`
  conformance runs are still required on a GPU machine.

See `perf\ImmediateRendering\README.md`.

## Comparative report format

For each test case, produce one table with the four modes as columns and the selected metrics as rows. Show
absolute medians and spread, plus a ratio to `COMP`. Mark any mode that failed the correctness gate or fell back.

Example layout (illustrative only, not results):

| T7 static settings | COMP | GDIP | D2D-HW | D2D-SW |
|---|---|---|---|---|
| M1 private commit (MB) | value (range) | value (range), ratio | ... | ... |
| M3 GPU local (MB) | ... | ... | ... | ... |
| R1 p95 latency (ms) | ... | ... | ... | ... |
| E2 energy (J) | ... | ... | ... | ... |
| Correctness gate | pass | pass or fail | ... | ... |

Then summarize across tests:

- A **memory map**: T1 and T2 slopes per mode, showing where each backend's memory crosses `COMP`.
- A **cost map**: CPU, GPU, and energy versus damage size (T3), showing where immediate repainting gets expensive.
- A **suitability matrix**: for each workload (T7 to T14), which mode meets the proposal's success gates, and why.

## Measurement pitfalls

- Fewer visuals is not lower memory. Always report M1, M3, and M4 together.
- Working set can drop simply because memory was trimmed. Use private commit for demand and working set for
  residency.
- `D2D-SW` still uses the GPU for composition. Its GPU numbers are not zero and should not be expected to be.
- `GDIP` and `D2D-SW` may move cost from GPU memory to CPU memory and bandwidth. Read M1 and M3 side by side.
- High DPI changes surface size quadratically. Never compare runs at different DPI.
- Average FPS hides stalls and idle windows should not render. Use R1 to R3 instead.
- A full-redraw prototype does not represent the final damage-driven backend. Label such results.
- DWM memory is shared across the desktop. M4 is approximate and requires a matched baseline in the same session.

### Full-backend results S1-S23 (quick, one run, GPU-less VM)

`Run-RenderBench.ps1 -Modes composition,gdiplus,d2d-sw -Scenes <all 27 entries> -InputMode programmatic -Repetitions 1
-Quick` on the integration branch, plus `-Modes composition -CompositionEngine System -OptionalChanges All` for Comp
(System). In-process private commit only; the Composition columns don't include DWM-side or GPU memory. Run-to-run
noise is about ±10%. All 108 runs pass their gates. S23 passes after e143f5aef: NavigationView mirrors
`UIElement.Opacity` on its selection indicator's hand-off visual, and before that fix this switched the window to
Composition.

| Id | Entry | Steady MB (Comp Lifted / Comp System / GDI+ / D2D-SW) | Peak MB (same order) | Immediate path |
|---|---|---|---|---|
| S1 | `settings` | 46 / 41 / 46 / 46 | 54 / 43 / 50 / 54 | immediate |
| S2 | `dashboard` | 118 / 109 / 42 / 54 | 171 / 113 / 48 / 59 | immediate |
| S3 | `inbox` | 53 / 49 / 55 / 72 | 63 / 54 / 57 / 87 | immediate |
| S4 | `reader` | 46 / 43 / 39 / 51 | 60 / 48 / 46 / 57 | immediate |
| S5 | `gallery` | 77 / 71 / 96 / 102 | 110 / 76 / 134 / 129 | immediate |
| S5 | `gallery-decoded` | 76 / 72 / 96 / 103 | 80 / 75 / 131 / 124 | immediate |
| S6 | `animated` | 33 / 29 / 43 / 44 | 44 / 30 / 46 / 47 | immediate |
| S6 | `animated-noring` | 32 / 29 / 43 / 42 | 44 / 33 / 46 / 47 | immediate |
| S7 | `layered` | 32 / 29 / 39 / 52 | 51 / 33 / 42 / 57 | immediate |
| S8 | `datagrid` | 144 / 134 / 114 / 130 | 201 / 177 / 137 / 158 | immediate |
| S9 | `multiwindow` | 46 / 41 / 33 / 46 | 77 / 67 / 84 / 94 | immediate |
| S9 | `multiwindow-tabs` | 53 / 46 / 46 / 54 | 146 / 120 / 107 / 122 | immediate |
| S10 | `form` | 35 / 31 / 37 / 50 | 47 / 32 / 59 / 61 | immediate |
| S11 | `interop` | 30 / 27 / 31 / 42 | 43 / 27 / 47 / 58 | Comp fallback |
| S11 | `interop-nointerop` | 30 / 27 / 33 / 38 | 43 / 27 / 36 / 42 | immediate |
| S12 | `lottie` | 32 / 28 / 31 / 35 | 48 / 31 / 48 / 47 | Comp fallback |
| S12 | `lottie-noplayer` | 32 / 28 / 34 / 36 | 48 / 28 / 40 / 40 | immediate |
| S13 | `materials` | 33 / 30 / 35 / 53 | 52 / 30 / 38 / 85 | immediate |
| S14 | `popups` | 35 / 31 / 36 / 50 | 43 / 32 / 39 / 64 | immediate |
| S15 | `independent` | 147 / 135 / 92 / 94 | 159 / 143 / 97 / 113 | immediate |
| S16 | `swapchain` | 126 / 123 / 127 / 131 | 136 / 124 / 139 / 145 | Comp fallback |
| S16 | `swapchain-nomedia` | 25 / 22 / 26 / 29 | 32 / 25 / 38 / 41 | Comp fallback |
| S19 | `text` | 57 / 53 / 45 / 46 | 77 / 72 / 68 / 81 | immediate |
| S20 | `dpitheme` | 47 / 41 / 45 / 47 | 60 / 42 / 47 / 67 | immediate |
| S21 | `devicelost` | 115 / 117 / 42 / 53 | 195 / 117 / 46 / 59 | immediate |
| S22 | `accessibility` | 67 / 42 / 58 / 65 | 81 / 42 / 73 / 81 | immediate |
| S23 | `appshell` | 95 / 80 / 84 / 82 | 107 / 90 / 86 / 96 | immediate |

Geometric mean vs Comp (Lifted): steady Comp (System) 89%, GDI+ 92%, D2D-SW 107%; peak Comp (System) 73%, GDI+ 84%,
D2D-SW 100%. Immediate wins where Composition keeps many visuals or surfaces alive (S2, S8, S15, S19, S21, S23).
It loses on decoded images (S5, where the painter holds its own copy) and on small static scenes (fixed back-buffer
and painter cost); D2D-SW adds 10-20 MB of WARP and cache overhead. S15: about 93 frames are presented across three
1 s UI-thread blocks on both immediate backends. S21: 14 simulated losses, recovery p95 9-11 ms. S22: RTB 100%, mean
abs diff ≤ 0.002. Leak gates (S20, S21) need non-quick runs.

Open gaps: animated scroll (`ChangeView`) does not tick off the UI thread; real device-removed paths are untested
(simulated only); RichEditBox text is not painted; gradient strokes use the average color; Mica is drawn as its
fallback color; D2D-HW is not measured (no GPU).

### D2D-SW back-buffer fix (commit 308ba5a15)

On a GPU-less VM Comp and D2D-SW both rasterize on WARP, so D2D-SW should cost about what Comp costs. Before the
fix it did not, because the painter drew into its own full-window texture and copied the damage into the swap chain
back buffer on every present. Now the D2D target wraps `GetBuffer(0)` while a swap chain is attached. Before drawing,
scrolling or reading pixels, the painter copies the previous frame's dirty rects forward from the front buffer
(`GetBuffer(BufferCount - 1)`), because flip model only fills the new back buffer at the next Present. On detach the
last frame is copied into a fresh offscreen target. Test: `AttachedSwapChainBackBufferIsTheTarget`; 230/230
RenderingBackendTests pass.

Remeasured Comp (Lifted) and D2D-SW on all 27 entries (54 runs, all gates pass). The rerun ran over RDP at 32 Hz,
where the first matrix ran on the console at 60 Hz, and that raised Comp too (S2 118 → 155 MB). So the comparison is
D2D-SW against Comp from the same session:

| Steady D2D-SW vs same-session Comp | Before | After |
|---|---|---|
| Geometric mean, 27 entries | 107% | 101% |
| S5 `gallery` | +32% | +11% |
| S7 `layered` | +61% | +28% |
| S10 `form` | +45% | +23% |
| S14 `popups` | +42% | +8% |
| S13 `materials` | +61% | +40% |

Live VirtualQuery check (committed private, idle): S10 D2D-SW 56.4 → 49.8 MB against Comp 36.8 MB; S1 D2D-SW
37.3 MB against Comp 39.7 MB. The remaining S10 gap is in the process heap (35 vs 26 MB commit), from WARP, D2D and
DWrite caches and per-popup painters.

Sharing XAML's D3D device is deliberately not done. The second WARP device costs about 0.4 MB (1.7 MB with its D2D
context), while sharing it would need the shared-device lock on both the UI thread and the independent-animation render
thread, plus a new device-loss path.

## XAML coverage (full backends)

Goal: GDI+ and D2D (SW and HW) render every XAML feature that doesn't need Composition interop, with no per-window
fallback. Interop content (SwapChainPanel, MediaPlayerElement, app Composition visuals and animations, app
AnimatedVisualPlayer sources) keeps the per-window fallback decided for S11/S16; there is no hybrid. WebView2 and
InkCanvas stay out of scope.

The corpus is a set of small coverage pages in RenderBench (`RenderBench\Coverage\Coverage.<Family>.cs`, one file per
family, contract in `CoverageContract.cs`). Each page is rendered in Comp (Lifted), GDI+ and D2D-SW. A page passes in an
immediate mode when all of these hold:

1. **No unexpected unsupported content:** the only `UnsupportedContent` details are ones the page declares (documented
   approximations).
2. **Fallback as declared:** no `WindowFallback` unless the page declares `ExpectFallback`.
3. **Fidelity:** mean absolute per-channel difference against the Comp rendering of the same page is at most the
   page's threshold (default 0.01).
4. **Stability:** no crash, no failed paint, and the page's exercise step completes.

| Family | Content | Owner branch |
|---|---|---|
| F1 Shapes and strokes | Path, Line, Polyline, Polygon; dash arrays, dash caps, line caps, joins, miter limit; Stretch | shapes |
| F2 Brushes | Linear and radial gradients on fills, strokes and text; spread and interpolation modes; ImageBrush stretch and alignment; brush transforms; Glyphs element | shapes |
| F3 Clips and transforms | Geometry clips (ellipse, path, rotated rectangle); clips under rotation and skew; UIElement Rotation, Scale, TransformMatrix, CenterPoint; static hand-off visual properties | clips3d |
| F4 3D | PlaneProjection, Matrix3DProjection, PerspectiveTransform3D, CompositeTransform3D | clips3d |
| F5 Text | TextBlock and RichTextBlock features (highlighters, decorations, typography, character spacing, line stacking, trimming, IsTextScaleFactorEnabled), Glyphs | text |
| F6 Text editing | TextBox (selection, IME composition, scrolling, placeholder, multi-line), PasswordBox, RichEditBox, AutoSuggestBox, NumberBox; exact caret invert | text |
| F7 Images and icons | Bitmap stretch modes, nine-grid, SVG, animated GIF, BitmapIcon (monochrome), FontIcon, PathIcon, ImageIcon, LoadedImageSurface | imaging |
| F8 Controls | Every MUX and MUXC control in default, hover-like and disabled states, light and dark | coverage |
| F9 Scrolling, zoom, input | Animated ChangeView, DManip panning and zoom, ZoomFactor, ScrollView/ScrollPresenter, ItemsView, SwipeControl, RefreshContainer, ParallaxView; dmanip branch adds ScrollViewer-KeyboardPageDown, ScrollViewer-BringIntoView-Animated, ScrollView-MouseWheel and ScrollView-TouchPan (19 pages) | scrolling, dmanip |
| F10 XAML animations | Storyboards, theme transitions, Scalar/Vector3/Brush transitions, ConnectedAnimation, NavigationView indicator, TeachingTip, ItemsRepeater animator | transitions |
| F11 Interop | Every interop kind falls back cleanly and then matches Comp | imaging |
| F12 Popups and dialogs | ContentDialog, Flyout, MenuFlyout, CommandBarFlyout, ToolTip, TeachingTip, ComboBox, CalendarDatePicker; windowed and not | coverage |
| F13 Windowing and hosting | Multiple windows, AppWindow title bar, SystemBackdrop (Mica, DesktopAcrylic), DesktopWindowXamlSource in a Win32 host, printing audit | imaging |
| F14 Theming and accessibility | High contrast, RTL, DPI changes, theme changes, legacy dotted focus visuals, RequestedTheme per subtree | coverage |

**Success measures**

| Measure | Target |
|---|---|
| Non-interop coverage pages that pass in both GDI+ and D2D-SW | ≥ 95%, every failure listed with an owner |
| Interop pages (F11) that fall back and then match Comp | 100% |
| Documented `UnsupportedContent` details outside F11 | Only `:approximated` entries listed in the proposal doc |
| Perf matrix (27 entries) | All gates pass; steady geometric mean vs Comp (Lifted) no more than 3 points worse than GDI+ 92% and D2D-SW 101% |
| Native and kit tests | RenderingBackendTests and RenderBench.Tests pass |

### XAML coverage: harness and results (coverage branch)

**Usage.** Run from `perf\ImmediateRendering` after staging a build into the RenderBench output (`validate-branch.ps1
-Mux -Muxc -Stage -Kit`):

```powershell
.\scripts\Run-Coverage.ps1 -Results <dir>                                    # whole corpus, comp + gdiplus + d2d-sw
.\scripts\Run-Coverage.ps1 -Pages 'F8*,!F8-controls/itemsview-*' -Modes composition,gdiplus
.\scripts\Run-Coverage.ps1 -Results <dir> -AnalyzeOnly                       # re-gate an existing tree
```

`-Pages` takes comma-separated globs over the page id (`F8-controls/buttons-light`), the family (`F8-controls`) or the
family number (`F8`), and `!glob` excludes. Other parameters are `-TimeoutSeconds` (default 60 per run) and
`-DeleteBmps`, which keeps BMPs only for failing runs (PNGs are always written). The whole corpus is 237 runs and takes
about 12 minutes.

**Pages.** A page is a static method `[CoveragePage(family, name, ExpectedUnsupported = ..., ExpectFallback = ...,
MaxMeanAbsDiff = 0.01)]` that returns a `CoveragePage` (content, optional `Exercise` step). The contract is in
`RenderBench\Coverage\CoverageContract.cs`. `RenderBench --list-coverage` discovers pages by reflection, and every
branch adds its own `Coverage.<Family>.cs`. Helpers for themed hosts and light/dark/disabled variants are in
`CoveragePageKit.cs`.

**Pipeline.**

1. Each (page, mode) runs in its own RenderBench process, so unsupported or fallback state never leaks between pages.
   - `WINUI_RENDERING_BACKEND` selects the mode and is unset for Composition.
   - `WINUI_IMMEDIATE_STATS` and `WINUI_IMMEDIATE_CAPTURE` point into the run folder.
   - The process holds the machine-wide bench lock (`Global\mux-ir-bench`) only while it runs.
2. The window has a fixed 1000x700 client area at the machine DPI. The harness waits for `Loaded`, awaits `Exercise`
   (for example, opening a flyout or switching the theme), then waits for two identical consecutive captures.
3. The process writes `capture.bmp`, `app.json` (exit status, private MB at idle) and `stats.txt`. The `# unsupported`,
   `# fallback` and `# paint-failed` lines in `stats.txt` replace WPR, which is too slow for 237 runs.
4. `RenderBench.Analyzer coverage <dir>` diffs every immediate capture against the Composition capture of the same page
   and applies the gates. It writes `result.json` per run plus `coverage.json` and `coverage.html` at the root. The HTML
   shows a page × mode matrix with pass/fail colors, Comp/immediate/diff thumbnails, family pass rates and the overall
   percentage against the 95% target.

**Capture method.** The harness uses `PrintWindow(PW_RENDERFULLCONTENT)` of the main window and of every visible popup
window it owns, composited at their screen offsets. Pure-black margins of windowed popups count as transparent.

| Option | Result |
|---|---|
| `RenderTargetBitmap` of the content root | Works headless but excludes popups (F12) |
| `Windows.Graphics.Capture` | Fails in a disconnected or RDP session |
| `BitBlt` of the screen DC | Black in a disconnected session |
| DWM thumbnail | Not tried: needs a composed, visible destination window, so it has the same session constraint |
| `PrintWindow(PW_RENDERFULLCONTENT)` | Works headless for Composition and for immediate HWND content; includes windowed popups when they are captured separately. **Chosen.** |

The same capture is used in every mode. In immediate modes `WINUI_IMMEDIATE_CAPTURE` is written alongside it as a
cross-check of what the backend presented.

**Gates** (`RenderBench.Core\Coverage\CoverageGates.cs`). These are the four gates listed above.

- Unsupported: allows the page's declared details plus the globally expected `AnimatedIcon:approximated`.
- Fallback: must match `ExpectFallback`.
- Fidelity: the mean absolute per-channel difference (0..1) must be at most `MaxMeanAbsDiff`.
- Stability: covers exit code, timeout and `# paint-failed`.

Composition is the reference and only gets the stability gate. Because the metric is a mean over 1000x700 pixels, a
missing focus rect moves it by only about 6e-4. The focus pages therefore use tight thresholds (0.0006–0.0015).

**High contrast** is a system-wide setting (`SPI_SETHIGHCONTRAST`) and can't be toggled per process. Toggling it would
disturb other agents and sessions on the machine, so F14 has no high-contrast page. It needs a dedicated VM run.

**Corpus on this branch.** 79 pages:

- F8 (54): buttons, toggles, range, status, bars, pickers, calendar + ColorPicker, collections, ItemsRepeater, ItemsView,
  FlipView/Pivot, TabView, NavigationView left/top/minimal, panels, panes, content, idle text and RichEditBox. Most come
  in light, dark and disabled variants, plus an indeterminate progress page.
- F12 (15): ContentDialog, Flyout, MenuFlyout with a submenu, CommandBarFlyout, TeachingTip and Popup, each windowed
  (`ShouldConstrainToRootBounds=false`) and non-windowed where that applies. Also ToolTip, an open ComboBox, an open
  CalendarDatePicker and AutoSuggestBox suggestions.
- F14 (10): RTL, RequestedTheme per subtree, runtime theme switch, Viewbox scale, RasterizationScale, and focus visuals:
  high-visibility light and dark, legacy `DottedLine`, Reveal, and custom brushes.

**Results** (`Run-Coverage.ps1 -Results cov-run4 -DeleteBmps`, all 3 modes):

| Mode | F8 | F12 | F14 | Total |
|---|---|---|---|---|
| Composition | 54/54 | 15/15 | 10/10 | 79/79 |
| GDI+ | 50/54 | 14/15 | 10/10 | 74/79 (93.7%) |
| D2D-SW | 50/54 | 14/15 | 10/10 | 74/79 (93.7%) |

Passing pages have a mean diff of 0.0010 against Composition, with a maximum of 0.0041 (ColorPicker). The 95% target is
not met only because of the failures below. Each has an owner, and all of them fail identically in GDI+ and D2D-SW.

| Page | Gate failed | Root cause | Owner |
|---|---|---|---|
| itemsview-light, itemsview-dark | Fallback | ItemsView reports `Panel Hand-off Composition visual` | scrolling (F9) |
| richeditbox-idle-light, richeditbox-idle-dark | Unsupported | `TextBoxView RichEditBox text` is not painted | text (F6) |
| autosuggestbox-suggestions | Unsupported | Focused TextBox caret reports `Rectangle CompositeMode DestInvert:approximated` (fidelity passes, 0.0012) | text (F6) |

**Fixes on this branch** (generic walk/render behavior and control chrome):

- Walk the element's local transition root (it holds the focus visual and secret transition targets). Removed the
  `IsActive` filter on secret layout-transition targets. Apply the LTE's local transform, so focus rects with a negative
  `FocusVisualMargin` line up.
- Legacy `FocusVisualKind.DottedLine` focus rects: `CFocusRectManager::GetLegacyFocusRectOptions` plus
  `EmitLegacyFocusRect`, which is pixel-identical to Composition.
- Reveal focus glow is reported as `Reveal focus glow:approximated` instead of triggering a hand-in fallback.
- CalendarView density bars and theme-aware surfaces for dark pages.
- Windowed popups: skip black when blending into the parent. In-app acrylic with nothing painted beneath it uses the
  brush's opaque fallback color, matching MUXC AcrylicBrush (AutoSuggestBox and ComboBox lists).
- MUXC CommandBarFlyout: when immediate rendering is requested, use the acrylic fallback instead of the
  SystemBackdrop placement visual.
- MUXC ColorSpectrum: SpectrumBrush (a `XamlCompositionBrushBase` over LoadedImageSurfaces with a transparent fallback)
  painted nothing in immediate modes. When immediate rendering is requested, the spectrum now cross-fades the same pixel
  data on the CPU into a WriteableBitmap ImageBrush.

**Memory** (private MB at idle across all 79 pages, mean / max): Composition 43.4 / 54.1, GDI+ 48.6 / 60.7,
D2D-SW 52.6 / 67.9.

### Full XAML coverage: integrated results

The integration branch `user/neeverma/immediate-rendering-integration` merges the seven coverage branches in this order:
coverage, shapes, clips3d, text, scrolling, transitions, imaging. All numbers below come from one session on the GPU-less
VM. The desktop was a 1024×768 console session; earlier matrices ran in a larger RDP desktop.

**Coverage.** `Run-Coverage.ps1 -Results C:\mux-ir\results\coverage-integrated -DeleteBmps` ran the whole corpus: 195
pages in F1-F14, all three modes, 585 runs, about 22 minutes, under the bench lock.

| Family | Composition | GDI+ | D2D-SW |
|---|---|---|---|
| F1-shapes | 5/5 | 5/5 | 5/5 |
| F2-brushes | 5/5 | 5/5 | 5/5 |
| F3-clips | 8/8 | 8/8 | 8/8 |
| F4-3d | 10/10 | 10/10 | 10/10 |
| F5-text | 9/9 | 9/9 | 9/9 |
| F6-editing | 8/8 | 8/8 | 8/8 |
| F7-images | 9/9 | 9/9 | 9/9 |
| F8-controls | 54/54 | 54/54 | 54/54 |
| F9-scrolling | 15/15 | 15/15 | 15/15 |
| F10-animations | 32/32 | 32/32 | 32/32 |
| F11-interop | 10/10 | 10/10 | 10/10 |
| F12-popups | 15/15 | 15/15 | 15/15 |
| F13-windowing | 5/5 | 5/5 | 5/5 |
| F14-theming | 10/10 | 10/10 | 10/10 |
| **Total** | **195/195** | **195/195 (100%)** | **195/195 (100%)** |

Both success measures are met:

- **Non-interop pages:** 184/184 (100%) pass in every immediate mode, against a 95% target.
- **Interop pages:** 11/11 fall back and match Comp, against a 100% target. These are the 10 F11 pages plus
  `F3-clips/handoff-path-clip`.

Fidelity, as the mean absolute difference against Comp:

| Mode | Mean | Max |
|---|---|---|
| GDI+ | 0.0010 | 0.047 |
| D2D-SW | 0.0009 | 0.047 |

The maximum is on the materials and backdrop pages, which have higher declared thresholds.

Private MB at idle, mean / max:

| Mode | Mean | Max |
|---|---|---|
| Comp | 43.6 | 93.5 |
| GDI+ | 48.7 | 62.5 |
| D2D-SW | 51.2 | 69.5 |

The only approximated content reported in immediate modes:

| Content | Runs |
|---|---|
| `AnimatedIcon:approximated` | 76 |
| `ConnectedAnimation cross-fade:approximated` | 8 |
| `SystemBackdrop:approximated` (Mica, DesktopAcrylic) | 4 |
| `Reveal focus glow:approximated` | 2 |

**Integration fixes.**

- `HasPaintUnder` lost its closing brace in the text merge.
- `F11-interop/hand-off-visual` now starts a 50 ms app `ScalarKeyFrameAnimation` on `RotationAngleInDegrees`.
  - With clips3d, static hand-off properties are painted without a fallback. The imaging page's static
    CenterPoint/RotationAngleInDegrees no longer triggered the expected fallback.
  - The page now covers the animated hand-off case. Static hand-off is covered by `F4-3d/handoff-*`.

No other cross-branch regressions were found. The coverage branch's failures pass after the merge:

- itemsview: fixed by scrolling, clips3d and transitions.
- richeditbox-idle: fixed by text.
- autosuggestbox caret: fixed by text.

**Perf matrix.** `C:\mux-ir\run-matrix-integrated.ps1` runs the 27 entries × {composition, gdiplus, d2d-sw}. Each run is
one `Run-RenderBench.ps1 -Modes <mode> -Scenes <entry> -Repetitions 1 -Quick -InputMode programmatic -DeleteTraces` process
under the bench lock, writing to `C:\mux-ir\results\matrix-integrated`.

80/81 gates pass. The exception is S22 `accessibility` Composition, `CaptureWithinTolerance` 23.3 (max 8), and it is
environmental:

- At 1024×768 the window extends past the desktop, so the on-screen capture is clipped.
- The immediate modes compare against their own presented buffer and pass.

Geometric means vs same-session Comp (Lifted):

| Measure | GDI+ | D2D-SW |
|---|---|---|
| Steady | 81% | 98% |
| Peak | 83% | 102% |
| Steady, previous matrix | 92% | 101% |
| Peak, previous matrix | 84% | 107% |

- D2D-SW is within the ±3 point target.
- GDI+ is 11 points lower, and the cause is the Comp baseline, not the immediate backends.
  - In this session Comp no longer drops from its start-up peak to about 32 MB at idle. Its steady geo mean is 114% of
    the first matrix, while its peak is 93%.
  - The pre-coverage kit (renderloop worktree) reproduces it in this session: S7 45 MB and S13 49 MB, against 32 and
    33 MB earlier.
- The immediate modes' absolute steady memory is flat:
  - GDI+ is 101% of the first matrix (peak 91%).
  - D2D-SW is 96% of its after-fix run (peak 94%).
- Notable per-scene changes beyond ±10%:
  - S5 gallery and gallery-decoded: GDI+ 21-31% lower, D2D-SW 15-20% lower. This comes from imaging's zero-copy
    bitmaps and content generations.
  - S2, S7, S13 and S21: Comp steady 36-53% higher, from the environment described above.

The report is `C:\mux-ir\results\memory-comparison-s1-s23.html`, in the "After full XAML coverage" and "XAML coverage"
sections. The CSV is `memory-s1-s23.csv`.

**Tests.**

All tests were run against the committed integration head, built in the `C:\mux-ir\base` build environment.

| Command | Total | Passed | Failed | Skipped |
|---|---|---|---|---|
| `validate-branch.ps1 -Backend integration -TestFilter '*'` (Isolated.Base TAEF) | 511 | 394 | 0 | 117 |
| `validate-branch.ps1 -Backend integration -TestFilter 'RenderingBackendTests::*' -Mux -Muxc -Stage -Kit`: TAEF | 455 | 338 | 0 | 117 |
| The same command: RenderBench.Tests (kit) | 116 | 115 | 0 | 1 |

All 117 skipped TAEF tests are the D2D-HW painter variants, which skip because there is no GPU adapter.

**Known gaps.**

- Scrolling:
  - Wheel, touch, pinch, inertia and keyboard scrolling jump instead of animating, because there is no DManip compositor.
  - Input injection is unavailable on the VM.
- Animations:
  - ExpressionAnimations and template-part animations silently paint their static value.
  - Transitions don't advance on the render thread while the UI thread is blocked.
- Per-window Composition fallback (by design): app Composition animations, hand-off path clips, app brushes and lights,
  SwapChainPanel, media and AnimatedVisualPlayer sources.
- Approximations:
  - Mica and DesktopAcrylic show white behind the swap chain (`:approximated`; thresholds 0.06 and 0.035).
  - Radial gradients interpolate in HSL approximately.
  - TextBox and RichEditBox gradient text uses the average colour.
  - The ConnectedAnimation cross-fade is approximated.
  - AnimatedIcon shows its fallback icon.
- Not tested:
  - High contrast can't be tested per process.
  - D2D-HW is unmeasured because the VM has no GPU, and its tests skip.
  - S22's Comp screen capture needs a desktop larger than the window.

## Backend decision scenarios (S24-S32)

S1-S23 compare typical pages at one size. Choosing a backend for an app depends on how cost grows with the app's
shape, so S24-S32 measure cost curves instead:

- fixed per-window cost;
- cost per window pixel and per visible element;
- behaviour while minimized or hidden;
- churn and leaks;
- CPU under continuous animation.

Every scene is a deterministic step sweep using programmatic input only. Each step records one `StepMetrics`, and each
(scene, backend, repetition) runs in its own process. Scene code: `RenderBench\DecisionScenes.cs`. Analysis:
`RenderBench.Core\Analysis\MetricsCalculator.Decision.cs`. Run them with `Run-RenderBench.ps1 -Scenes decision`
(aliases s24-s32).

### Scene definitions

| Id | Scene | Steps | Question |
|---|---|---|---|
| S24 | `winsize` | The S1 settings page at client sizes 640x480, 1280x800, 1920x1080, 2560x1440 and 3840x2160. | Does the immediate back buffer overtake the savings on large or high-resolution windows? MB per megapixel (slope) and intercept. |
| S25 | `density` | A full 1280x800 window with 250, 1k, 4k, 16k and 32k visible elements: a third each Border+TextBlock tiles (rounded corners), Rectangles and Ellipses. No virtualization. | Where does Composition's per-visual cost exceed immediate's? MB per 1k elements, and CPU ms per full-window frame. |
| S26 | `windowcount` | 1, 2, 4, 8 and 16 open 400x300 windows with light content. | Fixed per-window cost: MB per extra window (slope). |
| S27 | `background` | visible, minimized (12 s), restored, hidden (`AppWindow.Hide`, 12 s), shown, occluded (a topmost opaque popup covering the window), unoccluded. | Are back buffers and surfaces released when the window is not visible? This matters for tray and background apps. |
| S28 | `navchurn` | A Frame (CacheSize 0) shows 200 new pages, cycling through 10 page types. Each page has a unique 128x128 WriteableBitmap and seeded text. A full GC and a step every 20 navigations. | Growth slope (MB per 100 navigations) and the size of the painter caches. |
| S29 | `sustained` | pre (idle), animating (60 s: 200 tiles rotating forever plus a 20-tile ticker updated every frame), stopped. | CPU ms per second (process and by thread category), FPS and memory under the immediate backends' worst case. |
| S30 | `resize` | pre, storm (150 programmatic resizes to seeded sizes between 800x600 and 1600x1000, one frame each), settle (back to the start size), recovered (after a full GC). | Peak during the storm, steady state after settling, and recovery to the pre-storm level. |
| S31 | `minimal` | A 320x200 tray-style window with one TextBlock and one Button: idle (after one click), then after-gc. | Each backend's memory floor and fixed initialization cost. |
| S32 | `largecanvas` | A ScrollViewer over a 10000x10000 Canvas with 3000 items (a third each Border+TextBlock nodes, connecting Lines and shapes). Steps at ZoomFactor 1, 0.25 and 4, each followed by a 60-frame programmatic pan (`ChangeView`, disableAnimation). | Composition's virtual surfaces and tiling versus immediate repaint of the viewport. |

### Metrics

Each step's values are read from the second half of the step, after it has settled.

| Metric | Definition |
|---|---|
| In-proc MB | RenderBench private commit: the median of the 500 ms samples in the step. **Peak** is the largest sample in the step. |
| WS MB | Working set, median. |
| DWM Δ MB | Private bytes of the session's `dwm.exe` (the instance in RenderBench's session) at the step, minus a baseline taken before RenderBench creates its window. Composition surfaces, and the immediate backends' swap chains as composed by DWM, live there; in-process private bytes don't count them. Reading DWM needs the elevated shell. |
| Total MB | In-proc MB + DWM Δ MB. |
| CPU ms/s | Process CPU per second over the step. Per-thread categories (UI, WARP rasterizer, D2D/DWrite, Composition, Other) come from ETW CPU sampling. |
| Full-frame CPU ms | S25: the root background is toggled five times, two frames apart, each a full-window invalidation. The value is the process CPU per invalidation, averaged over the five. |
| Frames / FPS | Presented frames in the step. |
| Gates | UnsupportedContent = 0 and WindowFallbackCount = 0 for every run, plus the usual harness gates. |

The analysis computes these summaries:

- least-squares slope and intercept: S24 MB per megapixel, S25 MB per 1k elements, S26 MB per window;
- S28 growth slope over nav-20 to nav-200, after first use;
- S27 deltas of each state against the visible step;
- S30 storm peak, settle and recovered deltas against pre;
- S29 CPU ms per second by thread category.

Kit tests cover the fits, the per-window cost, the growth slope, the CPU rates and the option parsing of the new scenes
(`RenderBench.Tests\DecisionScenesTests.cs`).

### Deviations from the requested design

- S24:
  - The window is placed at (0,0) and extends past the VM's 1024x768 desktop. `WM_GETMINMAXINFO` is raised so that
    Windows doesn't clamp it.
  - The `ClientWidth` and `ClientHeight` reported per step confirm that every size is reached, and frames are
    presented at each size.
  - 3840x2160 is included.
- S25 measures five full-window invalidations per step instead of one, and reports their average.
- S27:
  - WinUI doesn't detect occlusion: `XamlRoot.IsHostVisible` stays true under the occluding popup. The occluded step
    therefore shows no backend reaction.
  - Invisible states hold for 12 s, which is longer than the 10 s requested and longer than the framework's 2 s
    background-resource timer.
- S28:
  - `Frame.Navigate` needs XAML type metadata, and RenderBench's code-only pages have none: Navigate fails with an
    access violation.
  - Each navigation therefore replaces `Frame.Content` with a new page instance, so there is no back stack. This is
    equivalent for page creation and teardown and for the caches.
- S29 uses 200 rotating tiles and a 20-tile ticker.
- DWM baseline:
  - dwm.exe keeps a closed window's surfaces until another top-level window is shown and closed. After a 4K window
    that is about 30 MB, which would otherwise leave the next run with DWM Δ ≈ -32 MB.
  - `Run-RenderBench.ps1` therefore shows and closes a small WinForms window before every decision run (Invoke-DwmSettle).
    RenderBench then waits for dwm.exe private bytes to stabilize before taking the baseline.
  - An in-process probe window, even a captioned one, did not trigger the release.
- D2D-HW isn't measured: the VM has no GPU.
- The ResourceInventory surface column (Backend surf MB) reflects one window.
- Harness idle CPU (sampling, ETW) is a constant 170-290 ms/s offset on this VM in every backend. Compare CPU across
  backends rather than in absolute terms.

### Decision results

The run is the matrix from 24 November 2026 on the GPU-less Hyper-V VM: two repetitions per (scene, backend), with medians
reported. The report is `C:\mux-ir\results\decision-scenarios.html`, generated by `C:\mux-ir\results\build-html-decision.ps1`
from `C:\mux-ir\results\decision\{main,system,notrim}`. Total MB = in-process private MB + DWM Δ.

The GDI+ column is from before the bounded layers. The re-measurement is in [GDI+ bounded layers](#gdi-bounded-layers).

| Scene | Metric | Comp (Lifted) | Comp (System) | GDI+ | D2D-SW |
|---|---|---|---|---|---|
| S24 winsize | Total MB per megapixel (slope) | 0.15 | 0.09 | 8.3 (5.1-11.5) | 0.96 |
| S24 winsize | Intercept MB | 48.8 | 39.2 | 31.2 | 43.5 |
| S25 density | MB per 1k visible elements | 98 | 86 | 4.9 | 23.6 (15.6-31.6) |
| S25 density | Full-frame CPU ms at 32k elements | 1600 | 1062 | 423 | 29,219 (5.3-53 s) |
| S26 windowcount | Total MB per extra window | 1.35 | 0.42 | 1.48 | 3.16 |
| S26 windowcount | First window MB | 31.4 | 25.4 | 28.5 | 34.3 |
| S27 background | In-process Δ minimized / hidden (MB) | -0.2 / 0 | +0.6 / +0.7 | -11.8 / -12.2 | -7.4 / -6.6 |
| S28 navchurn | Total MB per 100 navigations | 6.0 | 4.1 | 5.0 | 7.7 |
| S29 sustained | Process CPU ms/s | 512 | 438 | 1247 | 3275 |
| S29 sustained | FPS | 31.6 | 31.5 | 34.7 | 37.8 |
| S29 sustained | Total MB | 86.8 | 54.1 | 44.8 | 93.9 |
| S30 resize | Storm peak MB | 48.4 | 43.6 | 44.8 | 51.3 |
| S30 resize | Δ after settle vs pre-storm (MB) | -3.3 | -3.0 | -1.5 | +7.6 |
| S30 resize | Storm CPU ms/s | 960 | 1131 | 1295 | 1762 |
| S31 minimal | Floor in-process MB (Total) | 26.6 (27.9) | 23.8 (23.4) | 24.2 (23.1) | 28.3 (27.8) |
| S32 largecanvas | Peak MB | 850, crashed | 808, crashed | 64 | 74 |
| S32 largecanvas | Pan CPU ms/s | 299 | 271 | 641 | 908 |
| S32 largecanvas | Zoom 4 Δ MB vs zoom 1 | +360 | +317 | +1.3 | +11.7 |

Crossovers and slopes:

- S24: GDI+ crosses Comp (Lifted) at about 2.1 MP (about 1920x1080). D2D-SW crosses it at about 6.5 MP. Neither
  immediate backend is below Comp (System) above about 1 MP.
- S25: there is no crossover. GDI+ and D2D-SW are below Composition from 250 elements upwards, and the gap grows with
  the element count.
- S26: Comp (System) has the smallest fixed per-window cost. D2D-SW has the largest, because each window has its own
  painter and swap chain.
- S28: all backends grow by similar amounts, about the size of the unique 64 KB WriteableBitmap each page creates. The
  painter caches stay bounded: the cache maximum equals the end value, 7.4 MB for GDI+ and 6.5 MB for D2D-SW.

S1 anchor (steady MB, same session vs main matrix): Comp 46.5 vs 46, GDI+ 36.8 vs 46, D2D-SW 39.7 vs 44. The decision
runs sit at or below the main matrix, so they can be compared with it.

#### S27 hidden-window trim (product change)

- Before this branch, the immediate backends kept their swap chain and painter layers while minimized or hidden.
- The branch adds a per-island trim. When `XamlRoot.IsHostVisible` has been false for 2 s, ImmediateRenderer
  releases the island's swap chain and painter caches and calls `IDXGIDevice3::Trim`. The next visible frame recreates
  them.
  - The render walk is forced on visibility changes (`RequestImmediateVisibilityWalk`), so the trim runs even
    when nothing is dirty.
  - The trim runs only on the immediate path, and `WINUI_IMMEDIATE_TRIM_HIDDEN=0` disables it.
  - Native tests are in `RenderingBackendTests`.

| Backend | Minimized Δ MB, no trim | Minimized Δ MB, trim | Hidden Δ MB, no trim | Hidden Δ MB, trim |
|---|---|---|---|---|
| GDI+ | -0.95 | -11.8 (-7.1..-16.9) | -1.18 | -12.2 |
| D2D-SW | -0.33 | -7.4 | +0.76 | -6.6 |

Restore and show return to the visible level with no visible delay.

#### Findings and open questions

- **Comp `largecanvas` runs out of memory.** Both engines fail with E_OUTOFMEMORY (0x8007000e, stowed exception
  0xC000027B) in Microsoft.UI.Xaml.dll during the ZoomFactor 4 pan, at about 810-850 MB private, in 4 of 4 runs.
  - On this VM the WARP device and its surfaces are in-process.
  - Not yet checked whether stock WinUI behaves the same.
- **GDI+ winsize variance (fixed).** GdiPlusPainter allocated a full window-size layer per opacity or rounded-clip
  nesting depth, plus a mask, lazily. It kept them until the next resize, so its surfaces were 1-3x the window,
  depending on content and timing.
  - Fixed by [GDI+ bounded layers](#gdi-bounded-layers). At 3840x2160 the GDI+ surfaces are now just the back buffer
    (31.6 MB, was 94.9).
- **D2D-SW density pathology (not profiled).** Each rounded Border pushes a geometry-mask layer: about 10.7k per frame
  at 32k elements on WARP. The 32-entry clip-geometry LRU thrashes. Full-frame time varies between 5 and 53 s.
- **Navchurn retention.** The growth is the same in every backend and matches one bitmap per page. Pages or their
  bitmaps may be retained outside the backend (not backend-specific).
- **Occlusion** isn't reported by WinUI, so no backend reacts to it.
- **Thread categories:** WARP rasterizer threads are counted as "Other".
- **D2D-HW** wasn't measured: the VM has no GPU.

### Backend decision guide

| Decision factor | Memory winner | CPU winner | Measured crossover or slope | Recommendation |
|---|---|---|---|---|
| Large or high-DPI windows | Comp (System) | Comp | GDI+ ~3.9 MB/MP (bounded layers; was 5-12) crosses Comp (Lifted) at ~4.7 MP (was ~2); D2D-SW ~1 MB/MP in-process crosses at ~6.5 MP | Composition for large windows when memory matters. GDI+ is now competitive up to about 2560x1440. |
| Element-dense UI | GDI+ | GDI+ | Comp 86-98 MB per 1k elements vs GDI+ 4.9; no crossover | GDI+. Avoid D2D-SW on WARP with many rounded clips. |
| Many windows | Comp (System) | n/a | 0.4 (System), 1.35 (Lifted), 1.5 (GDI+), 3.2 (D2D-SW) MB per window | Comp (System); Lifted and GDI+ are close. |
| Tray or background app | GDI+ / D2D-SW with trim | n/a | -4.5 MB (GDI+, from a visible level that is now ~10 MB lower; -12 MB before bounded layers) and -7 MB (D2D-SW) within ~2 s when hidden | Immediate with the hidden-window trim. |
| Long-running, navigation-heavy app | none | n/a | 4-8 MB per 100 navigations in every backend | Not a differentiator. Painter caches are bounded. |
| Continuous animation on CPU-only machines | GDI+ | Comp | 440-510 vs 1250 (GDI+) vs 3270 (D2D-SW) ms/s at 31-38 FPS | Composition. |
| Resize-heavy | Comp / GDI+ | Comp (Lifted) | D2D-SW keeps +7.6 MB after the storm | Composition or GDI+. |
| Tiny utility | GDI+ / Comp (System) | n/a | Floors 23-24 MB vs 26.6 (Lifted) and 28.3 (D2D-SW) | GDI+ or Comp (System). |
| Huge zoomable canvas | GDI+ | Comp, until it crashes | Comp +317-360 MB at zoom 4, then OOM; immediate +1-12 MB | Immediate. |
| Interop-heavy (SwapChainPanel, media, Lottie, app visuals) | n/a | n/a | Falls back per window | Composition, which those windows use anyway. |

## GDI+ bounded layers

The branch `user/neeverma/immediate-rendering-gdilayers` replaces the GDI+ full-window layers with bounds-sized,
pooled, idle-trimmed layers. The design is in the proposal, under
[GDI+ bounded layers](immediate-rendering-backends-proposal.md#gdi-bounded-layers).

### Cost model

- Before: each opacity, rounded-clip or geometry-clip nesting depth held one window-sized PARGB layer (4 B/px). A
  rotated rounded clip added one window-sized mask. Nothing was released until the next resize.
- After:
  - Each layer is the size of clip ∩ damage (∩ the group's device bounds for UI-thread opacity groups), rounded up to
    64 px and capped at the target size.
  - Layers, the mask, the shadow scratch and blur buffers not used for 2 s are released. So are layers more than 2x
    their recent peak.
  - The steady-state GDI+ surface is the back buffer only.
  - The trim runs on a frame tick without a render walk, so an idle window still draws 0 frames per minute.
  - `ResourceInventory::layerBytes` reports the pooled layers and masks; they are also included in `surfaceBytes`.

### Tests

New tests in `GdiPlusPainterTests` (RenderingBackendTests). They were written first and failed against the
decision head:

- `LayersAreSizedToTheirBounds`: a small opacity group, rounded clip or geometry clip in a large target allocates
  about bounds × 4 bytes, not width × height × 4.
- `OpacityBoundsHintLimitsLayer`: `PushOpacityLayerWithBounds` limits the layer to the hint.
- `NestedLayersAtOffsetsComposite`: nested opacity and rounded-clip layers at different origins, including one
  past the target's edge under a fractional translation and a repaint with partial damage, give the expected pixels.
- `LayerContentMatchesDirectDrawing`: text, exact and mask-and-shade gradients, a shadow, a bitmap, backdrop blur,
  invert and a rotated clip drawn through nested offset layers at opacity 1 match drawing them directly (≤ 2 per
  channel).
- `PooledLayersAreReusedAndGrow`: layers are reused across frames and grow in 64 px buckets.
- `IdleTrimReleasesLayersAndScratch`: `TrimTransientResources` releases layers, masks and scratch after the idle
  delay, and reports when the next release is due.

Validation used `C:\mux-ir\validate-branch.ps1 -Backend gdilayers -Mux -Stage -Kit`:

- RenderingBackendTests: Total=462, Passed=345, Failed=0, Skipped=117. The skips are the D2D-HW, GPU and WinAppSDK
  cases, the same as on the decision head.
- Kit tests: Passed=122, Failed=0, Skipped=1.
- Coverage corpus (`Run-Coverage.ps1`): 195/195 in each of Composition, GDI+ and D2D-SW.
  - The `ScrollViewer-MouseWheel` page needed a rerun in all three modes: an unrelated window covered the
    injected-wheel point.
  - No page's fidelity changed by more than 0.002 against `coverage-integrated`. The mean diff is 0.00101 for GDI+
    and 0.00089 for D2D-SW, unchanged.

### Results

Results are in:

- `C:\mux-ir\results\matrix-gdilayers` and `C:\mux-ir\results\decision-gdilayers\{main,system,notrim}`: a full
  same-session rerun of all backends.
- `C:\mux-ir\results\decision-gdilayers-ab`: GDI+ old build against new build, interleaved, two rounds.
- Reports: `memory-comparison-s1-s23.html` (columns `gl-*`) and `decision-scenarios.html` (section "GDI+ bounded
  layers (before → after)"). The previous versions are kept as `*.pre-gdilayers.html`.

Session drift: the rerun session had the VM console active, so DWM composed the windows at 64 Hz. The decision run
had a disconnected session at 32-60 Hz. Comp and D2D-SW changed with it: for example, Comp (Lifted) S29 went from
512 to 1174 ms/s and D2D-SW's S24 DWM share grew. Compare GDI+ before and after through the same-session A/B, or
against the same-session Comp, rather than across sessions.

S1-S23 (geomean of the steady and peak private MB over 27 entries, same session):

| | Steady | Peak |
|---|---|---|
| GDI+ / Comp (Lifted), before (integrated) | 81% | 83% |
| GDI+ / Comp (Lifted), after | 74% | 77% |
| GDI+ after / GDI+ before (absolute) | 92% | 92% |
| D2D-SW / Comp (Lifted), after (before) | 98% (98%) | 104% (102%) |

The largest GDI+ reductions are in S9 multiwindow (peak 89 → 70 MB), S23 appshell (steady 72 → 58), S20 dpitheme
(44 → 35), S6 animated (42 → 34) and S1 settings (44 → 37). The matrix gates passed in 80 of 81 runs. The one
failure is accessibility in Composition (CaptureWithinTolerance), the same as in the integrated run.

Same-session A/B (GDI+, old = decision head kit, new = this branch, median of 2 interleaved rounds):

| Metric | Old | New |
|---|---|---|
| S1 steady / peak private MB | 45.1 / 45.5 | 35.5 / 37.3 |
| S24 in-process MB per megapixel | 11.5 | 3.85 |
| S24 in-process MB at 3840x2160 | 128 | 64.7 |
| S24 backend surfaces MB at 3840x2160 | 94.9 | 31.6 |
| S25 MB per 1k elements | 5.17 | 5.19 |
| S25 full-frame CPU ms at 32k elements | 594 | 588 |
| S29 CPU ms/s / FPS (CPU ms per frame) | 1265 / 30.7 (41.2) | 1328 / 29.9 (44.5) |
| S29 in-process MB animating | 48.8 | 40.4 |
| S32 peak MB / pan CPU ms/s | 66.1 / 554 | 63.1 / 543 |

Decision scenarios (GDI+ before = the decision run, after = the rerun; Comp and D2D-SW are from the same rerun):

| Scene | Metric | GDI+ before | GDI+ after | Comp (Lifted) | Comp (System) | D2D-SW |
|---|---|---|---|---|---|---|
| S24 winsize | In-process MB per megapixel | 8.31 | 3.85 | 0.11 | -0.25 | 1.11 |
| S24 winsize | Total MB per megapixel | 8.33 | 3.83 | 0.00 | -0.19 | 4.94 |
| S24 winsize | Total intercept MB | 31.2 | 29.1 | 47.1 | 39.5 | 44.0 |
| S24 winsize | Crossover vs Comp (Lifted), MP | 2.1 | 4.7 | - | - | 0.6 |
| S25 density | MB per 1k elements | 4.95 | 5.29 | 103 | 83 | 34.5 |
| S25 density | Full-frame CPU ms at 32k | 423 | 558 | 1028 | 481 | 35,672 |
| S26 windowcount | Total MB per extra window | 1.52 | 1.39 | 1.29 | 0.35 | 3.20 |
| S27 background | In-process Δ minimized / hidden | -11.8 / -12.2 | -4.5 / -4.6 | +0.1 / -0.4 | +0.1 / +0.2 | -6.1 / -9.9 |
| S28 navchurn | Total MB per 100 navigations | 5.2 | 5.3 | 8.4 | 3.8 | 5.7 |
| S29 sustained | CPU ms/s / FPS | 1247 / 34.7 | 1253 / 31.9 | 1174 / 53.1 | 546 / 57.7 | 3101 / 32.7 |
| S29 sustained | Total MB | 44.8 | 39.3 | 109.5 | 65.7 | 145.9 |
| S30 resize | Storm peak MB / CPU ms/s | 44.8 / 1295 | 44.6 / 1420 | 53.6 / 1571 | 42.3 / 1285 | 56.0 / 2248 |
| S31 minimal | Floor in-process (Total) MB | 24.2 (23.1) | 26.1 (23.3) | 30.2 (26.7) | 23.8 (22.0) | 30.2 (27.4) |
| S32 largecanvas | Peak MB / pan CPU ms/s | 64.4 / 641 | 63.6 / 574 | 1593 / 620 | 1476 / 302 | 161 / 1043 |

Notes:

- The S24 D2D-SW crossover moved from 6.5 to 0.6 MP because its DWM Δ grows with area when the console is active.
  Its in-process slope is unchanged at about 1 MB/MP.
- Composition S32 reached 1.4-1.6 GB in every run of this session without crashing. In the decision session it
  crashed at about 810-850 MB.
- The D2D-SW S32 peak (161 vs 74 MB before) is session drift: the decision-head build also peaks at 153 MB in this
  session (`C:\mux-ir\results\decision-gdilayers-ab-d2d`).
- The S27 GDI+ trim releases less because the layers are already released while visible. The visible level fell from
  39-48 MB to about 37 MB, while the hidden floor is unchanged at about 32 MB.
- The S25 full-frame time varies between sessions (253-594 ms before). The same-session A/B shows the new build is not
  slower.
- S29 GDI+ CPU per frame is about 8% higher in the A/B (41.2 → 44.5 ms; the rounds range 39.9-42.5 old vs 44.2-44.8 new). The decision rerun shows 1247 → 1253 ms/s. A small per-frame cost is possible: layer rects are recomputed every frame, and offset blits are used. Not profiled.

### Caveats

- No renderer-level unit test covers the trim tick or the render-thread bound hint; they are covered by integration
  runs only (the SteadyStateIsIdle gate and the backend surface MB).
- The first rerun was invalid because the session was locked (composition captures were uniform). The session was
  moved to the console with `tscon` and the runs restarted.

## S33 scroll physics (DManip ticking)

Branch dmanip (proposal, "DManip ticking (scroll physics)"). Scene code: RenderBench\ScrollPhysicsScene.cs
(--scene scrollphysics, plan id S33, part of the decision group). Each step resets the view, injects real input,
records the offset trajectory (ScrollViewer.ViewChanging / ScrollView.ViewChanged with QPC timestamps) until 500 ms
without change, then idles 1.5 s. The trajectory goes to 	rajectory.csv and the step values to the metrics.

### Steps

| Step | Input |
|---|---|
| wheel-1, wheel-3 | 1 and 3 mouse-wheel notches (SendInput) over a ScrollViewer list of 60 px rows |
| flick | Touch flick with inertia (InjectSyntheticPointerInput, PT_TOUCH) |
| pinch | Two-contact touch pinch on a zoomable ScrollViewer (MaxZoomFactor 4) |
| pgdn | Keyboard PgDn with focus in the ScrollViewer |
| snap | Touch flick over 120 px rows with mandatory snap points |
| bringintoview | StartBringIntoView with AnimationDesired to a row 2400 px down |
| changeview | ChangeView(null, 2400, null, false) |
| sv-wheel, sv-flick | Wheel notch and touch flick on a MUXC ScrollView |
| flick-blocked | Touch flick; the UI thread sleeps 200 ms during inertia while a sampler thread reads back a viewport column. Run with `WINUI_IMMEDIATE_OFFTHREAD_SCROLL` off and on |
| overpan | Touch drag of a ScrollViewer past its top extent, then release; the gutter above row 0 is read back from the screen (`overpan-screen` trajectory, `OverpanMaxPx`) |
| sv-overpan | The same on a MUXC ScrollView |

### Running

Input injection needs an interactive desktop: on the lab VM the RDP session was moved to the console with
	scon <id> /dest:console. The OS "Play animations in Windows" setting is off on the VM; DManip then disables inertia
and animated scrolling in every mode, so S33 runs with it temporarily on (C:\mux-ir\with-animations.ps1, which restores
it). Helper scripts outside the repo: C:\mux-ir\run-s33.ps1 (per-mode runs with WINUI_IMMEDIATE_DMANIP_LOG and
WINUI_IMMEDIATE_STATS), C:\mux-ir\analyze-s33.ps1 (fidelity, summary.json), C:\mux-ir\build-html-s33.ps1
(C:\mux-ir\results\scroll-physics-s33.html).

### Metrics

- **Raw fidelity**: share of 16 ms samples (input-relative timeline, linear interpolation) where the immediate offset is
  within max(1 px, 2% of the travel) of every Composition run.
- **Shifted fidelity**: the same after the best constant shift of 0-64 ms (4 ms steps); the shift is the start lag.
- **Update Hz / p95 interval**: offset updates per second and the p95 interval during motion.
- **Motion / idle CPU**: process CPU ms/s during motion and during the 1.5 s idle.
- **DManip updates**: Update calls during motion and after settle (native log).
- **Blit / repaint frames**: motion frames that scrolled by blit vs repainted the viewport (stats file).
- **Bounce**: the screen-sampled gutter compared with every Composition run; max deviation raw and after the best
  0-64 ms shift, and the peak gutter difference in px.
- **Blocked screen changes**: screen samples that changed during the 200 ms UI block, out of the samples taken (each
  readback takes ~50 ms, so 4-7 fit).

### Results

Three runs per mode, one session, GPU-less VM (~32 Hz UI and DWM). C = Composition, G = GDI+, D = D2D-SW. "Before" is
the branch base (d0e3b0f2b), two runs per immediate mode. Results: C:\mux-ir\results\dmanip-s33,
C:\mux-ir\results\dmanip-s33-before.

| Step | Final offset (zoom) C / G / D | Raw fidelity % C / G / D | Shifted % (lag ms) G / D | Update Hz C / G / D | p95 interval ms C / G / D | Motion CPU ms/s C / G / D | Peak MB C / G / D | Before final G / D |
|---|---|---|---|---|---|---|---|---|
| wheel-1 | 90 / 90 / 90 | 88 / 63 / 56 | 69 (4) / 73 (8) | 33 / 35 / 32 | 31.7 / 31.6 / 33.3 | 390 / 377 / 293 | 54 / 48.5 / 56 | 90 / 90 |
| wheel-3 | 270 / 270 / 270 | 100 / 17 / 17 | 82 (32) / 86 (24) | 32 / 32 / 32 | 31.9 / 31.7 / 31.7 | 161 / 178 / 178 | 54 / 48.5 / 56.4 | 270 / 270 |
| flick | 1440 / 1440 / 1440 | 5 / 53 / 45 | 53 (0) / 53 (0) | 32 / 32 / 32 | 32.7 / 31.8 / 31.9 | 197 / 215 / 242 | 58 / 52.5 / 60.4 | 1440 / 1470 |
| pinch | 2.411 / 2.400 / 2.405 | 76 / 73 / 67 | 95 (4) / 91 (4) | 99 / 99 / 99 | 31.1 / 31.1 / 31.1 | 221 / 234 / 218 | 58.7 / 54.8 / 68.1 | 2.397 / 2.364 |
| pgdn | 601 / 601 / 601 | 100 / 41 / 38 | 97 (32) / 97 (36) | 32 / 32 / 32 | 31.8 / 31.5 / 31.6 | 140 / 132 / 133 | 58.8 / 54.8 / 69.3 | 601 / 601 |
| snap | 1440 / 1440 / 1440 | 35 / 94 / 60 | 100 (8) / 100 (12) | 32 / 32 / 32 | 31.7 / 31.7 / 31.7 | 159 / 145 / 134 | 61.4 / 54.8 / 70.2 | 1440 / 1440 |
| bringintoview | 2400 / 2400 / 2400 | 100 / 68 / 71 | 87 (12) / 93 (12) | 32 / 32 / 33 | 31.9 / 31.6 / 31.6 | 142 / 171 / 173 | 62.4 / 55.1 / 70.3 | 2400 / 2400 |
| changeview | 2400 / 2400 / 2400 | 100 / 77 / 75 | 87 (4) / 88 (4) | 33 / 33 / 32 | 31.7 / 31.7 / 33.2 | 156 / 173 / 162 | 62.4 / 55.1 / 70.3 | 2400 / 2400 |
| sv-wheel | 48 / 48 / 48 | 28 / 55 / 45 | 72 (4) / 45 (0) | 35 / 30 / 32 | 32.3 / 45.4 / 32.1 | 179 / 321 / 116 | 70.9 / 61.9 / 75 | 0 / 0 |
| sv-flick | 1072.9 / 1094.4 / 1074.3 | 98 / 79 / 90 | 93 (4) / 97 (4) | 32 / 31 / 31 | 32.3 / 48.5 / 32.7 | 171 / 499 / 149 | 70.9 / 62.1 / 75.6 | 0 / 0 |
| flick-blocked | 1500 / 1440 / 1500 | 100 / 44 / 97 | 57 (8) / 99 (0) | 26 / 26 / 26 | 31.7 / 36.4 / 32.5 | 171 / 202 / 158 | 71.4 / 62.3 / 77.7 | 1470 / 1500 |


- **Finals** match Composition in every step except the pinch zoom (2.400 / 2.405 vs 2.411, within the run-to-run range
  2.31-2.41) and the ScrollView flick (1094 / 1074 vs 1073: InteractionTracker inertia varies with the injected
  velocity).
- **Shape**: immediate motion starts about one UI frame later (StartLag 24-36 ms on wheel-3 and PgDn); after the shift
  PgDn, snap and the ScrollView flick are 93-100% within tolerance. Raw fidelity of the flick is low in every mode,
  including Composition run to run (5%), because the injected touch velocity varies.
- **Before**: offsets jumped (flick deviation ~1050 px, BringIntoView ~1900 px, PgDn 358 px), and ScrollView did not
  scroll (final 0).
- **Idle**: 0 DManip updates after settle in every step and mode.
- **Blocked UI**: Composition changed 6 of 7 screen samples during the 200 ms block, GDI+ and D2D-SW 0 of 7 (ticking
  runs on the UI thread), followed by a ~470-480 px catch-up.
- **Memory**: the peaks are cumulative over the scene; GDI+ stays below Composition, D2D-SW above it (WARP).
- **Sanity subset** (S3, S4, S8, S23, S24, S31; `Run-RenderBench.ps1 -Quick -InputMode programmatic`, pre-change bench
  `-BenchExe` vs this branch, same session, `C:\mux-ir\results\dmanip-sanity`): all gates pass, and steady private
  memory rises by at most 1 MB per scene in GDI+ and D2D-SW (Inbox, Reader and DataGrid rerun with 2 repetitions;
  AppShell D2D-SW was 4.6 MB lower in its single run), which is within noise. Ticking only runs while a viewport is moving, so the steady and idle phases are unaffected.

### Coverage

Run-Coverage.ps1 -Results C:\mux-ir\results\coverage-dmanip-final -DeleteBmps (OS animations off, as the 195-page
baseline): Composition 199/199, GDI+ 198/199, D2D-SW 198/199 (199/199 each after the rerun). The one failure, F14-theming/focus-highvisibility
(fidelity 0.0012 vs 0.001 in both immediate modes), passed 3/3 on rerun (coverage-dmanip-final-rerun). F9 with OS
animations on (coverage-dmanip-f9-animations): 19/19 in each mode. With animations on, four ConnectedAnimation pages
and two F10 transition pages fail in the immediate modes by timing; the baseline runs with animations off.

The first full run (coverage-dmanip) crashed with 0xC000027B at shutdown on the seven ScrollPresenter pages. This was
fixed by detaching the hit-test proxies when the island is removed.

### Scroll gaps results

Branch `user/neeverma/immediate-rendering-scrollgaps`. The full report is `C:\mux-ir\results\scroll-gaps-s33.html`; the
per-step data is `scroll-gaps-s33-data.csv`. S33 ran with 3 repetitions per mode (median) on the console session with OS
animations on, once with the opt-in off (`scrollgaps-final-off`) and once with `WINUI_IMMEDIATE_OFFTHREAD_SCROLL=1`
(`scrollgaps-final-on`). This session ran at ~55 Hz, while the earlier DManip results ran at ~32 Hz, so compare CPU only
against Composition in the same run.

| Gap | Target | Result (G = GDI+, D = D2D-SW) |
|---|---|---|
| 2 Start latency | Lag ≤8 ms vs Comp; PgDn / wheel / BringIntoView raw fidelity ≥95% | Partly met. Lag 24–36 → 0–12 ms. Raw PgDn 41/38 → 74/77 %, wheel-3 17/17 → 83/70 %, BringIntoView 68/71 → 71/81 %. Comp's own wheel-3 raw fidelity across runs is 71–75 % |
| 5 GDI+ ScrollView flick CPU | Within 20 % of Comp | Met. 313 vs 277 ms/s (opt-in off), 312 vs 379 (on); was 499 vs 171. 80–83 blit frames; fidelity 97–100 % |
| 4 Proxy coverage | Wheel and flick under perspective, hand-off transform and windowed popup | Met. The three F9 proxy pages pass in all modes |
| 3 Overpan bounce | Within 2 px of Comp | Missed, within noise. Max deviation after shift: ScrollViewer 8.3 / 10.6 px, ScrollView 18.8 / 27.1 px, against Comp-vs-Comp 8.0 / 13.7 |
| 1 Blocked UI thread | ≥5/7 screen samples move | Partly met (opt-in). 4/5 in G and D (Comp 4/5); 0/5 and 0/4 with it off |

- **Native and kit tests.** `C:\mux-ir\validate-branch.ps1 -Backend scrollgaps -TestFilter 'ImmediateRenderingPolicyTests::*' -Mux -Stage -Kit`:
  35/35 native tests and 126 kit tests passed (0 failed, 1 skipped). After the windowed popup fix, `-Mux -Stage` gave 35/35.
- **Coverage.** `Run-Coverage.ps1 -Pages '*,!F9*' -Results C:\mux-ir\results\coverage-scrollgaps-final -DeleteBmps`
  (animations off): 180/180 in each mode. F9 with animations on: 22/22 in each mode (`coverage-scrollgaps-f9-animations-fix`).
  - The first F9 run crashed `ScrollView-Proxy-WindowedPopup` with 0xC000027B (RO_E_CLOSED) in GDI+ and D2D-SW. The hand-off visual was still parented under the popup's island when that island disposed it. Fix: `CPopup::ReleaseDCompResourcesForWindowedPopup` now removes the popup's hit proxies.
  - Popup and windowing pages rerun with the fix (`coverage-scrollgaps-popupfix`): 27/27 in each mode.
- **Sanity subset** (opt-in off; `Run-RenderBench.ps1 -Modes gdiplus,d2d-sw -InputMode programmatic -Quick`; S3, S4 and S8 with 2 repetitions, S1, S23 and S24 with 1; DManip bench vs this branch, `C:\mux-ir\results\scrollgaps-sanity`). All 44 gates pass. Most cells are within ±1 MB, but the <1 MB budget is **missed** in four cells (steady median):
  - Gallery D2D-SW +5.3 MB. This is consistent across 5 runs after the change (91.1–92.6 vs 85.8–86.6 before), while peak dropped 105.9 → 100.8. The `WINUI_IMMEDIATE_STATS` paint, damage and full-frame totals match the DManip build to within 2 %. The cause was not isolated.
  - DataGrid D2D-SW +2.2 MB. The per-run ranges overlap.
  - Inbox GDI+ +1.9 MB, from one outlier run.
  - AppShell GDI+ +1.2 MB.
