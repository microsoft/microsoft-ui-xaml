using System.Diagnostics.Tracing;

namespace RenderBench;

public enum BenchPhase
{
    Warmup = 1,
    Steady = 2,
    Interaction = 3,
    Recovery = 4,
}

// Harness markers for the comparative runs. Self-describing (TraceLogging) format so the analyzer parses it the same
// way as the native Microsoft-WinUI-RenderingBackend provider. GUID = name hash: 114b4a9f-4cba-5c88-a321-355d52c822e6.
[EventSource(Name = ProviderName)]
public sealed class BenchEventSource : EventSource
{
    public const string ProviderName = "Microsoft-WinUI-RenderBench";
    public static readonly Guid ProviderId = new("114b4a9f-4cba-5c88-a321-355d52c822e6");
    public static readonly BenchEventSource Log = new();

    BenchEventSource() : base(EventSourceSettings.EtwSelfDescribingEventFormat) { }

    [Event(1, Level = EventLevel.Informational)]
    public void RunStarted(string Scene, string RequestedBackend, int Seed, int Width, int Height, double RasterizationScale)
        => WriteEvent(1, Scene, RequestedBackend, Seed, Width, Height, RasterizationScale);

    [Event(2, Level = EventLevel.Informational)]
    public void PhaseBegin(int Phase, string PhaseName) => WriteEvent(2, Phase, PhaseName);

    [Event(3, Level = EventLevel.Informational)]
    public void PhaseEnd(int Phase, string PhaseName) => WriteEvent(3, Phase, PhaseName);

    [Event(4, Level = EventLevel.Informational)]
    public void SceneReady(string Scene, int ElementCount) => WriteEvent(4, Scene, ElementCount);

    [Event(5, Level = EventLevel.Informational)]
    public void InputInjected(long InputId, string Kind, string Target) => WriteEvent(5, InputId, Kind, Target);

    [Event(6, Level = EventLevel.Informational)]
    public void MemorySample(
        int Phase,
        long PrivateBytes,
        long WorkingSetBytes,
        long PeakWorkingSetBytes,
        long GpuLocalBytes,
        long GpuNonLocalBytes,
        int GdiObjects,
        int UserObjects,
        double CpuUserMs,
        double CpuKernelMs)
        => WriteEvent(6, Phase, PrivateBytes, WorkingSetBytes, PeakWorkingSetBytes, GpuLocalBytes, GpuNonLocalBytes,
            GdiObjects, UserObjects, CpuUserMs, CpuKernelMs);

    [Event(7, Level = EventLevel.Verbose)]
    public void UpdateApplied(long Tick, int ChangedCount) => WriteEvent(7, Tick, ChangedCount);

    [Event(8, Level = EventLevel.Informational)]
    public void RunCompleted(int Status, string Detail) => WriteEvent(8, Status, Detail);

    [Event(9, Level = EventLevel.Informational)]
    public void ScreenshotSaved(string Path, int Width, int Height, string Method) => WriteEvent(9, Path, Width, Height, Method);

    // ---- Extended scenes S3-S10 ----
    [Event(11, Level = EventLevel.Informational)]
    public void InputModeSelected(string Mode, string Reason) => WriteEvent(11, Mode, Reason);

    [Event(12, Level = EventLevel.Informational)]
    public void SceneTraits(bool AnimatesWhileIdle, bool ContinuousInteraction) => WriteEvent(12, AnimatesWhileIdle, ContinuousInteraction);

    [Event(13, Level = EventLevel.Informational)]
    public void DisplayInfo(double RefreshHz) => WriteEvent(13, RefreshHz);

    [Event(14, Level = EventLevel.Informational)]
    public void SceneStep(int Step, string StepName, int Windows) => WriteEvent(14, Step, StepName, Windows);

    [Event(15, Level = EventLevel.Informational)]
    public void SceneValue(string Name, double Value) => WriteEvent(15, Name, Value);

    [Event(16, Level = EventLevel.Informational)]
    public void PopupOpened(long InputId, string Popup) => WriteEvent(16, InputId, Popup);

    // ---- Full-backend scenes S11-S23 ----
    // S15: the UI thread is about to be blocked for DurationMs (planned) / was blocked for DurationMs (measured).
    [Event(17, Level = EventLevel.Informational)]
    public void UiBlockBegin(int BlockId, double DurationMs) => WriteEvent(17, BlockId, DurationMs);

    [Event(18, Level = EventLevel.Informational)]
    public void UiBlockEnd(int BlockId, double DurationMs) => WriteEvent(18, BlockId, DurationMs);
    // ---- Added by the D2D-HW branch ----
    // Per-adapter GPU memory for this process (IDXGIAdapter3::QueryVideoMemoryInfo), one event per adapter per sample,
    // emitted next to MemorySample. AdapterLuid uses the DeviceInfo packing so the analyzer can pick the render adapter.
    [Event(10, Level = EventLevel.Informational)]
    public void GpuAdapterMemorySample(int Phase, ulong AdapterLuid, long LocalBytes, long NonLocalBytes)
        => WriteEvent(10, Phase, AdapterLuid, LocalBytes, NonLocalBytes);
    // ---- Added by immediate-rendering-d2d-sw: per-thread CPU split (W1/W2), ids 20+ ----

    // Cumulative CPU (user + kernel) per ThreadCategory since the sampler started, including threads that have exited.
    [Event(20, Level = EventLevel.Informational)]
    public void ThreadCpuSample(int Phase, double UiMs, double WarpRasterizerMs, double D2DDWriteMs, double CompositionMs,
        double OtherMs, int ThreadCount)
        => WriteEvent(20, Phase, UiMs, WarpRasterizerMs, D2DDWriteMs, CompositionMs, OtherMs, ThreadCount);

    // Emitted once per start-address module at run end, so the classification can be checked against reality.
    [Event(21, Level = EventLevel.Informational)]
    public void ThreadModuleSummary(string Module, string Category, int Threads, double CpuMs)
        => WriteEvent(21, Module, Category, Threads, CpuMs);

    // File versions of the raster stack (d3d10warp, d2d1, dwrite, ...); WARP golden images are valid per version.
    [Event(22, Level = EventLevel.Informational)]
    public void RasterModuleVersion(string Module, string Version) => WriteEvent(22, Module, Version);
    // ---- End immediate-rendering-d2d-sw ----
}
