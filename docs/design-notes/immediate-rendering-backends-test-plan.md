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
| `GDIP` | Immediate rendering, GDI+ painter (CPU) with DirectWrite text |
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

**D2D-SW readiness (prototype).** The following are in place for D2D-SW:

- **Per-thread CPU split for W1/W2.** The harness emits `ThreadCpuSample`. It classifies threads by start module into
  UI, WARP rasterizer, D2D/DWrite, composition, and other. `ntdll` thread-pool threads count as other, so attribute
  those with Timing-profile stacks in WPA.
- **`DeviceLost` event** on the native provider.
- **Documented W5 copy contract.** For frames presented through a swap chain, upload bytes are damaged pixels × 4.
  Offscreen frames report 0.
- **WARP determinism test.** WARP output is pixel-identical across devices, so D2D-SW renders can serve as the C1
  golden reference. The reference is pinned to the recorded `d3d10warp`/`d2d1`/`dwrite` versions.

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
