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
- [Required WinUI instrumentation](#required-winui-instrumentation)
- [Comparative report format](#comparative-report-format)
- [Measurement pitfalls](#measurement-pitfalls)

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
