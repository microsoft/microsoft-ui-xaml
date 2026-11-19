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
| `RenderBench\` | WinUI harness app. Scenes S1 *settings* and S2 *dashboard*; phases Warmup → Steady → Interaction → Recovery; `Microsoft-WinUI-RenderBench` markers; memory, GPU, GDI and CPU sampling; SendInput clicks; screenshot |
| `RenderBench.Core\` | Shared options, mode names, EventSource, and the metrics model and calculator |
| `RenderBench.Analyzer\` | `analyze <etl>` → `metrics.json` + gate, `gate <json>...`, `report <dir>` → `report.md` |
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

## Gate (per run)

`HarnessRunStarted`, `HarnessCompleted`, `BackendSelectedPresent`, `RequestReachedRuntime`, `EffectiveMatchesRequested`,
`DeviceMatchesMode` (D2D-SW must be WARP, D2D-HW must not be), `FramesObserved`, `SteadyStateIsIdle` (at most 6 drawn
frames/min), `MemorySampled`, `NoUnsupportedContent`. Reports compute medians from gated runs only.

## Notes

- R1* is input-injected → next drawn UI-thread frame end. It's a proxy; visible latency needs DWM present correlation (Gpu profile).
- GPU memory is `IDXGIAdapter3::QueryVideoMemoryInfo` for this process. `GpuLocalBytes`/`GpuNonLocalBytes` are
  summed over all adapters, as before. Each sample also records per-adapter usage (`GpuAdapterMemorySample`, keyed by
  LUID), and the report's "M3 ... render adapter" row uses the adapter whose LUID is in `DeviceInfo`. On a machine
  without a hardware GPU (for example a Hyper-V VM), GPU memory reads 0.
- Measure fre builds. chk builds capture an allocation stack per allocation.

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
