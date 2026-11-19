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
- GPU memory is `IDXGIAdapter3::QueryVideoMemoryInfo` for this process, summed over adapters. On a machine without a
  hardware GPU (for example a Hyper-V VM), it reads 0, and D2D-HW correctly fails `DeviceMatchesMode`.
- Measure fre builds. chk builds capture an allocation stack per allocation.
